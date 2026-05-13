#include "documentparser.h"
#include <QFile>
#include <QTextStream>
#include <QXmlStreamReader>
#include <QDebug>
#include <QByteArray>
#include <QXmlStreamAttributes>
#include <algorithm>
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

quint16 readUInt16(const QByteArray &data, qsizetype offset)
{
    return static_cast<quint16>(static_cast<unsigned char>(data[offset]))
        | static_cast<quint16>(static_cast<unsigned char>(data[offset + 1]) << 8);
}

quint32 readUInt32(const QByteArray &data, qsizetype offset)
{
    return static_cast<quint32>(readUInt16(data, offset))
        | (static_cast<quint32>(readUInt16(data, offset + 2)) << 16);
}

qsizetype findEndOfCentralDirectory(const QByteArray &data)
{
    const qsizetype minOffset = std::max<qsizetype>(0, data.size() - 65557);
    for (qsizetype offset = data.size() - 22; offset >= minOffset; --offset) {
        if (readUInt32(data, offset) == ZipEndOfCentralDirectorySignature) {
            return offset;
        }
    }

    return -1;
}

QByteArray inflateRawDeflate(const QByteArray &compressed, quint32 uncompressedSize)
{
    QByteArray output;
    output.resize(static_cast<qsizetype>(uncompressedSize));

    z_stream stream = {};
    stream.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(compressed.constData()));
    stream.avail_in = static_cast<uInt>(compressed.size());
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

QString attributeValue(const QXmlStreamAttributes &attributes, QStringView localName)
{
    for (const QXmlStreamAttribute &attribute : attributes) {
        if (attribute.name() == localName) {
            return attribute.value().toString();
        }
    }

    return QString();
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

    const quint16 entryCount = readUInt16(zipData, eocdOffset + 10);
    const quint32 centralDirectoryOffset = readUInt32(zipData, eocdOffset + 16);
    qsizetype offset = centralDirectoryOffset;

    for (quint16 entryIndex = 0; entryIndex < entryCount; ++entryIndex) {
        if (offset + 46 > zipData.size() || readUInt32(zipData, offset) != ZipCentralDirectorySignature) {
            qWarning() << "Invalid DOCX ZIP central directory entry:" << filePath;
            return QString();
        }

        const quint16 flags = readUInt16(zipData, offset + 8);
        const quint16 compressionMethod = readUInt16(zipData, offset + 10);
        const quint32 compressedSize = readUInt32(zipData, offset + 20);
        const quint32 uncompressedSize = readUInt32(zipData, offset + 24);
        const quint16 fileNameLength = readUInt16(zipData, offset + 28);
        const quint16 extraLength = readUInt16(zipData, offset + 30);
        const quint16 commentLength = readUInt16(zipData, offset + 32);
        const quint32 localHeaderOffset = readUInt32(zipData, offset + 42);

        if (offset + 46 + fileNameLength + extraLength + commentLength > zipData.size()) {
            qWarning() << "Invalid DOCX ZIP entry bounds:" << filePath;
            return QString();
        }

        const QString fileName = QString::fromUtf8(zipData.constData() + offset + 46, fileNameLength);
        offset += 46 + fileNameLength + extraLength + commentLength;

        if (fileName != entryName) {
            continue;
        }

        if ((flags & 0x1) != 0) {
            qWarning() << "Encrypted DOCX entries are not supported:" << filePath;
            return QString();
        }

        if (localHeaderOffset + 30 > static_cast<quint32>(zipData.size())
            || readUInt32(zipData, localHeaderOffset) != ZipLocalFileHeaderSignature) {
            qWarning() << "Invalid DOCX ZIP local file header:" << filePath;
            return QString();
        }

        const quint16 localFileNameLength = readUInt16(zipData, localHeaderOffset + 26);
        const quint16 localExtraLength = readUInt16(zipData, localHeaderOffset + 28);
        const qsizetype dataOffset = localHeaderOffset + 30 + localFileNameLength + localExtraLength;

        if (dataOffset + compressedSize > zipData.size()) {
            qWarning() << "Invalid DOCX ZIP compressed data bounds:" << filePath;
            return QString();
        }

        const QByteArray compressed = zipData.mid(dataOffset, compressedSize);
        QByteArray uncompressed;

        if (compressionMethod == ZipStored) {
            uncompressed = compressed;
        } else if (compressionMethod == ZipDeflated) {
            uncompressed = inflateRawDeflate(compressed, uncompressedSize);
        } else {
            qWarning() << "Unsupported DOCX ZIP compression method:" << compressionMethod;
            return QString();
        }

        if (uncompressed.isEmpty() && uncompressedSize != 0) {
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

            if (name == QString("tc")) {
                inTableCell = true;
            } else if (name == QString("p")) {
                inParagraph = true;
                currentElement.type = inTableCell ? DocumentElement::TableCell : DocumentElement::Paragraph;
                currentElement.level = 0;
                currentText.clear();
            } else if (name == QString("t") || name == QString("instrText")) {
                currentText += xml.readElementText();
            } else if (name == QString("tab")) {
                currentText += "\t";
            } else if (name == QString("br") || name == QString("cr")) {
                currentText += "\n";
            } else if (name == QString("pStyle") && inParagraph) {
                const QString val = attributeValue(xml.attributes(), QStringLiteral("val"));
                if (val.startsWith("Heading")) {
                    currentElement.type = DocumentElement::Heading;
                    currentElement.level = val.mid(7).toInt();
                }
            } else if (name == QString("numPr") && inParagraph && currentElement.type != DocumentElement::Heading) {
                currentElement.type = DocumentElement::ListItem;
            } else if (name == QString("ilvl") && currentElement.type == DocumentElement::ListItem) {
                currentElement.level = attributeValue(xml.attributes(), QStringLiteral("val")).toInt();
            }
        } else if (xml.isEndElement()) {
            const QStringView name = xml.name();

            if (name == QString("p")) {
                currentElement.content = currentText;
                if (!currentText.trimmed().isEmpty()) {
                    structure.elements.append(currentElement);
                }
                currentElement = DocumentElement();
                currentText.clear();
                inParagraph = false;
            } else if (name == QString("tc")) {
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
