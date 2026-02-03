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

QVector<DiffEngine::Edit> DiffEngine::myersDiff(const QStringList &lines1, const QStringList &lines2)
{
    // Simplified Myers diff algorithm
    // This is a basic implementation for demonstration
    QVector<Edit> edits;
    
    int i = 0, j = 0;
    while (i < lines1.size() || j < lines2.size()) {
        if (i < lines1.size() && j < lines2.size() && lines1[i] == lines2[j]) {
            // Equal lines
            Edit e;
            e.type = Edit::Equal;
            e.pos1 = i;
            e.pos2 = j;
            e.length = 1;
            edits.append(e);
            i++;
            j++;
        } else if (i < lines1.size() && (j >= lines2.size() || 
                   (j + 1 < lines2.size() && lines1[i] == lines2[j + 1]))) {
            // Insert in text2
            Edit e;
            e.type = Edit::Insert;
            e.pos1 = i;
            e.pos2 = j;
            e.length = 1;
            edits.append(e);
            j++;
        } else if (j < lines2.size() && (i >= lines1.size() || 
                   (i + 1 < lines1.size() && lines1[i + 1] == lines2[j]))) {
            // Delete from text1
            Edit e;
            e.type = Edit::Delete;
            e.pos1 = i;
            e.pos2 = j;
            e.length = 1;
            edits.append(e);
            i++;
        } else if (i < lines1.size() && j < lines2.size()) {
            // Modified line
            Edit del, ins;
            del.type = Edit::Delete;
            del.pos1 = i;
            del.pos2 = j;
            del.length = 1;
            
            ins.type = Edit::Insert;
            ins.pos1 = i;
            ins.pos2 = j;
            ins.length = 1;
            
            edits.append(del);
            edits.append(ins);
            i++;
            j++;
        }
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
