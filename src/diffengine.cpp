#include "diffengine.h"
#include <QStringList>
#include <QRegularExpression>
#include <algorithm>

DiffEngine::DiffEngine(QObject *parent)
    : QObject(parent)
{
}

DiffEngine::~DiffEngine()
{
}

QString DiffEngine::normalizeWhitespace(const QString &text)
{
    QString result = text;
    // Replace multiple spaces with single space
    result.replace(QRegularExpression("[ \\t]+"), " ");
    // Remove trailing whitespace from each line
    result.replace(QRegularExpression("[ \\t]+$", QRegularExpression::MultilineOption), "");
    // Remove leading whitespace from each line
    result.replace(QRegularExpression("^[ \\t]+", QRegularExpression::MultilineOption), "");
    return result;
}

QString DiffEngine::removePunctuation(const QString &text)
{
    QString result = text;
    // Remove common punctuation marks
    result.remove(QRegularExpression("[.,;:!?'\"]"));
    return result;
}

QString DiffEngine::normalizeReflow(const QString &text)
{
    // Join lines in paragraphs (separated by double newlines)
    QStringList paragraphs = text.split(QRegularExpression("\\n\\s*\\n"));
    QStringList normalized;
    
    for (const QString &para : paragraphs) {
        QString joined = para;
        joined.replace(QRegularExpression("\\s*\\n\\s*"), " ");
        joined = joined.trimmed();
        if (!joined.isEmpty()) {
            normalized.append(joined);
        }
    }
    
    return normalized.join("\n\n");
}

QVector<DiffHunk> DiffEngine::computeDiff(const QString &text1, const QString &text2)
{
    // Split into lines for line-based diff
    QStringList lines1 = text1.split('\n');
    QStringList lines2 = text2.split('\n');
    
    // Compute edits using Myers algorithm
    QVector<Edit> edits = myersDiff(lines1, lines2);
    
    // Convert edits to hunks
    return editsToHunks(edits, text1, text2);
}

QVector<DiffLinePair> DiffEngine::computeAlignedLines(const QString &text1, const QString &text2)
{
    const QStringList lines1 = text1.split('\n');
    const QStringList lines2 = text2.split('\n');
    const QVector<Edit> edits = myersDiff(lines1, lines2);

    QVector<DiffLinePair> aligned;

    int i = 0;
    while (i < edits.size()) {
        if (edits[i].type == Edit::Equal) {
            DiffLinePair pair;
            pair.type = DiffHunk::Unchanged;
            pair.leftLine = edits[i].pos1 < lines1.size() ? lines1[edits[i].pos1] : QString();
            pair.rightLine = edits[i].pos2 < lines2.size() ? lines2[edits[i].pos2] : QString();
            aligned.append(pair);
            ++i;
            continue;
        }

        QVector<QString> deletedLines;
        QVector<QString> insertedLines;

        while (i < edits.size() && edits[i].type != Edit::Equal) {
            if (edits[i].type == Edit::Delete) {
                deletedLines.append(edits[i].pos1 < lines1.size() ? lines1[edits[i].pos1] : QString());
            } else if (edits[i].type == Edit::Insert) {
                insertedLines.append(edits[i].pos2 < lines2.size() ? lines2[edits[i].pos2] : QString());
            }
            ++i;
        }

        const int pairedCount = std::min(deletedLines.size(), insertedLines.size());
        for (int p = 0; p < pairedCount; ++p) {
            DiffLinePair pair;
            pair.type = DiffHunk::Modified;
            pair.leftLine = deletedLines[p];
            pair.rightLine = insertedLines[p];
            aligned.append(pair);
        }

        for (int p = pairedCount; p < deletedLines.size(); ++p) {
            DiffLinePair pair;
            pair.type = DiffHunk::Deleted;
            pair.leftLine = deletedLines[p];
            pair.rightLine.clear();
            aligned.append(pair);
        }

        for (int p = pairedCount; p < insertedLines.size(); ++p) {
            DiffLinePair pair;
            pair.type = DiffHunk::Added;
            pair.leftLine.clear();
            pair.rightLine = insertedLines[p];
            aligned.append(pair);
        }
    }

    return aligned;
}

QVector<DiffEngine::Edit> DiffEngine::myersDiff(const QStringList &lines1, const QStringList &lines2)
{
    // Use LCS-based reconstruction to produce a stable line diff.
    QVector<Edit> edits;

    const int n = lines1.size();
    const int m = lines2.size();
    QVector<QVector<int>> lcs(n + 1, QVector<int>(m + 1, 0));

    for (int i = n - 1; i >= 0; --i) {
        for (int j = m - 1; j >= 0; --j) {
            if (lines1[i] == lines2[j]) {
                lcs[i][j] = 1 + lcs[i + 1][j + 1];
            } else {
                lcs[i][j] = std::max(lcs[i + 1][j], lcs[i][j + 1]);
            }
        }
    }

    int i = 0;
    int j = 0;
    while (i < n && j < m) {
        Edit e;
        e.length = 1;
        e.pos1 = i;
        e.pos2 = j;

        if (lines1[i] == lines2[j]) {
            e.type = Edit::Equal;
            edits.append(e);
            ++i;
            ++j;
        } else if (lcs[i + 1][j] >= lcs[i][j + 1]) {
            e.type = Edit::Delete;
            edits.append(e);
            ++i;
        } else {
            e.type = Edit::Insert;
            edits.append(e);
            ++j;
        }
    }

    while (i < n) {
        Edit e;
        e.type = Edit::Delete;
        e.pos1 = i;
        e.pos2 = j;
        e.length = 1;
        edits.append(e);
        ++i;
    }

    while (j < m) {
        Edit e;
        e.type = Edit::Insert;
        e.pos1 = i;
        e.pos2 = j;
        e.length = 1;
        edits.append(e);
        ++j;
    }

    return edits;
}

QVector<DiffHunk> DiffEngine::editsToHunks(const QVector<Edit> &edits, const QString &text1, const QString &text2)
{
    QVector<DiffHunk> hunks;
    QStringList lines1 = text1.split('\n');
    QStringList lines2 = text2.split('\n');
    
    int pos1 = 0, pos2 = 0;
    
    for (int i = 0; i < edits.size(); i++) {
        const Edit &edit = edits[i];
        
        DiffHunk hunk;
        
        if (edit.type == Edit::Delete) {
            // Check if next edit is Insert at same position (modification)
            if (i + 1 < edits.size() && edits[i + 1].type == Edit::Insert &&
                edits[i + 1].pos1 == edit.pos1) {
                hunk.type = DiffHunk::Modified;
                hunk.leftStart = pos1;
                hunk.leftEnd = pos1 + (edit.pos1 < lines1.size() ? lines1[edit.pos1].length() : 0);
                hunk.rightStart = pos2;
                hunk.rightEnd = pos2 + (edits[i+1].pos2 < lines2.size() ? lines2[edits[i+1].pos2].length() : 0);
                
                pos1 += (edit.pos1 < lines1.size() ? lines1[edit.pos1].length() + 1 : 0);
                pos2 += (edits[i+1].pos2 < lines2.size() ? lines2[edits[i+1].pos2].length() + 1 : 0);
                i++; // Skip the next insert
            } else {
                hunk.type = DiffHunk::Deleted;
                hunk.leftStart = pos1;
                hunk.leftEnd = pos1 + (edit.pos1 < lines1.size() ? lines1[edit.pos1].length() : 0);
                hunk.rightStart = pos2;
                hunk.rightEnd = pos2;
                
                pos1 += (edit.pos1 < lines1.size() ? lines1[edit.pos1].length() + 1 : 0);
            }
            hunks.append(hunk);
        } else if (edit.type == Edit::Insert) {
            hunk.type = DiffHunk::Added;
            hunk.leftStart = pos1;
            hunk.leftEnd = pos1;
            hunk.rightStart = pos2;
            hunk.rightEnd = pos2 + (edit.pos2 < lines2.size() ? lines2[edit.pos2].length() : 0);
            
            pos2 += (edit.pos2 < lines2.size() ? lines2[edit.pos2].length() + 1 : 0);
            hunks.append(hunk);
        } else if (edit.type == Edit::Equal) {
            // Move positions forward for equal lines
            pos1 += (edit.pos1 < lines1.size() ? lines1[edit.pos1].length() + 1 : 0);
            pos2 += (edit.pos2 < lines2.size() ? lines2[edit.pos2].length() + 1 : 0);
        }
    }
    
    return hunks;
}
