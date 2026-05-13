#include "mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QSettings>
#include <QFileInfo>

namespace {
const char *LastOriginalKey = "files/lastOriginal";
const char *LastModifiedKey = "files/lastModified";
const char *LastDirectoryKey = "files/lastDirectory";

QString fileDialogFilter()
{
    return QObject::tr("All Supported Files (*.txt *.md *.docx *.pdf);;Text Files (*.txt);;Markdown Files (*.md);;Word Documents (*.docx);;PDF Files (*.pdf);;All Files (*)");
}

QStringList localDroppedFiles(const QMimeData *mimeData)
{
    QStringList files;

    if (!mimeData->hasUrls()) {
        return files;
    }

    for (const QUrl &url : mimeData->urls()) {
        if (!url.isLocalFile()) {
            continue;
        }

        const QString path = url.toLocalFile();
        if (QFileInfo(path).isFile()) {
            files.append(path);
        }
    }

    return files;
}

bool hasExactlyTwoLocalDroppedFiles(const QMimeData *mimeData)
{
    if (!mimeData->hasUrls()) {
        return false;
    }

    int localFileCount = 0;
    for (const QUrl &url : mimeData->urls()) {
        if (!url.isLocalFile() || !QFileInfo(url.toLocalFile()).isFile()) {
            continue;
        }

        ++localFileCount;
        if (localFileCount > 2) {
            return false;
        }
    }

    return localFileCount == 2;
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
    createActions();
    createMenus();
    createToolBar();
    updateRecentPairAction();
    setAcceptDrops(true);
    
    setWindowTitle("DiffyInAJiffy - Side-by-Side Diff Viewer");
    resize(1200, 800);
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    // Create main splitter for folder view and diff view
    mainSplitter = new QSplitter(Qt::Horizontal, this);
    
    // Create folder view (file tree)
    folderView = new FolderView(this);
    
    // Create diff view (side-by-side comparison)
    diffView = new DiffView(this);
    
    // Add widgets to splitter
    mainSplitter->addWidget(folderView);
    mainSplitter->addWidget(diffView);
    
    // Set initial sizes (folder view 20%, diff view 80%)
    mainSplitter->setStretchFactor(0, 1);
    mainSplitter->setStretchFactor(1, 4);
    
    // Connect signals
    connect(folderView, &FolderView::fileSelected, 
            diffView, &DiffView::loadFiles);
    
    setCentralWidget(mainSplitter);
    
    statusBar()->showMessage("Ready");
}

void MainWindow::createActions()
{
    openFilesAction = new QAction(tr("&Open Files..."), this);
    openFilesAction->setShortcut(QKeySequence::Open);
    openFilesAction->setStatusTip(tr("Select an original file and then a modified file"));
    connect(openFilesAction, &QAction::triggered, this, &MainWindow::openFiles);

    recentPairAction = new QAction(tr("Open &Recent Pair"), this);
    recentPairAction->setStatusTip(tr("Reopen the last compared file pair"));
    connect(recentPairAction, &QAction::triggered, this, &MainWindow::openRecentPair);
    
    openFoldersAction = new QAction(tr("Open &Folders..."), this);
    openFoldersAction->setShortcut(tr("Ctrl+Shift+O"));
    openFoldersAction->setStatusTip(tr("Open two folders to compare"));
    connect(openFoldersAction, &QAction::triggered, this, &MainWindow::openFolders);
    
    exitAction = new QAction(tr("E&xit"), this);
    exitAction->setShortcut(QKeySequence::Quit);
    exitAction->setStatusTip(tr("Exit the application"));
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    
    ignoreWhitespaceAction = new QAction(tr("Ignore &Whitespace"), this);
    ignoreWhitespaceAction->setCheckable(true);
    ignoreWhitespaceAction->setStatusTip(tr("Ignore whitespace differences"));
    connect(ignoreWhitespaceAction, &QAction::toggled, 
            this, &MainWindow::toggleIgnoreWhitespace);
    
    ignoreReflowAction = new QAction(tr("Ignore &Reflow"), this);
    ignoreReflowAction->setCheckable(true);
    ignoreReflowAction->setStatusTip(tr("Ignore text reflow differences"));
    connect(ignoreReflowAction, &QAction::toggled, 
            this, &MainWindow::toggleIgnoreReflow);
    
    ignorePunctuationAction = new QAction(tr("Ignore &Punctuation"), this);
    ignorePunctuationAction->setCheckable(true);
    ignorePunctuationAction->setStatusTip(tr("Ignore punctuation-only changes"));
    connect(ignorePunctuationAction, &QAction::toggled, 
            this, &MainWindow::toggleIgnorePunctuation);

        nextChangeAction = new QAction(tr("&Next Change"), this);
        nextChangeAction->setShortcut(tr("F7"));
        nextChangeAction->setStatusTip(tr("Jump to next difference"));
        connect(nextChangeAction, &QAction::triggered,
            diffView, &DiffView::goToNextChange);

        previousChangeAction = new QAction(tr("&Previous Change"), this);
        previousChangeAction->setShortcut(tr("Shift+F7"));
        previousChangeAction->setStatusTip(tr("Jump to previous difference"));
        connect(previousChangeAction, &QAction::triggered,
            diffView, &DiffView::goToPreviousChange);
    
    aboutAction = new QAction(tr("&About"), this);
    aboutAction->setStatusTip(tr("About DiffyInAJiffy"));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::aboutDialog);
}

void MainWindow::createMenus()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(openFilesAction);
    fileMenu->addAction(recentPairAction);
    fileMenu->addAction(openFoldersAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);
    
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(ignoreWhitespaceAction);
    viewMenu->addAction(ignoreReflowAction);
    viewMenu->addAction(ignorePunctuationAction);
    viewMenu->addSeparator();
    viewMenu->addAction(previousChangeAction);
    viewMenu->addAction(nextChangeAction);
    
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAction);
}

void MainWindow::createToolBar()
{
    QToolBar *toolBar = addToolBar(tr("Main"));
    toolBar->addAction(openFilesAction);
    toolBar->addAction(recentPairAction);
    toolBar->addAction(openFoldersAction);
    toolBar->addSeparator();
    toolBar->addAction(ignoreWhitespaceAction);
    toolBar->addAction(ignoreReflowAction);
    toolBar->addAction(ignorePunctuationAction);
    toolBar->addSeparator();
    toolBar->addAction(previousChangeAction);
    toolBar->addAction(nextChangeAction);
}

void MainWindow::openFiles()
{
    QSettings settings;
    const QString startDirectory = settings.value(LastDirectoryKey).toString();

    const QString originalFile = QFileDialog::getOpenFileName(
        this,
        tr("Select Original File"),
        startDirectory,
        fileDialogFilter()
    );

    if (originalFile.isEmpty()) {
        return;
    }

    const QString modifiedFile = QFileDialog::getOpenFileName(
        this,
        tr("Select Modified File"),
        QFileInfo(originalFile).absolutePath(),
        fileDialogFilter()
    );

    if (modifiedFile.isEmpty()) {
        return;
    }

    compareFiles(originalFile, modifiedFile);
}

void MainWindow::openRecentPair()
{
    QSettings settings;
    const QString originalFile = settings.value(LastOriginalKey).toString();
    const QString modifiedFile = settings.value(LastModifiedKey).toString();

    if (!QFileInfo(originalFile).isFile() || !QFileInfo(modifiedFile).isFile()) {
        QMessageBox::warning(this, tr("Recent Pair"), tr("The last compared file pair is no longer available."));
        updateRecentPairAction();
        return;
    }

    compareFiles(originalFile, modifiedFile);
}

void MainWindow::openFolders()
{
    QString folder1 = QFileDialog::getExistingDirectory(
        this, tr("Select First Folder"));
    
    if (folder1.isEmpty())
        return;
        
    QString folder2 = QFileDialog::getExistingDirectory(
        this, tr("Select Second Folder"));
    
    if (folder2.isEmpty())
        return;
    
    QSettings settings;
    settings.setValue(LastDirectoryKey, QFileInfo(folder1).absolutePath());

    folderView->loadFolders(folder1, folder2);
    statusBar()->showMessage(tr("Comparing folders: %1 and %2").arg(folder1).arg(folder2));
}

void MainWindow::compareFiles(const QString &file1, const QString &file2)
{
    diffView->loadFiles(file1, file2);

    QSettings settings;
    settings.setValue(LastOriginalKey, file1);
    settings.setValue(LastModifiedKey, file2);
    settings.setValue(LastDirectoryKey, QFileInfo(file1).absolutePath());

    updateRecentPairAction();
    statusBar()->showMessage(tr("Comparing: %1 and %2").arg(file1).arg(file2));
}

void MainWindow::updateRecentPairAction()
{
    QSettings settings;
    const QString originalFile = settings.value(LastOriginalKey).toString();
    const QString modifiedFile = settings.value(LastModifiedKey).toString();
    const bool filesExist = QFileInfo(originalFile).isFile() && QFileInfo(modifiedFile).isFile();

    recentPairAction->setEnabled(filesExist);
    if (filesExist) {
        recentPairAction->setStatusTip(tr("Reopen: %1 and %2").arg(originalFile, modifiedFile));
    } else {
        recentPairAction->setStatusTip(tr("Reopen the last compared file pair"));
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (hasExactlyTwoLocalDroppedFiles(event->mimeData())) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QStringList files = localDroppedFiles(event->mimeData());
    if (files.size() != 2) {
        QMessageBox::warning(this, tr("Drop Files"), tr("Drop exactly two files to compare them."));
        return;
    }

    compareFiles(files[0], files[1]);
    event->acceptProposedAction();
}

void MainWindow::toggleIgnoreWhitespace(bool enabled)
{
    diffView->setIgnoreWhitespace(enabled);
    statusBar()->showMessage(enabled ? tr("Ignoring whitespace") : tr("Not ignoring whitespace"), 2000);
}

void MainWindow::toggleIgnoreReflow(bool enabled)
{
    diffView->setIgnoreReflow(enabled);
    statusBar()->showMessage(enabled ? tr("Ignoring reflow") : tr("Not ignoring reflow"), 2000);
}

void MainWindow::toggleIgnorePunctuation(bool enabled)
{
    diffView->setIgnorePunctuation(enabled);
    statusBar()->showMessage(enabled ? tr("Ignoring punctuation") : tr("Not ignoring punctuation"), 2000);
}

void MainWindow::aboutDialog()
{
    QMessageBox::about(this, tr("About DiffyInAJiffy"),
        tr("<h2>DiffyInAJiffy 1.0</h2>"
           "<p>GitHub-style side-by-side diff viewer for:</p>"
           "<ul>"
           "<li>Text files (.txt, .md)</li>"
           "<li>Word documents (.docx)</li>"
           "<li>PDF files (.pdf)</li>"
           "</ul>"
           "<p>Features:</p>"
           "<ul>"
           "<li>Side-by-side comparison</li>"
           "<li>Folder comparison</li>"
           "<li>Ignore whitespace/reflow/punctuation</li>"
           "</ul>"
           "<p>Planned enhancements:</p>"
           "<ul>"
           "<li>PDF overlay mode</li>"
           "<li>Full DOCX structure parsing</li>"
           "</ul>"));
}
