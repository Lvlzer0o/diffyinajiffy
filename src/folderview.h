#ifndef FOLDERVIEW_H
#define FOLDERVIEW_H

#include <QWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QDir>

class FolderView : public QWidget
{
    Q_OBJECT

public:
    explicit FolderView(QWidget *parent = nullptr);
    ~FolderView();

    void loadFolders(const QString &folder1, const QString &folder2);

signals:
    void fileSelected(const QString &file1, const QString &file2);

private slots:
    void onItemClicked(QTreeWidgetItem *item, int column);

private:
    void setupUI();
    void compareDirectories(const QString &path1, const QString &path2, QTreeWidgetItem *parentItem);
    void addFileItem(const QString &name, const QString &path1, const QString &path2, 
                     const QString &status, QTreeWidgetItem *parent);
    
    QTreeWidget *treeWidget;
    QString baseFolder1;
    QString baseFolder2;
};

#endif // FOLDERVIEW_H
