#include "documentparser.h"
#include <QFile>
#include <QTextStream>
#include <QXmlStreamReader>
#include <QDebug>
#include <QByteArray>
#include <QXmlStreamAttributes>
#include <algorithm>
#include <limits>
#include <optional>
#include <zlib.h>

#ifdef HAVE_POPPLER
#include <poppler/qt6/poppler-qt6.h>
#endif

namespace {
constexpr quint32 ZipLocalFileHeaderSignature = 0x04034b50;
constexpr quint32 ZipCentralDirectorySignature = 0x02014b50;
constexpr quint32 ZipEndOfCentralDirectorySignature = 0x06054b50;
constexpr quint16 ZipStored = 0;
constexpr quint16 ZipDeflated = 8;
constexpr qint64 MaxDocxFileSize = 100ll * 1024 * 1024;
constexpr quint32 MaxDocxEntrySize = 100u * 1024 * 1024;

bool hasBytes(const QByteArray &data, qsizetype offset, qsizetype length)
{
    return offset >= 0 && length >= 0 && offset <= data.size() - length;
}

std::optional<quint16> readUInt16(const QByteArray &data, qsizetype offset)
{
    if (!hasBytes(data, offset, 2)) {
        return std::nullopt;
    }

    return static_cast<quint16>(static_cast<unsigned char>(data[offset]))
        | static_cast<quint16>(static_cast<unsigned char>(data[offset + 1]) << 8);
}

std::optional<quint32> readUInt32(const QByteArray &data, qsizetype offset)
{
    const std::optional<quint16> low = readUInt16(data, offset);
    const std::optional<quint16> high = readUInt16(data, offset + 2);
    if (!low || !high) {
        return std::nullopt;
    }

    return static_cast<quint32>(*low) | (static_cast<quint32>(*high) << 16);
}

qsizetype findEndOfCentralDirectory(const QByteArray &data)
{
    if (data.size() < 22) {
        return -1;
    }

    const qsizetype minOffset = std::max<qsizetype>(0, data.size() - 65557);
    for (qsizetype offset = data.size() - 22;; --offset) {
        const std::optional<quint32> signature = readUInt32(data, offset);
        if (signature && *signature == ZipEndOfCentralDirectorySignature) {
            return offset;
        }

        if (offset == minOffset) {
            break;
        }
    }

    return -1;
}

QByteArray inflateRawDeflate(const QByteArray &compressed, quint32 uncompressedSize)
{
    if (uncompressedSize > MaxDocxEntrySize
        || compressed.size() > std::numeric_limits<uInt>::max()
        || uncompressedSize > std::numeric_limits<uInt>::max()) {
        return QByteArray();
    }

    QByteArray output;
    output.resize(static_cast<qsizetype>(uncompressedSize));
    QByteArray input = compressed;

    z_stream stream = {};
    stream.next_in = reinterpret_cast<Bytef *>(input.data());
    stream.avail_in = static_cast<uInt>(input.size());
    stream.next_out = reinterpret_cast<Bytef *>(output.data());
    stream.avail_out = static_cast<uInt>(output.size());

    if (inflateInit2(&stream, -MAX_WBITS) != Z_OK) {
        return QByteArray();
    }

    const int status = inflate(&stream, Z_FINISH);
    inflateEnd(&stream);

    if (status != Z_STREAM_END) {
        return QByteArray();
    }

    output.resize(static_cast<qsizetype>(stream.total_out));
    return output;
}
}

DocumentParser::DocumentParser(QObject *parent)
    : QObject(parent)
{
}

DocumentParser::~DocumentParser()
{
}

QString DocumentParser::parsePdf(const QString &filePath)
{
#ifdef HAVE_POPPLER
    // Use Poppler to extract text from PDF
    Poppler::Document *document = Poppler::Document::load(filePath);
    
    if (!document || document->isLocked()) {
        qWarning() << "Failed to load PDF:" << filePath;
        return QString();
    }
    
    QString text;
    int numPages = document->numPages();
    
    for (int i = 0; i < numPages; ++i) {
        Poppler::Page *page = document->page(i);
        if (page) {
            text += QString("--- Page %1 ---\n").arg(i + 1);
            text += page->text(QRectF());
            text += "\n\n";
            delete page;
        }
    }
    
    delete document;
    return text;
#else
    qWarning() << "PDF support not available (Poppler not found)";
    return QString("PDF support requires Poppler library\nFile: %1").arg(filePath);
#endif
}

DocumentStructure DocumentParser::parseDocx(const QString &filePath)
{
    DocumentStructure structure;
    
    // DOCX is a ZIP file containing XML
    // For simplicity, we'll extract text from document.xml
    QString xmlContent = extractTextFromZip(filePath, "word/document.xml");
    
    if (xmlContent.isEmpty()) {
        qWarning() << "Failed to parse DOCX:" << filePath;
        return structure;
    }
    
    return parseDocxXml(xmlContent);
}

QString DocumentParser::extractTextFromZip(const QString &filePath, const QString &entryName)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open DOCX file:" << filePath;
        return QString();
    }

    if (file.size() > MaxDocxFileSize) {
        qWarning() << "DOCX file is too large to parse safely:" << filePath;
        return QString();
    }

    const QByteArray zipData = file.readAll();
    if (zipData.size() < 22) {
        qWarning() << "Invalid DOCX ZIP archive:" << filePath;
        return QString();
    }

    const qsizetype eocdOffset = findEndOfCentralDirectory(zipData);
    if (eocdOffset < 0 || eocdOffset + 22 > zipData.size()) {
        qWarning() << "DOCX ZIP central directory not found:" << filePath;
        return QString();
    }

    const std::optional<quint16> entryCount = readUInt16(zipData, eocdOffset + 10);
    const std::optional<quint32> centralDirectoryOffset = readUInt32(zipData, eocdOffset + 16);
    if (!entryCount || !centralDirectoryOffset || *centralDirectoryOffset > static_cast<quint32>(zipData.size())) {
        qWarning() << "Invalid DOCX ZIP central directory metadata:" << filePath;
        return QString();
    }

    const QByteArray expectedEntryName = entryName.toUtf8();
    qsizetype offset = static_cast<qsizetype>(*centralDirectoryOffset);

    for (quint16 entryIndex = 0; entryIndex < *entryCount; ++entryIndex) {
        const std::optional<quint32> centralDirectorySignature = readUInt32(zipData, offset);
        if (!centralDirectorySignature || *centralDirectorySignature != ZipCentralDirectorySignature) {
            qWarning() << "Invalid DOCX ZIP central directory entry:" << filePath;
            return QString();
        }

        const std::optional<quint16> flags = readUInt16(zipData, offset + 8);
        const std::optional<quint16> compressionMethod = readUInt16(zipData, offset + 10);
        const std::optional<quint32> compressedSize = readUInt32(zipData, offset + 20);
        const std::optional<quint32> uncompressedSize = readUInt32(zipData, offset + 24);
        const std::optional<quint16> fileNameLength = readUInt16(zipData, offset + 28);
        const std::optional<quint16> extraLength = readUInt16(zipData, offset + 30);
        const std::optional<quint16> commentLength = readUInt16(zipData, offset + 32);
        const std::optional<quint32> localHeaderOffset = readUInt32(zipData, offset + 42);
        if (!flags || !compressionMethod || !compressedSize || !uncompressedSize
            || !fileNameLength || !extraLength || !commentLength || !localHeaderOffset
            || !hasBytes(zipData, offset, 46 + *fileNameLength + *extraLength + *commentLength)) {
            qWarning() << "Invalid DOCX ZIP entry bounds:" << filePath;
            return QString();
        }

        const QByteArray fileName = zipData.mid(offset + 46, *fileNameLength);
        offset += 46 + *fileNameLength + *extraLength + *commentLength;

        if (fileName != expectedEntryName) {
            continue;
        }

        if ((*flags & 0x1) != 0) {
            qWarning() << "Encrypted DOCX entries are not supported:" << filePath;
            return QString();
        }

        if (*uncompressedSize > MaxDocxEntrySize || *compressedSize > MaxDocxEntrySize) {
            qWarning() << "DOCX entry is too large to parse safely:" << entryName;
            return QString();
        }

        const qsizetype localOffset = static_cast<qsizetype>(*localHeaderOffset);
        const std::optional<quint32> localSignature = readUInt32(zipData, localOffset);
        if (!localSignature || *localSignature != ZipLocalFileHeaderSignature) {
            qWarning() << "Invalid DOCX ZIP local file header:" << filePath;
            return QString();
        }

        const std::optional<quint16> localFileNameLength = readUInt16(zipData, localOffset + 26);
        const std::optional<quint16> localExtraLength = readUInt16(zipData, localOffset + 28);
        if (!localFileNameLength || !localExtraLength
            || !hasBytes(zipData, localOffset, 30 + *localFileNameLength + *localExtraLength)) {
            qWarning() << "Invalid DOCX ZIP local file header bounds:" << filePath;
            return QString();
        }

        const qsizetype dataOffset = localOffset + 30 + *localFileNameLength + *localExtraLength;

        if (!hasBytes(zipData, dataOffset, *compressedSize)) {
            qWarning() << "Invalid DOCX ZIP compressed data bounds:" << filePath;
            return QString();
        }

        const QByteArray compressed = zipData.mid(dataOffset, *compressedSize);
        QByteArray uncompressed;

        if (*compressionMethod == ZipStored) {
            uncompressed = compressed;
        } else if (*compressionMethod == ZipDeflated) {
            uncompressed = inflateRawDeflate(compressed, *uncompressedSize);
        } else {
            qWarning() << "Unsupported DOCX ZIP compression method:" << *compressionMethod;
            return QString();
        }

        if (static_cast<quint32>(uncompressed.size()) != *uncompressedSize) {
            qWarning() << "Failed to decompress DOCX entry:" << entryName;
            return QString();
        }

        return QString::fromUtf8(uncompressed);
    }

    qWarning() << "DOCX entry not found:" << entryName;
    return QString();
}

DocumentStructure DocumentParser::parseDocxXml(const QString &xmlContent)
{
    DocumentStructure structure;
    QXmlStreamReader xml(xmlContent);
    
    DocumentElement currentElement;
    QString currentText;
    bool inParagraph = false;
    bool inTableCell = false;
    
    while (!xml.atEnd()) {
        xml.readNext();
        
        if (xml.isStartElement()) {
            const QStringView name = xml.name();

            if (name == u"tc") {
                inTableCell = true;
            } else if (name == u"p") {
                inParagraph = true;
                currentElement.type = inTableCell ? DocumentElement::TableCell : DocumentElement::Paragraph;
                currentElement.level = 0;
                currentText.clear();
            } else if (name == u"t" || name == u"instrText") {
                currentText += xml.readElementText();
            } else if (name == u"tab") {
                currentText += "\t";
            } else if (name == u"br" || name == u"cr") {
                currentText += "\n";
            } else if (name == u"pStyle" && inParagraph) {
                const QString val = xml.attributes().value(u"val").toString();
                if (val.startsWith("Heading")) {
                    currentElement.type = DocumentElement::Heading;
                    currentElement.level = val.mid(7).toInt();
                }
            } else if (name == u"numPr" && inParagraph && currentElement.type != DocumentElement::Heading) {
                currentElement.type = DocumentElement::ListItem;
            } else if (name == u"ilvl" && currentElement.type == DocumentElement::ListItem) {
                currentElement.level = xml.attributes().value(u"val").toInt();
            }
        } else if (xml.isEndElement()) {
            const QStringView name = xml.name();

            if (name == u"p") {
                currentElement.content = currentText;
                if (!currentText.trimmed().isEmpty()) {
                    structure.elements.append(currentElement);
                }
                currentElement = DocumentElement();
                currentText.clear();
                inParagraph = false;
            } else if (name == u"tc") {
                inTableCell = false;
            }
        }
    }
    
    if (xml.hasError()) {
        qWarning() << "XML parsing error:" << xml.errorString();
    }
    
    return structure;
}

QString DocumentParser::formatStructure(const DocumentStructure &structure)
{
    QString result;
    
    for (const DocumentElement &element : structure.elements) {
        switch (element.type) {
        case DocumentElement::Heading:
            result += QString("#").repeated(element.level) + " " + element.content + "\n\n";
            break;
        case DocumentElement::ListItem:
            result += QString("  ").repeated(element.level) + "* " + element.content + "\n";
            break;
        case DocumentElement::TableCell:
            result += "| " + element.content + " ";
            break;
        case DocumentElement::Paragraph:
        case DocumentElement::Text:
        default:
            result += element.content + "\n\n";
            break;
        }
    }
    
    return result.trimmed();
}
