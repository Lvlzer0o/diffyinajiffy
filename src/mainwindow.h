#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSplitter>
#include <QToolBar>
#include <QMenuBar>
#include <QStatusBar>
#include "diffview.h"
#include "folderview.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void openFiles();
    void openFolders();
    void toggleIgnoreWhitespace(bool enabled);
    void toggleIgnoreReflow(bool enabled);
    void toggleIgnorePunctuation(bool enabled);
    void aboutDialog();

private:
    void createActions();
    void createMenus();
    void createToolBar();
    void setupUI();
    
    // UI Components
    QSplitter *mainSplitter;
    FolderView *folderView;
    DiffView *diffView;
    
    // Actions
    QAction *openFilesAction;
    QAction *openFoldersAction;
    QAction *exitAction;
    QAction *ignoreWhitespaceAction;
    QAction *ignoreReflowAction;
    QAction *ignorePunctuationAction;
    QAction *aboutAction;
};

#endif // MAINWINDOW_H
