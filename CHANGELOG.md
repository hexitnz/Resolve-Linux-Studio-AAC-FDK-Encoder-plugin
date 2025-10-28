# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Planned
- Multi-channel (5.1, 7.1) surround sound support
- Additional AAC profile options (LC, HE-AAC, HE-AACv2)
- Configurable encoder quality modes
- Real-time encoding progress indicator

## [1.0.0] - 2025-10-26

### Added
- Initial release of FDK-AAC encoder plugin for DaVinci Resolve Studio on Linux
- High-quality AAC audio encoding using FDK-AAC library
- Configurable bitrate from 128 kbps to 320 kbps via UI slider
- Support for stereo (2-channel) audio encoding
- Sample rates: 48 kHz (primary), 44.1 kHz, and 32 kHz
- VBR (Variable Bitrate) encoding mode for optimal quality
- Proper PTS (Presentation Timestamp) and duration handling for MP4 muxing
- Ring buffer implementation for exact AAC frame alignment (1024 samples)
- Automated installer script with dependency checking
- Support for major Linux distributions:
  - Ubuntu 20.04, 22.04, 24.04
  - Linux Mint 22 (tested - based on Ubuntu 24.04)
  - Debian 11, 12
  - Fedora 38, 39, 40
  - Arch Linux
  - openSUSE Leap/Tumbleweed

### Documentation
- Comprehensive README with installation and usage instructions
- Detailed building guide (BUILDING.md)
- Troubleshooting guide (TROUBLESHOOTING.md)
- GitHub repository setup guide (GITHUB_SETUP.md)
- Contributing guidelines (CONTRIBUTING.md)
- MIT License with third-party acknowledgments

### Technical Details
- Uses FDK-AAC in VBR mode 5 for high quality
- Audio Object Type: AAC-LC (Low Complexity)
- Frame size: 1024 samples per frame
- Buffer management with proper memory allocation
- Thread-safe buffer operations
- Efficient sample format conversion (float to int16)

### Known Limitations
- Stereo only (multi-channel not yet implemented)
- Fixed VBR mode (configurable modes planned for future release)
- Linux only (macOS and Windows support not planned)
- Requires DaVinci Resolve Studio 18.0 or later (tested on 20.1)

## Version History Notes

### [1.0.0] - Release Notes

This is the first stable release of the FDK-AAC plugin for DaVinci Resolve Studio on Linux. After extensive development and testing, the plugin now reliably encodes high-quality AAC audio for video exports.

**Key Achievements:**
- Successfully resolves the lack of native FDK-AAC support in Linux versions of DaVinci Resolve
- Provides superior audio quality compared to some built-in AAC encoders
- Easy installation with automated dependency management
- Comprehensive documentation for users and contributors

**Testing:**
- Tested on Linux Mint 22 (based on Ubuntu 24.04) with DaVinci Resolve Studio 20.1
- Verified exports with various bitrates (128-320 kbps)
- Confirmed proper audio synchronization and MP4 muxing
- Validated with MediaInfo and audio analysis tools

**Breaking Changes:**
- N/A (initial release)

**Migration Guide:**
- N/A (initial release)

**Contributors:**
- [Your Name] - Initial development and release

---

## Future Roadmap

### Version 1.1.0 (Planned)
- [ ] Add 5.1 surround sound support
- [ ] Implement AAC-HE (High Efficiency) profile
- [ ] Add encoder quality presets (low/medium/high/ultra)
- [ ] Improve error handling and user feedback

### Version 1.2.0 (Planned)
- [ ] Support for more sample rates (96 kHz, 192 kHz)
- [ ] AAC-HEv2 profile support
- [ ] Custom encoder parameter configuration file
- [ ] Performance optimizations for multi-core systems

### Version 2.0.0 (Future)
- [ ] Major refactoring for improved maintainability
- [ ] Plugin settings UI within DaVinci Resolve
- [ ] Support for additional container formats
- [ ] Advanced audio processing options

---

## Release Process

### Versioning Scheme

- **Major (X.0.0):** Breaking changes, major new features
- **Minor (1.X.0):** New features, backwards compatible
- **Patch (1.0.X):** Bug fixes, minor improvements

### How to Release

1. Update version number in CMakeLists.txt
2. Update this CHANGELOG.md with release notes
3. Tag the release:
   ```bash
   git tag -a v1.0.0 -m "Release version 1.0.0"
   git push origin v1.0.0
   ```
4. Create GitHub release with compiled binaries (if applicable)
5. Update README.md with latest version info

---

## Deprecation Notices

None at this time.

---

## Security Updates

None at this time.

For security issues, please see [SECURITY.md](SECURITY.md) (if applicable) or contact the maintainer directly.

---

[Unreleased]: https://github.com/hexitnz/Resolve-Linux-Studio-AAC-FDK-Encoder-plugin/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/hexitnz/Resolve-Linux-Studio-AAC-FDK-Encoder-plugin/releases/tag/v1.0.0
