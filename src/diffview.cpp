#include "diffview.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>

DiffView::DiffView(QWidget *parent)
    : QWidget(parent)
    , ignoreWhitespace(false)
    , ignoreReflow(false)
    , ignorePunctuation(false)
{
    diffEngine = new DiffEngine(this);
    docParser = new DocumentParser(this);
    setupUI();
}

DiffView::~DiffView()
{
}

void DiffView::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Create splitter for side-by-side view
    splitter = new QSplitter(Qt::Horizontal, this);
    
    // Left pane
    QWidget *leftWidget = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    QLabel *leftLabel = new QLabel(tr("Original"));
    leftLabel->setStyleSheet("font-weight: bold; padding: 5px; background-color: #f0f0f0;");
    leftPane = new QTextEdit();
    leftPane->setReadOnly(true);
    leftPane->setLineWrapMode(QTextEdit::NoWrap);
    leftLayout->addWidget(leftLabel);
    leftLayout->addWidget(leftPane);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    
    // Right pane
    QWidget *rightWidget = new QWidget();
    QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);
    QLabel *rightLabel = new QLabel(tr("Modified"));
    rightLabel->setStyleSheet("font-weight: bold; padding: 5px; background-color: #f0f0f0;");
    rightPane = new QTextEdit();
    rightPane->setReadOnly(true);
    rightPane->setLineWrapMode(QTextEdit::NoWrap);
    rightLayout->addWidget(rightLabel);
    rightLayout->addWidget(rightPane);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    
    splitter->addWidget(leftWidget);
    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    
    mainLayout->addWidget(splitter);
    
    // Synchronize scroll bars
    connect(leftPane->verticalScrollBar(), &QScrollBar::valueChanged,
            rightPane->verticalScrollBar(), &QScrollBar::setValue);
    connect(rightPane->verticalScrollBar(), &QScrollBar::valueChanged,
            leftPane->verticalScrollBar(), &QScrollBar::setValue);
    
    connect(leftPane->horizontalScrollBar(), &QScrollBar::valueChanged,
            rightPane->horizontalScrollBar(), &QScrollBar::setValue);
    connect(rightPane->horizontalScrollBar(), &QScrollBar::valueChanged,
            leftPane->horizontalScrollBar(), &QScrollBar::setValue);
}

void DiffView::loadFiles(const QString &file1, const QString &file2)
{
    currentFile1 = file1;
    currentFile2 = file2;
    
    QFileInfo info1(file1);
    QFileInfo info2(file2);
    
    QString ext1 = info1.suffix().toLower();
    QString ext2 = info2.suffix().toLower();
    
    // Determine file type and display accordingly
    if (ext1 == "pdf" && ext2 == "pdf") {
        displayPdfDiff(file1, file2);
    } else if (ext1 == "docx" && ext2 == "docx") {
        displayDocxDiff(file1, file2);
    } else if ((ext1 == "txt" || ext1 == "md") && (ext2 == "txt" || ext2 == "md")) {
        // Read text files
        QFile f1(file1), f2(file2);
        QString text1, text2;
        
        if (f1.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&f1);
            text1 = in.readAll();
            f1.close();
        }
        
        if (f2.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&f2);
            text2 = in.readAll();
            f2.close();
        }
        
        displayTextDiff(text1, text2);
    } else {
        // Try as text by default
        QFile f1(file1), f2(file2);
        QString text1, text2;
        
        if (f1.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&f1);
            text1 = in.readAll();
            f1.close();
        }
        
        if (f2.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&f2);
            text2 = in.readAll();
            f2.close();
        }
        
        displayTextDiff(text1, text2);
    }
}

void DiffView::displayTextDiff(const QString &text1, const QString &text2)
{
    // Apply preprocessing based on options
    QString processedText1 = text1;
    QString processedText2 = text2;
    
    if (ignoreWhitespace) {
        processedText1 = diffEngine->normalizeWhitespace(processedText1);
        processedText2 = diffEngine->normalizeWhitespace(processedText2);
    }
    
    if (ignorePunctuation) {
        processedText1 = diffEngine->removePunctuation(processedText1);
        processedText2 = diffEngine->removePunctuation(processedText2);
    }
    
    // Compute diff
    QVector<DiffHunk> hunks = diffEngine->computeDiff(processedText1, processedText2);
    
    // Display in panes
    leftPane->setPlainText(text1);
    rightPane->setPlainText(text2);
    
    // Highlight differences
    highlightDifferences(hunks);
}

void DiffView::displayPdfDiff(const QString &file1, const QString &file2)
{
    // Parse PDF files
    QString text1 = docParser->parsePdf(file1);
    QString text2 = docParser->parsePdf(file2);
    
    // For now, display as text diff
    // TODO: Implement page overlay mode
    displayTextDiff(text1, text2);
}

void DiffView::displayDocxDiff(const QString &file1, const QString &file2)
{
    // Parse DOCX with rich structure
    DocumentStructure doc1 = docParser->parseDocx(file1);
    DocumentStructure doc2 = docParser->parseDocx(file2);
    
    // Format as text preserving structure
    QString text1 = docParser->formatStructure(doc1);
    QString text2 = docParser->formatStructure(doc2);
    
    displayTextDiff(text1, text2);
}

void DiffView::highlightDifferences(const QVector<DiffHunk> &hunks)
{
    QTextCursor leftCursor(leftPane->document());
    QTextCursor rightCursor(rightPane->document());
    
    QTextCharFormat addedFormat;
    addedFormat.setBackground(QColor(200, 255, 200)); // Light green
    
    QTextCharFormat deletedFormat;
    deletedFormat.setBackground(QColor(255, 200, 200)); // Light red
    
    QTextCharFormat modifiedFormat;
    modifiedFormat.setBackground(QColor(255, 255, 200)); // Light yellow
    
    for (const DiffHunk &hunk : hunks) {
        if (hunk.type == DiffHunk::Added) {
            rightCursor.setPosition(hunk.rightStart);
            rightCursor.setPosition(hunk.rightEnd, QTextCursor::KeepAnchor);
            rightCursor.setCharFormat(addedFormat);
        } else if (hunk.type == DiffHunk::Deleted) {
            leftCursor.setPosition(hunk.leftStart);
            leftCursor.setPosition(hunk.leftEnd, QTextCursor::KeepAnchor);
            leftCursor.setCharFormat(deletedFormat);
        } else if (hunk.type == DiffHunk::Modified) {
            leftCursor.setPosition(hunk.leftStart);
            leftCursor.setPosition(hunk.leftEnd, QTextCursor::KeepAnchor);
            leftCursor.setCharFormat(modifiedFormat);
            
            rightCursor.setPosition(hunk.rightStart);
            rightCursor.setPosition(hunk.rightEnd, QTextCursor::KeepAnchor);
            rightCursor.setCharFormat(modifiedFormat);
        }
    }
}

void DiffView::setIgnoreWhitespace(bool ignore)
{
    ignoreWhitespace = ignore;
    if (!currentFile1.isEmpty() && !currentFile2.isEmpty()) {
        loadFiles(currentFile1, currentFile2);
    }
}

void DiffView::setIgnoreReflow(bool ignore)
{
    ignoreReflow = ignore;
    if (!currentFile1.isEmpty() && !currentFile2.isEmpty()) {
        loadFiles(currentFile1, currentFile2);
    }
}

void DiffView::setIgnorePunctuation(bool ignore)
{
    ignorePunctuation = ignore;
    if (!currentFile1.isEmpty() && !currentFile2.isEmpty()) {
        loadFiles(currentFile1, currentFile2);
    }
}
