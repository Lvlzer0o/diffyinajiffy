#ifndef DIFFENGINE_H
#define DIFFENGINE_H

#include <QObject>
#include <QString>
#include <QVector>

struct DiffHunk {
    enum Type {
        Unchanged,
        Added,
        Deleted,
        Modified
    };
    
    Type type;
    int leftStart;
    int leftEnd;
    int rightStart;
    int rightEnd;
    
    DiffHunk() : type(Unchanged), leftStart(0), leftEnd(0), rightStart(0), rightEnd(0) {}
};

class DiffEngine : public QObject
{
    Q_OBJECT

public:
    explicit DiffEngine(QObject *parent = nullptr);
    ~DiffEngine();

    // Compute differences between two texts
    QVector<DiffHunk> computeDiff(const QString &text1, const QString &text2);
    
    // Text normalization utilities
    QString normalizeWhitespace(const QString &text);
    QString removePunctuation(const QString &text);
    QString normalizeReflow(const QString &text);

private:
    // Myers diff algorithm implementation
    struct Edit {
        enum Type { Insert, Delete, Equal };
        Type type;
        int pos1;
        int pos2;
        int length;
    };
    
    QVector<Edit> myersDiff(const QStringList &lines1, const QStringList &lines2);
    QVector<DiffHunk> editsToHunks(const QVector<Edit> &edits, const QString &text1, const QString &text2);
};

#endif // DIFFENGINE_H
