#include "documentparser.h"
#include <QFile>
#include <QTextStream>
#include <QXmlStreamReader>
#include <QDebug>

#ifdef HAVE_POPPLER
#include <poppler/qt6/poppler-qt6.h>
#endif

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
    // This is a placeholder implementation
    // In a real implementation, you would use QuaZip or similar library
    // For now, we'll return a simple message
    
    qWarning() << "DOCX parsing requires QuaZip library (not implemented in this basic version)";
    
    // Try to read as plain text as fallback
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        // DOCX files are binary, so this won't work well
        // but it's a fallback
        return QString("DOCX support requires QuaZip library\nFile: %1").arg(filePath);
    }
    
    return QString();
}

DocumentStructure DocumentParser::parseDocxXml(const QString &xmlContent)
{
    DocumentStructure structure;
    QXmlStreamReader xml(xmlContent);
    
    DocumentElement currentElement;
    QString currentText;
    
    while (!xml.atEnd()) {
        xml.readNext();
        
        if (xml.isStartElement()) {
            if (xml.name() == QString("p")) {
                // Paragraph
                currentElement.type = DocumentElement::Paragraph;
                currentText.clear();
            } else if (xml.name() == QString("t")) {
                // Text run
                currentText += xml.readElementText();
            } else if (xml.name() == QString("pStyle")) {
                // Check for heading style
                QString val = xml.attributes().value("val").toString();
                if (val.startsWith("Heading")) {
                    currentElement.type = DocumentElement::Heading;
                    currentElement.level = val.mid(7).toInt(); // Extract number from "Heading1"
                }
            }
        } else if (xml.isEndElement()) {
            if (xml.name() == QString("p")) {
                // End of paragraph
                currentElement.content = currentText;
                if (!currentText.trimmed().isEmpty()) {
                    structure.elements.append(currentElement);
                }
                currentElement = DocumentElement();
                currentText.clear();
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
    
    return result;
}
