#include "diffview.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QTextBlock>
#include <QScrollBar>
#include <QTextFormat>
#include <QFont>
#include <QFontMetrics>

namespace {
int lineToPosition(const QTextDocument *doc, int lineNumber)
{
    QTextBlock block = doc->findBlockByNumber(lineNumber);
    if (!block.isValid()) {
        return 0;
    }
    return block.position();
}

void appendRowSelection(QTextEdit *editor,
                        QList<QTextEdit::ExtraSelection> &selections,
                        int lineNumber,
                        const QTextCharFormat &format)
{
    QTextBlock block = editor->document()->findBlockByNumber(lineNumber);
    if (!block.isValid()) {
        return;
    }

    QTextEdit::ExtraSelection selection;
    selection.cursor = QTextCursor(block);
    selection.cursor.clearSelection();
    selection.format = format;
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selections.append(selection);
}

void resizeLineNumberGutter(QTextEdit *gutter, int digitCount)
{
    const int visibleDigits = digitCount < 2 ? 2 : digitCount;
    const QFontMetrics metrics(gutter->font());
    const int documentMargins = static_cast<int>(gutter->document()->documentMargin()) * 2;
    gutter->setFixedWidth(metrics.horizontalAdvance(QString(visibleDigits, QChar('9'))) + documentMargins + 14);
}
}

DiffView::DiffView(QWidget *parent)
    : QWidget(parent)
    , ignoreWhitespace(false)
    , ignoreReflow(false)
    , ignorePunctuation(false)
    , currentChangeIndex(-1)
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
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QFont diffFont("Consolas");
    diffFont.setStyleHint(QFont::Monospace);
    diffFont.setPointSize(10);
    
    // Create splitter for side-by-side view
    splitter = new QSplitter(Qt::Horizontal, this);
    
    // Left pane
    QWidget *leftWidget = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    QLabel *leftLabel = new QLabel(tr("Original"));
    leftLabel->setStyleSheet("font-weight: bold; padding: 6px 8px; background-color: #252e3a; color: #f4f7fa; border: 1px solid #334052; border-bottom: 0;");
    QWidget *leftTextArea = new QWidget();
    QHBoxLayout *leftTextLayout = new QHBoxLayout(leftTextArea);
    leftGutter = new QTextEdit();
    leftGutter->setObjectName("lineNumberGutter");
    leftGutter->setReadOnly(true);
    leftGutter->setLineWrapMode(QTextEdit::NoWrap);
    leftGutter->setFocusPolicy(Qt::NoFocus);
    leftGutter->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    leftGutter->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    leftGutter->setTextInteractionFlags(Qt::NoTextInteraction);
    leftGutter->setFont(diffFont);
    leftGutter->document()->setDocumentMargin(5);
    resizeLineNumberGutter(leftGutter, 2);
    leftPane = new QTextEdit();
    leftPane->setReadOnly(true);
    leftPane->setLineWrapMode(QTextEdit::NoWrap);
    leftPane->setFont(diffFont);
    leftPane->document()->setDocumentMargin(5);
    leftTextLayout->addWidget(leftGutter);
    leftTextLayout->addWidget(leftPane);
    leftTextLayout->setContentsMargins(0, 0, 0, 0);
    leftTextLayout->setSpacing(0);
    leftLayout->addWidget(leftLabel);
    leftLayout->addWidget(leftTextArea);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    
    // Right pane
    QWidget *rightWidget = new QWidget();
    QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);
    QLabel *rightLabel = new QLabel(tr("Modified"));
    rightLabel->setStyleSheet("font-weight: bold; padding: 6px 8px; background-color: #252e3a; color: #f4f7fa; border: 1px solid #334052; border-bottom: 0;");
    QWidget *rightTextArea = new QWidget();
    QHBoxLayout *rightTextLayout = new QHBoxLayout(rightTextArea);
    rightGutter = new QTextEdit();
    rightGutter->setObjectName("lineNumberGutter");
    rightGutter->setReadOnly(true);
    rightGutter->setLineWrapMode(QTextEdit::NoWrap);
    rightGutter->setFocusPolicy(Qt::NoFocus);
    rightGutter->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    rightGutter->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    rightGutter->setTextInteractionFlags(Qt::NoTextInteraction);
    rightGutter->setFont(diffFont);
    rightGutter->document()->setDocumentMargin(5);
    resizeLineNumberGutter(rightGutter, 2);
    rightPane = new QTextEdit();
    rightPane->setReadOnly(true);
    rightPane->setLineWrapMode(QTextEdit::NoWrap);
    rightPane->setFont(diffFont);
    rightPane->document()->setDocumentMargin(5);
    rightTextLayout->addWidget(rightGutter);
    rightTextLayout->addWidget(rightPane);
    rightTextLayout->setContentsMargins(0, 0, 0, 0);
    rightTextLayout->setSpacing(0);
    rightLayout->addWidget(rightLabel);
    rightLayout->addWidget(rightTextArea);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    
    splitter->addWidget(leftWidget);
    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    
    mainLayout->addWidget(splitter);
    
    auto syncVerticalScroll = [this](int value) {
        leftPane->verticalScrollBar()->setValue(value);
        rightPane->verticalScrollBar()->setValue(value);
        leftGutter->verticalScrollBar()->setValue(value);
        rightGutter->verticalScrollBar()->setValue(value);
    };

    connect(leftPane->verticalScrollBar(), &QScrollBar::valueChanged,
            this, syncVerticalScroll);
    connect(rightPane->verticalScrollBar(), &QScrollBar::valueChanged,
            this, syncVerticalScroll);
    connect(leftGutter->verticalScrollBar(), &QScrollBar::valueChanged,
            this, syncVerticalScroll);
    connect(rightGutter->verticalScrollBar(), &QScrollBar::valueChanged,
            this, syncVerticalScroll);
    
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
    QString displayText1 = text1;
    QString displayText2 = text2;
    
    if (ignoreWhitespace) {
        displayText1 = diffEngine->normalizeWhitespace(displayText1);
        displayText2 = diffEngine->normalizeWhitespace(displayText2);
    }

    if (ignoreReflow) {
        displayText1 = diffEngine->normalizeReflow(displayText1);
        displayText2 = diffEngine->normalizeReflow(displayText2);
    }
    
    if (ignorePunctuation) {
        displayText1 = diffEngine->removePunctuation(displayText1);
        displayText2 = diffEngine->removePunctuation(displayText2);
    }
    
    // Compute and display aligned line pairs so inserted/deleted blocks stay aligned.
    const QVector<DiffLinePair> alignedLines = diffEngine->computeAlignedLines(displayText1, displayText2);

    QStringList leftLines;
    QStringList rightLines;
    QStringList leftLineNumbers;
    QStringList rightLineNumbers;
    leftLines.reserve(alignedLines.size());
    rightLines.reserve(alignedLines.size());
    leftLineNumbers.reserve(alignedLines.size());
    rightLineNumbers.reserve(alignedLines.size());

    int leftLineNumber = 1;
    int rightLineNumber = 1;
    const int leftLineNumberWidth = QString::number(displayText1.count('\n') + 1).length();
    const int rightLineNumberWidth = QString::number(displayText2.count('\n') + 1).length();
    resizeLineNumberGutter(leftGutter, leftLineNumberWidth);
    resizeLineNumberGutter(rightGutter, rightLineNumberWidth);

    changeLineIndices.clear();
    for (int i = 0; i < alignedLines.size(); ++i) {
        leftLines.append(alignedLines[i].leftLine);
        rightLines.append(alignedLines[i].rightLine);

        if (alignedLines[i].type == DiffHunk::Added) {
            leftLineNumbers.append(QString());
            rightLineNumbers.append(QString("%1").arg(rightLineNumber++, rightLineNumberWidth));
        } else if (alignedLines[i].type == DiffHunk::Deleted) {
            leftLineNumbers.append(QString("%1").arg(leftLineNumber++, leftLineNumberWidth));
            rightLineNumbers.append(QString());
        } else {
            leftLineNumbers.append(QString("%1").arg(leftLineNumber++, leftLineNumberWidth));
            rightLineNumbers.append(QString("%1").arg(rightLineNumber++, rightLineNumberWidth));
        }

        if (alignedLines[i].type != DiffHunk::Unchanged) {
            changeLineIndices.append(i);
        }
    }
    currentChangeIndex = changeLineIndices.isEmpty() ? -1 : 0;

    leftPane->setPlainText(leftLines.join("\n"));
    rightPane->setPlainText(rightLines.join("\n"));
    leftGutter->setPlainText(leftLineNumbers.join("\n"));
    rightGutter->setPlainText(rightLineNumbers.join("\n"));

    highlightDifferences(alignedLines);
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

void DiffView::highlightDifferences(const QVector<DiffLinePair> &lines)
{
    QTextCharFormat addedFormat;
    addedFormat.setBackground(QColor(25, 78, 52));
    addedFormat.setForeground(QColor(220, 255, 232));
    
    QTextCharFormat deletedFormat;
    deletedFormat.setBackground(QColor(88, 38, 49));
    deletedFormat.setForeground(QColor(255, 224, 229));
    
    QTextCharFormat modifiedFormat;
    modifiedFormat.setBackground(QColor(82, 70, 25));
    modifiedFormat.setForeground(QColor(255, 241, 181));

    QList<QTextEdit::ExtraSelection> leftSelections;
    QList<QTextEdit::ExtraSelection> rightSelections;
    QList<QTextEdit::ExtraSelection> leftGutterSelections;
    QList<QTextEdit::ExtraSelection> rightGutterSelections;

    for (int lineIndex = 0; lineIndex < lines.size(); ++lineIndex) {
        if (lines[lineIndex].type == DiffHunk::Added) {
            appendRowSelection(rightPane, rightSelections, lineIndex, addedFormat);
            appendRowSelection(rightGutter, rightGutterSelections, lineIndex, addedFormat);
        } else if (lines[lineIndex].type == DiffHunk::Deleted) {
            appendRowSelection(leftPane, leftSelections, lineIndex, deletedFormat);
            appendRowSelection(leftGutter, leftGutterSelections, lineIndex, deletedFormat);
        } else if (lines[lineIndex].type == DiffHunk::Modified) {
            appendRowSelection(leftPane, leftSelections, lineIndex, modifiedFormat);
            appendRowSelection(rightPane, rightSelections, lineIndex, modifiedFormat);
            appendRowSelection(leftGutter, leftGutterSelections, lineIndex, modifiedFormat);
            appendRowSelection(rightGutter, rightGutterSelections, lineIndex, modifiedFormat);
        }
    }

    leftPane->setExtraSelections(leftSelections);
    rightPane->setExtraSelections(rightSelections);
    leftGutter->setExtraSelections(leftGutterSelections);
    rightGutter->setExtraSelections(rightGutterSelections);
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

void DiffView::goToNextChange()
{
    if (changeLineIndices.isEmpty()) {
        return;
    }

    currentChangeIndex = (currentChangeIndex + 1) % changeLineIndices.size();
    const int lineIndex = changeLineIndices[currentChangeIndex];

    QTextCursor leftCursor(leftPane->textCursor());
    leftCursor.setPosition(lineToPosition(leftPane->document(), lineIndex));
    leftPane->setTextCursor(leftCursor);
    leftPane->ensureCursorVisible();

    QTextCursor rightCursor(rightPane->textCursor());
    rightCursor.setPosition(lineToPosition(rightPane->document(), lineIndex));
    rightPane->setTextCursor(rightCursor);
    rightPane->ensureCursorVisible();
}

void DiffView::goToPreviousChange()
{
    if (changeLineIndices.isEmpty()) {
        return;
    }

    currentChangeIndex = (currentChangeIndex - 1 + changeLineIndices.size()) % changeLineIndices.size();
    const int lineIndex = changeLineIndices[currentChangeIndex];

    QTextCursor leftCursor(leftPane->textCursor());
    leftCursor.setPosition(lineToPosition(leftPane->document(), lineIndex));
    leftPane->setTextCursor(leftCursor);
    leftPane->ensureCursorVisible();

    QTextCursor rightCursor(rightPane->textCursor());
    rightCursor.setPosition(lineToPosition(rightPane->document(), lineIndex));
    rightPane->setTextCursor(rightCursor);
    rightPane->ensureCursorVisible();
}
