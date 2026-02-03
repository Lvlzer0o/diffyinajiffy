#include "mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileInfo>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
    createActions();
    createMenus();
    createToolBar();
    
    setWindowTitle("DiffyInAJiffy - Side-by-Side Diff Viewer");
    resize(1200, 800);
    setAcceptDrops(true);
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

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (!event->mimeData()->hasUrls()) {
        return;
    }

    int localCount = 0;
    const auto urls = event->mimeData()->urls();
    for (const auto &url : urls) {
        if (url.isLocalFile()) {
            ++localCount;
        }
    }

    if (localCount >= 2) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    if (!event->mimeData()->hasUrls()) {
        return;
    }

    QStringList paths;
    const auto urls = event->mimeData()->urls();
    for (const auto &url : urls) {
        if (url.isLocalFile()) {
            paths.append(url.toLocalFile());
        }
    }

    if (paths.size() < 2) {
        QMessageBox::warning(this, tr("Drag and Drop"),
            tr("Please drop exactly two files or two folders."));
        return;
    }

    if (paths.size() > 2) {
        QMessageBox::information(this, tr("Drag and Drop"),
            tr("More than two items were dropped. Using the first two."));
    }

    const QString path1 = paths[0];
    const QString path2 = paths[1];
    QFileInfo info1(path1);
    QFileInfo info2(path2);

    if (info1.isDir() && info2.isDir()) {
        folderView->loadFolders(path1, path2);
        statusBar()->showMessage(tr("Comparing folders: %1 and %2").arg(path1).arg(path2));
        event->acceptProposedAction();
        return;
    }

    if (info1.isFile() && info2.isFile()) {
        diffView->loadFiles(path1, path2);
        statusBar()->showMessage(tr("Comparing: %1 and %2").arg(path1).arg(path2));
        event->acceptProposedAction();
        return;
    }

    QMessageBox::warning(this, tr("Drag and Drop"),
        tr("Please drop two files or two folders (not a mix)."));
}

void MainWindow::createActions()
{
    openFilesAction = new QAction(tr("&Open Files..."), this);
    openFilesAction->setShortcut(QKeySequence::Open);
    openFilesAction->setStatusTip(tr("Open two files to compare"));
    connect(openFilesAction, &QAction::triggered, this, &MainWindow::openFiles);
    
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
    
    aboutAction = new QAction(tr("&About"), this);
    aboutAction->setStatusTip(tr("About DiffyInAJiffy"));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::aboutDialog);
}

void MainWindow::createMenus()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(openFilesAction);
    fileMenu->addAction(openFoldersAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);
    
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(ignoreWhitespaceAction);
    viewMenu->addAction(ignoreReflowAction);
    viewMenu->addAction(ignorePunctuationAction);
    
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAction);
}

void MainWindow::createToolBar()
{
    QToolBar *toolBar = addToolBar(tr("Main"));
    toolBar->addAction(openFilesAction);
    toolBar->addAction(openFoldersAction);
    toolBar->addSeparator();
    toolBar->addAction(ignoreWhitespaceAction);
    toolBar->addAction(ignoreReflowAction);
    toolBar->addAction(ignorePunctuationAction);
}

void MainWindow::openFiles()
{
    QStringList files = QFileDialog::getOpenFileNames(
        this,
        tr("Select Two Files to Compare"),
        QString(),
        tr("All Supported Files (*.txt *.md *.docx *.pdf);;Text Files (*.txt);;Markdown Files (*.md);;Word Documents (*.docx);;PDF Files (*.pdf);;All Files (*)")
    );
    
    if (files.size() == 2) {
        diffView->loadFiles(files[0], files[1]);
        statusBar()->showMessage(tr("Comparing: %1 and %2").arg(files[0]).arg(files[1]));
    } else if (files.size() > 0) {
        QMessageBox::warning(this, tr("File Selection"), 
            tr("Please select exactly two files to compare."));
    }
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
    
    folderView->loadFolders(folder1, folder2);
    statusBar()->showMessage(tr("Comparing folders: %1 and %2").arg(folder1).arg(folder2));
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
