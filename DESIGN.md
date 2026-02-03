# DiffyInAJiffy - Design Document

## Architecture Overview

DiffyInAJiffy is designed as a modern Qt6-based application following the Model-View architecture pattern with clear separation of concerns.

## Component Diagram

```
┌─────────────────────────────────────────────────────┐
│                   MainWindow                         │
│  - Menu Bar (File, View, Help)                      │
│  - Toolbar (Quick actions)                          │
│  - Status Bar                                        │
└────────────┬────────────────────────────────────────┘
             │
      ┌──────┴──────┐
      │             │
┌─────▼─────┐  ┌───▼──────┐
│FolderView │  │ DiffView │
│           │  │          │
│ File Tree │  │ Left|Right│
│ PR-like   │  │ Pane|Pane│
└───────────┘  └─────┬────┘
                     │
            ┌────────┴────────┐
            │                 │
      ┌─────▼──────┐  ┌──────▼───────┐
      │DiffEngine  │  │DocParser     │
      │            │  │              │
      │Myers Algo  │  │PDF/DOCX Parse│
      └────────────┘  └──────────────┘
```

## Core Components

### 1. MainWindow

**Purpose**: Main application container and coordinator

**Responsibilities**:
- Create and manage UI layout
- Handle menu and toolbar actions
- Coordinate between FolderView and DiffView
- Manage application settings (ignore whitespace, etc.)

**Key Methods**:
- `openFiles()`: File selection dialog
- `openFolders()`: Folder comparison dialog
- `toggleIgnoreWhitespace()`: Toggle diff option
- `toggleIgnoreReflow()`: Toggle reflow normalization
- `toggleIgnorePunctuation()`: Toggle punctuation filtering

### 2. DiffView

**Purpose**: Side-by-side diff display

**Responsibilities**:
- Display two panes with synchronized scrolling
- Highlight differences (additions, deletions, modifications)
- Handle different file types (txt, md, docx, pdf)
- Apply diff options (whitespace, reflow, punctuation)

**Key Features**:
- Synchronized vertical and horizontal scrolling
- Color-coded highlighting:
  - Green: Added lines
  - Red: Deleted lines
  - Yellow: Modified lines

**Key Methods**:
- `loadFiles(file1, file2)`: Load and compare files
- `displayTextDiff()`: Show text file comparison
- `displayPdfDiff()`: Show PDF comparison
- `displayDocxDiff()`: Show DOCX comparison
- `highlightDifferences()`: Apply color highlighting

### 3. DiffEngine

**Purpose**: Compute differences between texts

**Algorithm**: Simplified Myers diff algorithm

**Responsibilities**:
- Line-based difference computation
- Text normalization (whitespace, punctuation, reflow)
- Generate diff hunks with position information

**Data Structure - DiffHunk**:
```cpp
struct DiffHunk {
    Type type;        // Added, Deleted, Modified, Unchanged
    int leftStart;    // Start position in left text
    int leftEnd;      // End position in left text
    int rightStart;   // Start position in right text
    int rightEnd;     // End position in right text
};
```

**Key Methods**:
- `computeDiff(text1, text2)`: Main diff computation
- `normalizeWhitespace(text)`: Remove/normalize spaces
- `removePunctuation(text)`: Strip punctuation
- `normalizeReflow(text)`: Join reflowed paragraphs

### 4. DocumentParser

**Purpose**: Extract text from various document formats

**Supported Formats**:
- **PDF**: Uses Poppler-Qt6 for page-by-page text extraction
- **DOCX**: Parses XML structure to preserve document hierarchy
  - Headings (with levels)
  - Lists (with nesting)
  - Tables
  - Paragraphs

**Data Structure - DocumentStructure**:
```cpp
struct DocumentElement {
    Type type;      // Heading, Paragraph, ListItem, TableCell
    int level;      // For headings and nested lists
    QString content;
};

struct DocumentStructure {
    QVector<DocumentElement> elements;
};
```

**Key Methods**:
- `parsePdf(filePath)`: Extract text from PDF
- `parseDocx(filePath)`: Parse DOCX with structure
- `formatStructure(structure)`: Convert to formatted text

### 5. FolderView

**Purpose**: File tree for folder comparison (PR-like view)

**Responsibilities**:
- Recursive directory comparison
- File tree visualization
- Status indication (Added, Deleted, Modified, Identical)
- File selection for detailed diff

**Features**:
- Color-coded status:
  - Green: Added files
  - Red: Deleted files
  - Orange: Modified files
  - Gray: Identical files
- Recursive directory traversal
- Binary file detection

**Key Methods**:
- `loadFolders(folder1, folder2)`: Start comparison
- `compareDirectories()`: Recursive comparison
- Signal: `fileSelected(file1, file2)`: User clicks file

## Diff Algorithm Details

### Simplified Myers Algorithm

The implementation uses a simplified version of Eugene W. Myers' diff algorithm:

1. **Line-based comparison**: Split texts into lines
2. **Sequential scan**: Compare lines sequentially
3. **Edit detection**:
   - **Equal**: Lines match exactly
   - **Insert**: Line exists only in text2
   - **Delete**: Line exists only in text1
   - **Modify**: Consecutive delete+insert at same position

4. **Hunk generation**: Convert edits to position-based hunks

### Normalization Pipeline

Before diff computation, texts can be normalized:

1. **Whitespace normalization**:
   - Collapse multiple spaces to single space
   - Remove leading/trailing whitespace per line

2. **Punctuation removal**:
   - Strip common punctuation: `.,;:!?'"`
   - Preserves word structure

3. **Reflow normalization**:
   - Join lines within paragraphs
   - Preserve paragraph breaks (double newlines)
   - Useful for comparing reflowed documents

## File Format Support

### Text Files (.txt, .md)

- **Method**: Direct file reading
- **Encoding**: UTF-8 assumed
- **Diff**: Line-based

### PDF Files (.pdf)

- **Library**: Poppler-Qt6
- **Method**: Page-by-page text extraction
- **Output**: Text with page markers
- **Limitation**: No overlay rendering yet

### DOCX Files (.docx)

- **Format**: ZIP archive with XML
- **Current**: Basic text extraction
- **Planned**: Full QuaZip integration for:
  - Table structure preservation
  - List hierarchy
  - Heading levels
  - Inline formatting

## UI/UX Design

### Layout

```
┌─────────────────────────────────────────────────┐
│ File | View | Help                              │ Menu Bar
├─────────────────────────────────────────────────┤
│ [Open] [Folders] | [WS] [Reflow] [Punct]        │ Toolbar
├──────────┬──────────────────────────────────────┤
│ File Tree│ Original          │ Modified         │
│          │                   │                  │
│ ├─file1  │ Line 1            │ Line 1           │
│ ├─file2  │ Line 2 deleted    │ Line 2 modified  │
│ └─dir/   │ Line 3            │ Line 3           │
│   ├─file3│ Line 4            │ Line 4 added     │
│          │                   │                  │
│  20% ──  │─────────────── 80% ─────────────────┤
├──────────┴──────────────────────────────────────┤
│ Status: Comparing file1.txt and file2.txt       │ Status Bar
└─────────────────────────────────────────────────┘
```

### Color Scheme

- **Background**: White (`#ffffff`)
- **Headers**: Light gray (`#f0f0f0`)
- **Added**: Light green (`#c8ffc8` / RGB 200,255,200)
- **Deleted**: Light red (`#ffc8c8` / RGB 255,200,200)
- **Modified**: Light yellow (`#ffffc8` / RGB 255,255,200)
- **Status colors**:
  - Green: Added
  - Red: Deleted
  - Orange: Modified
  - Gray: Identical

## Build System

### CMake Configuration

- **Minimum version**: 3.16
- **C++ standard**: C++17
- **Qt components**: Core, Gui, Widgets
- **Optional**: Poppler-Qt6 (for PDF)

### Dependencies

**Required**:
- Qt6 (Core, Gui, Widgets)
- C++17 compiler
- CMake 3.16+

**Optional**:
- Poppler-Qt6 (PDF support)
- QuaZip (Full DOCX support - future)

## Future Enhancements

### Planned Features

1. **Full Myers Algorithm**
   - More accurate diff computation
   - Better handling of moved blocks

2. **PDF Overlay Mode**
   - Render PDF pages as images
   - Overlay diff highlighting
   - Side-by-side page view

3. **Advanced DOCX Support**
   - QuaZip integration
   - Table diff visualization
   - Preserve formatting hints

4. **Syntax Highlighting**
   - Language detection
   - Code-specific diff
   - Semantic comparison

5. **Export Options**
   - HTML diff output
   - PDF report generation
   - Patch file generation

6. **Command Line Interface**
   - Batch comparison
   - Scriptable operation
   - CI/CD integration

## Performance Considerations

- **Large files**: Line-based diff is O(n*m) worst case
  - Consider chunking for very large files
  - Add progress indicators

- **Directory comparison**: Recursive can be slow
  - Use threading for independent comparisons
  - Add cancel operation

- **PDF rendering**: Memory intensive
  - Lazy page loading
  - Cache management

## Security Considerations

- **File validation**: Check file types before parsing
- **Path traversal**: Sanitize folder comparison paths
- **Memory limits**: Prevent DOS with huge files
- **XML parsing**: Use safe XML parser settings

## Testing Strategy

### Unit Tests
- DiffEngine algorithms
- Text normalization functions
- Document parser components

### Integration Tests
- File loading and display
- Folder comparison
- UI interactions

### Manual Tests
- Different file formats
- Edge cases (empty files, binary files)
- Large files and directories
- UI responsiveness

## Contributing Guidelines

- Follow Qt coding conventions
- Use MOC where appropriate
- Document public APIs
- Add unit tests for new features
- Test on Linux distributions

## References

- [Myers Diff Algorithm](http://www.xmailserver.org/diff2.pdf)
- [Qt6 Documentation](https://doc.qt.io/qt-6/)
- [Poppler Documentation](https://poppler.freedesktop.org/)
- [DOCX Format Specification](https://www.ecma-international.org/publications-and-standards/standards/ecma-376/)
