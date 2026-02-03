# DiffyInAJiffy

A GitHub-style side-by-side diff viewer for Linux with Qt 6, supporting multiple file formats offline.

## Features

- **Side-by-side comparison** with synchronized scrolling
- **Multiple file format support**:
  - Text files (.txt)
  - Markdown files (.md)
  - Word documents (.docx) with rich structure parsing
  - PDF files (.pdf) with page rendering
- **Folder comparison** with file tree view (PR-like interface)
- **Diff options**:
  - Ignore whitespace
  - Ignore reflow
  - Ignore punctuation-only changes
- **Syntax highlighting** for additions, deletions, and modifications
- **Linux-native** feel with Qt 6 widgets

## Requirements

- Qt 6 (Core, Gui, Widgets)
- CMake 3.16 or higher
- C++17 compiler
- Poppler-Qt6 (for PDF support)
- pkg-config

### Ubuntu/Debian

```bash
sudo apt-get install build-essential cmake qt6-base-dev libpoppler-qt6-dev pkg-config
```

### Fedora

```bash
sudo dnf install cmake qt6-qtbase-devel poppler-qt6-devel gcc-c++ pkg-config
```

### Arch Linux

```bash
sudo pacman -S cmake qt6-base poppler-qt6 gcc pkg-config
```

## Building

```bash
mkdir build
cd build
cmake ..
make
```

## Installation

```bash
sudo make install
```

This will install the `diffyinajiffy` binary to `/usr/local/bin`.

## Usage

### File Comparison

1. Launch the application: `diffyinajiffy`
2. Click "Open Files..." or press Ctrl+O
3. Select two files to compare
4. View side-by-side differences

### Folder Comparison

1. Click "Open Folders..." or press Ctrl+Shift+O
2. Select two folders to compare
3. Browse the file tree to see changes
4. Click on any file to view its diff

### Diff Options

Use the View menu or toolbar to toggle:
- **Ignore Whitespace**: Normalizes spaces and tabs
- **Ignore Reflow**: Joins paragraph lines
- **Ignore Punctuation**: Removes punctuation marks for comparison

## Architecture

### Components

- **MainWindow**: Main application window with menus and toolbar
- **DiffView**: Side-by-side diff display with synchronized scrolling
- **DiffEngine**: Computes differences using Myers algorithm
- **DocumentParser**: Extracts text from DOCX and PDF files
- **FolderView**: File tree for folder comparison

### Diff Algorithm

Uses a simplified Myers diff algorithm for line-based comparison with:
- Addition detection (green highlighting)
- Deletion detection (red highlighting)
- Modification detection (yellow highlighting)

### Document Parsing

- **PDF**: Uses Poppler-Qt6 to extract text page-by-page
- **DOCX**: Parses XML structure to preserve headings, lists, and tables
  - Note: Full DOCX support requires QuaZip library (future enhancement)

## Limitations

- DOCX parsing is currently basic (full support requires QuaZip)
- PDF overlay mode is planned for future release
- Diff algorithm is simplified (not full Myers implementation)

## Future Enhancements

- [ ] Full Myers diff algorithm implementation
- [ ] QuaZip integration for complete DOCX support
- [ ] PDF overlay rendering mode
- [ ] Syntax highlighting for code files
- [ ] Export diff as HTML/PDF
- [ ] Command-line interface
- [ ] Configuration file support

## License

MIT License (or your chosen license)

## Contributing

Contributions welcome! Please submit issues and pull requests.