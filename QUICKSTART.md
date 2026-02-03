# Quick Start Guide

## Installation

### From Source

1. Install dependencies:
   ```bash
   # Ubuntu/Debian
   sudo apt-get install build-essential cmake qt6-base-dev libpoppler-qt6-dev pkg-config
   
   # Fedora
   sudo dnf install cmake qt6-qtbase-devel poppler-qt6-devel gcc-c++ pkg-config
   
   # Arch Linux
   sudo pacman -S cmake qt6-base poppler-qt6 gcc pkg-config
   ```

2. Build and install:
   ```bash
   ./build.sh
   cd build
   sudo make install
   ```

## Basic Usage

### Comparing Two Files

1. Launch the application:
   ```bash
   diffyinajiffy
   ```

2. Select files to compare:
   - Click **File → Open Files...** (or press `Ctrl+O`)
   - Select exactly two files
   - Click **Open**

3. View the differences:
   - Left pane shows the original file
   - Right pane shows the modified file
   - Changes are highlighted in color:
     - 🟢 Green = Added
     - 🔴 Red = Deleted
     - 🟡 Yellow = Modified

### Comparing Two Folders

1. Select folders:
   - Click **File → Open Folders...** (or press `Ctrl+Shift+O`)
   - Select the first folder
   - Select the second folder

2. Browse the file tree:
   - Left panel shows file tree with status indicators
   - Click on any file to view its diff

3. File status indicators:
   - 🟢 Added = File exists only in second folder
   - 🔴 Deleted = File exists only in first folder
   - 🟠 Modified = File exists in both but differs
   - ⚪ Identical = Files are the same

### Using Diff Options

Enable options from the **View** menu or toolbar:

- **Ignore Whitespace**: Compares files ignoring spaces and tabs
  - Useful for code with different indentation
  
- **Ignore Reflow**: Treats reflowed paragraphs as same
  - Useful for comparing wrapped text documents
  
- **Ignore Punctuation**: Ignores punctuation-only changes
  - Useful for comparing edited prose

## Supported File Formats

| Format | Extension | Features |
|--------|-----------|----------|
| Text | `.txt` | Line-by-line comparison |
| Markdown | `.md` | Line-by-line comparison |
| Word | `.docx` | Structure-aware parsing* |
| PDF | `.pdf` | Text extraction and comparison* |

*Requires Poppler-Qt6 for PDF support  
*DOCX support is basic; full support planned

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+O` | Open files |
| `Ctrl+Shift+O` | Open folders |
| `Ctrl+Q` | Quit application |

## Tips and Tricks

### Synchronized Scrolling

- Both panes scroll together automatically
- Useful for comparing long files
- Works for both vertical and horizontal scrolling

### Finding Differences

- Use scrollbar to navigate between changes
- Colored highlights make differences stand out
- Modified lines show what changed

### Best Practices

1. **Use appropriate file types**: Compare text with text, PDF with PDF
2. **Enable relevant options**: Use whitespace ignore for code
3. **Folder comparison**: Great for reviewing changes across projects
4. **Check status colors**: Quickly identify file changes in folder view

## Troubleshooting

### "PDF support not available"

- Install Poppler-Qt6:
  ```bash
  sudo apt-get install libpoppler-qt6-dev  # Ubuntu/Debian
  ```
- Rebuild the application

### "DOCX requires QuaZip library"

- DOCX support is currently basic
- Full support will be added in future release
- You can still compare DOCX files but structure may not be perfect

### Application won't start

- Check Qt6 is installed:
  ```bash
  pkg-config --modversion Qt6Core
  ```
- Ensure all dependencies are present

### Files show no differences but should

- Check if ignore options are enabled
- Try disabling ignore whitespace/reflow/punctuation
- Verify you're comparing the correct files

## Examples

Try the included example files:

```bash
# Compare text files
diffyinajiffy examples/text/version1.txt examples/text/version2.txt

# Compare markdown files
diffyinajiffy examples/markdown/doc1.md examples/markdown/doc2.md
```

## Getting Help

- Read the full README.md for detailed information
- Check DESIGN.md for architecture details
- Report issues on GitHub
- Contribute improvements (see CONTRIBUTING.md)

## Next Steps

- Explore folder comparison features
- Try different file formats
- Experiment with diff options
- Customize for your workflow

Happy diffing! 🎉
