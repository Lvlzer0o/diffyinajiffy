#include "folderview.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QFileInfo>
#include <QDirIterator>
#include <QSet>

FolderView::FolderView(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

FolderView::~FolderView()
{
}

void FolderView::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    
    QLabel *label = new QLabel(tr("File Tree"));
    label->setStyleSheet("font-weight: bold; padding: 5px; background-color: #f0f0f0;");
    
    treeWidget = new QTreeWidget();
    treeWidget->setHeaderLabels(QStringList() << tr("File") << tr("Status"));
    treeWidget->setColumnWidth(0, 200);
    
    connect(treeWidget, &QTreeWidget::itemClicked, 
            this, &FolderView::onItemClicked);
    
    layout->addWidget(label);
    layout->addWidget(treeWidget);
    layout->setContentsMargins(0, 0, 0, 0);
}

void FolderView::loadFolders(const QString &folder1, const QString &folder2)
{
    baseFolder1 = folder1;
    baseFolder2 = folder2;
    
    treeWidget->clear();
    
    // Create root item
    QTreeWidgetItem *rootItem = new QTreeWidgetItem(treeWidget);
    rootItem->setText(0, tr("Comparing Folders"));
    rootItem->setExpanded(true);
    
    // Compare directories
    compareDirectories(folder1, folder2, rootItem);
}

void FolderView::compareDirectories(const QString &path1, const QString &path2, QTreeWidgetItem *parentItem)
{
    QDir dir1(path1);
    QDir dir2(path2);
    
    // Get all files and subdirectories
    QSet<QString> allNames;
    
    QFileInfoList list1 = dir1.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    QFileInfoList list2 = dir2.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    
    for (const QFileInfo &info : list1) {
        allNames.insert(info.fileName());
    }
    for (const QFileInfo &info : list2) {
        allNames.insert(info.fileName());
    }
    
    // Compare each item
    QList<QString> sortedNames = allNames.values();
    std::sort(sortedNames.begin(), sortedNames.end());
    
    for (const QString &name : sortedNames) {
        QString fullPath1 = path1 + "/" + name;
        QString fullPath2 = path2 + "/" + name;
        
        QFileInfo info1(fullPath1);
        QFileInfo info2(fullPath2);
        
        bool exists1 = info1.exists();
        bool exists2 = info2.exists();
        
        QString status;
        if (exists1 && exists2) {
            if (info1.isDir() && info2.isDir()) {
                status = "Directory";
                QTreeWidgetItem *dirItem = new QTreeWidgetItem(parentItem);
                dirItem->setText(0, name);
                dirItem->setText(1, status);
                dirItem->setData(0, Qt::UserRole, fullPath1);
                dirItem->setData(1, Qt::UserRole, fullPath2);
                
                // Recursively compare subdirectories
                compareDirectories(fullPath1, fullPath2, dirItem);
            } else if (info1.isFile() && info2.isFile()) {
                // Compare file contents
                QFile f1(fullPath1), f2(fullPath2);
                bool identical = false;
                
                if (f1.size() == f2.size()) {
                    if (f1.open(QIODevice::ReadOnly) && f2.open(QIODevice::ReadOnly)) {
                        identical = (f1.readAll() == f2.readAll());
                        f1.close();
                        f2.close();
                    }
                }
                
                status = identical ? "Identical" : "Modified";
                addFileItem(name, fullPath1, fullPath2, status, parentItem);
            } else {
                status = "Type mismatch";
                addFileItem(name, fullPath1, fullPath2, status, parentItem);
            }
        } else if (exists1) {
            status = "Deleted";
            addFileItem(name, fullPath1, "", status, parentItem);
        } else if (exists2) {
            status = "Added";
            addFileItem(name, "", fullPath2, status, parentItem);
        }
    }
}

void FolderView::addFileItem(const QString &name, const QString &path1, const QString &path2, 
                             const QString &status, QTreeWidgetItem *parent)
{
    QTreeWidgetItem *item = new QTreeWidgetItem(parent);
    item->setText(0, name);
    item->setText(1, status);
    item->setData(0, Qt::UserRole, path1);
    item->setData(1, Qt::UserRole, path2);
    
    // Color code based on status
    if (status == "Modified") {
        item->setForeground(1, QColor(255, 140, 0)); // Orange
    } else if (status == "Added") {
        item->setForeground(1, QColor(0, 128, 0)); // Green
    } else if (status == "Deleted") {
        item->setForeground(1, QColor(255, 0, 0)); // Red
    } else if (status == "Identical") {
        item->setForeground(1, QColor(128, 128, 128)); // Gray
    }
}

void FolderView::onItemClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    
    QString path1 = item->data(0, Qt::UserRole).toString();
    QString path2 = item->data(1, Qt::UserRole).toString();
    
    // Only emit signal for files, not directories
    QFileInfo info1(path1);
    QFileInfo info2(path2);
    
    if (info1.isFile() || info2.isFile()) {
        // If one is missing, use the other one for both (will show as empty)
        if (path1.isEmpty() && !path2.isEmpty()) {
            // File only in folder2
            emit fileSelected(path2, path2); // Show same file twice with note
        } else if (path2.isEmpty() && !path1.isEmpty()) {
            // File only in folder1
            emit fileSelected(path1, path1);
        } else {
            emit fileSelected(path1, path2);
        }
    }
}
