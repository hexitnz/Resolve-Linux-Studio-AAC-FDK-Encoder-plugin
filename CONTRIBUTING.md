# Contributing to DaVinci Resolve FDK-AAC Plugin

Thank you for your interest in contributing! This document provides guidelines for contributing to this project.

## Ways to Contribute

- **Bug Reports:** Report issues you encounter
- **Feature Requests:** Suggest new features or improvements
- **Code Contributions:** Submit bug fixes or new features
- **Documentation:** Improve or translate documentation
- **Testing:** Test on different Linux distributions

## Getting Started

### Fork and Clone

1. Fork the repository on GitHub
2. Clone your fork:
   ```bash
   git clone https://github.com/YOUR_USERNAME/Resolve-Linux-Studio-AAC-FDK-Encoder-plugin.git
   cd Resolve-Linux-Studio-AAC-FDK-Encoder-plugin
   ```

3. Add upstream remote:
   ```bash
   git remote add upstream https://github.com/hexitnz/Resolve-Linux-Studio-AAC-FDK-Encoder-plugin.git
   ```

### Development Setup

1. Install dependencies (see [BUILDING.md](docs/BUILDING.md))
2. Build the project:
   ```bash
   mkdir build && cd build
   cmake -DCMAKE_BUILD_TYPE=Debug ..
   make -j$(nproc)
   ```

3. Test your changes:
   ```bash
   sudo cp -r aac_fdk_plugin.dvcp.bundle /opt/resolve/IOPlugins/
   # Test in DaVinci Resolve
   ```

## Reporting Bugs

### Before Submitting

- Check existing issues to avoid duplicates
- Test with the latest version
- Verify it's not a configuration issue (see [TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md))

### Bug Report Template

Include:

- **Description:** Clear description of the bug
- **Steps to Reproduce:** Numbered steps
- **Environment:** Linux distro, Resolve version, plugin version
- **Logs:** Relevant error messages
- **Additional Context:** Screenshots, files, etc.

## Code Contributions

### Coding Standards

- **Language:** C++11 or later
- **Style:** 4 spaces for indentation, descriptive names, comments for complex logic

### Commit Messages

Use clear, descriptive commit messages:
- Format: `[type]: Brief description`
- Types: `fix`, `feat`, `docs`, `refactor`, `test`, `chore`

Examples:
```
fix: Correct PTS calculation for variable frame rates
feat: Add support for 5.1 surround sound encoding
docs: Update installation instructions for Arch Linux
```

### Pull Request Process

1. Create a branch: `git checkout -b feature/your-feature-name`
2. Make your changes with tests
3. Commit: `git commit -m "feat: Add your feature"`
4. Push: `git push origin feature/your-feature-name`
5. Create Pull Request on GitHub

## Testing Guidelines

Before submitting a PR, test:

1. Build succeeds without warnings
2. Plugin loads in Resolve
3. Basic export works (10-30 second clip)
4. Various bitrates (128, 192, 256, 320 kbps)
5. Different audio configurations

## Documentation Contributions

- Use clear, simple language
- Include code examples
- Test all commands
- Keep formatting consistent

## Community Guidelines

- Be respectful and constructive
- Welcome newcomers
- Focus on the issue, not the person
- Assume good intentions

## License

By contributing, you agree that your contributions will be licensed under the same license as the project (MIT).

Thank you for contributing to making DaVinci Resolve better on Linux!
