# Contributing to DiffyInAJiffy

Thank you for your interest in contributing to DiffyInAJiffy!

## Getting Started

1. Fork the repository
2. Clone your fork
3. Create a feature branch: `git checkout -b feature/my-feature`
4. Make your changes
5. Test your changes
6. Commit with clear messages
7. Push to your fork
8. Open a Pull Request

## Development Setup

### Prerequisites

- Qt 6 development libraries
- CMake 3.16+
- C++17 compatible compiler
- Poppler-Qt6 (optional, for PDF support)

### Building

```bash
./build.sh
```

Or manually:

```bash
mkdir build && cd build
cmake ..
make
```

## Code Style

- Follow Qt coding conventions
- Use meaningful variable names
- Comment complex algorithms
- Keep functions focused and small
- Use const correctness

### Naming Conventions

- Classes: `PascalCase` (e.g., `DiffEngine`)
- Methods: `camelCase` (e.g., `computeDiff`)
- Member variables: `camelCase` (e.g., `ignoreWhitespace`)
- Constants: `UPPER_CASE` or `kPascalCase`

### Example

```cpp
class DiffEngine : public QObject
{
    Q_OBJECT

public:
    explicit DiffEngine(QObject *parent = nullptr);
    
    QVector<DiffHunk> computeDiff(const QString &text1, const QString &text2);
    
private:
    bool ignoreWhitespace;
    static const int kMaxLineLength = 10000;
};
```

## Testing

### Manual Testing

1. Test file comparison:
   - Open two text files
   - Verify highlighting is correct
   - Test scroll synchronization

2. Test folder comparison:
   - Compare two directories
   - Check file tree accuracy
   - Verify status colors

3. Test diff options:
   - Toggle ignore whitespace
   - Toggle ignore reflow
   - Toggle ignore punctuation
   - Verify results change appropriately

### Adding Tests

Future work: We plan to add Qt Test framework integration.

## Adding New Features

### New File Format Support

1. Add parsing in `DocumentParser`
2. Add display logic in `DiffView`
3. Update file filters in `MainWindow`
4. Document in README.md

### New Diff Options

1. Add normalization function in `DiffEngine`
2. Add UI toggle in `MainWindow`
3. Connect to `DiffView` option setters
4. Test with various files

## Documentation

- Update README.md for user-facing changes
- Update DESIGN.md for architectural changes
- Add inline comments for complex code
- Update help dialog in `MainWindow`

## Commit Messages

Use clear, descriptive commit messages:

```
Add PDF overlay rendering mode

- Implement page-by-page image rendering
- Add overlay diff highlighting
- Update UI to show PDF pages
- Add zoom controls
```

## Pull Request Process

1. Ensure code compiles without warnings
2. Test on at least one Linux distribution
3. Update documentation
4. Describe changes in PR description
5. Link related issues

## Areas for Contribution

### High Priority

- [ ] Full Myers diff algorithm implementation
- [ ] QuaZip integration for complete DOCX support
- [ ] PDF overlay rendering mode
- [ ] Unit test framework setup

### Medium Priority

- [ ] Syntax highlighting for code files
- [ ] Export to HTML/PDF
- [ ] Command-line interface
- [ ] Configuration file support
- [ ] Performance optimization for large files

### Nice to Have

- [ ] Dark mode support
- [ ] Custom color schemes
- [ ] Plugin system
- [ ] Git integration
- [ ] Merge conflict resolution

## Questions?

- Open an issue for questions
- Check existing issues and PRs
- Review DESIGN.md for architecture

## License

By contributing, you agree that your contributions will be licensed under the same license as the project (MIT).
