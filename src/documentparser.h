#ifndef DOCUMENTPARSER_H
#define DOCUMENTPARSER_H

#include <QObject>
#include <QString>
#include <QVector>

struct DocumentElement {
    enum Type {
        Heading,
        Paragraph,
        ListItem,
        TableCell,
        Text
    };
    
    Type type;
    int level;  // For headings, list depth
    QString content;
    
    DocumentElement() : type(Text), level(0) {}
};

struct DocumentStructure {
    QVector<DocumentElement> elements;
};

class DocumentParser : public QObject
{
    Q_OBJECT

public:
    explicit DocumentParser(QObject *parent = nullptr);
    ~DocumentParser();

    // Parse different document formats
    QString parsePdf(const QString &filePath);
    DocumentStructure parseDocx(const QString &filePath);
    
    // Format structured document as text
    QString formatStructure(const DocumentStructure &structure);

private:
    QString extractTextFromZip(const QString &filePath, const QString &entryName);
    DocumentStructure parseDocxXml(const QString &xmlContent);
};

#endif // DOCUMENTPARSER_H
