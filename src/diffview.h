#ifndef DIFFVIEW_H
#define DIFFVIEW_H

#include <QWidget>
#include <QTextEdit>
#include <QScrollBar>
#include <QSplitter>
#include "diffengine.h"
#include "documentparser.h"

class DiffView : public QWidget
{
    Q_OBJECT

public:
    explicit DiffView(QWidget *parent = nullptr);
    ~DiffView();

    void setIgnoreWhitespace(bool ignore);
    void setIgnoreReflow(bool ignore);
    void setIgnorePunctuation(bool ignore);

public slots:
    void loadFiles(const QString &file1, const QString &file2);

private:
    void setupUI();
    void displayTextDiff(const QString &text1, const QString &text2);
    void displayPdfDiff(const QString &file1, const QString &file2);
    void displayDocxDiff(const QString &file1, const QString &file2);
    void highlightDifferences(const QVector<DiffHunk> &hunks);
    
    QTextEdit *leftPane;
    QTextEdit *rightPane;
    QSplitter *splitter;
    
    DiffEngine *diffEngine;
    DocumentParser *docParser;
    
    bool ignoreWhitespace;
    bool ignoreReflow;
    bool ignorePunctuation;
    
    QString currentFile1;
    QString currentFile2;
};

#endif // DIFFVIEW_H
