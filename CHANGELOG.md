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

### Added
- `examples/ffprobe-info.nemo_action`: a right-click "Media Info" action
  for Nemo (Linux Mint/Cinnamon) using `ffprobe`, useful for verifying
  exported file properties without a terminal -- particularly handy given
  MediaInfo's known misreporting issue with this plugin's output (see FAQ).

### Documentation
- Broadened the QuickTime troubleshooting entry to cover strict local
  players generally, not just QuickTime -- confirmed via a real-world
  report where a "won't play locally" file played fine via Nextcloud/VLC
  and verified correct via ffprobe and VLC's codec info panel.
- FAQ entry on MediaInfo misreporting now also covers the channel
  count/layout variant (e.g. reporting "4 channels" / "C L R Cb" instead
  of stereo), confirmed reproducible across multiple machines, with
  VLC's Codec Information panel added as a no-terminal verification option.

### Fixed (follow-up to 1.1.2)
- Removing `mkv` from `pIOPropContainerList` at registration (1.1.2) did
  **not** actually stop Resolve's deliver page from allowing MKV + this
  codec together -- confirmed by direct testing, the export proceeded and
  produced a broken (zero-duration audio) file regardless. Added a hard
  runtime guard in `DoInit` that reads `pIOPropContainerExt` and rejects
  the export outright if the target container is MKV, logging a clear
  reason instead of silently producing broken output. This needs
  real-world testing to confirm `pIOPropContainerExt` is actually
  populated at `DoInit` time for this host -- a warning is logged if not,
  so this is visible if MKV exports are ever reported as still succeeding.

## [1.1.2] - 2026-06-22

### Fixed
- **MKV exports had no audio in some players.** Confirmed via direct
  comparison: the plugin's per-frame `pIOPropPTS`/`pIOPropDuration` values
  and `pIOPropTimeBase` declaration are correct and identical to what
  produces correct timing in MP4/MOV exports, but Resolve's Matroska muxer
  consistently wrote `Duration: 00:00:00` on the audio track regardless,
  which many players treat as an empty/silent track. The SDK exposes no
  separate total-track-duration property a codec plugin could use to
  correct this -- it's a Resolve-side MKV muxer issue, not something
  fixable from the codec plugin interface. Removed `mkv` from the
  advertised container list (`pIOPropContainerList`) so it's no longer
  offered as an export option; MP4 and MOV are unaffected and remain fully
  supported. (Reported via GitHub issue feedback.)

### Documentation
- Generalized the QuickTime/strict-player troubleshooting entry further
  and added a dedicated entry explaining the MKV duration issue and the
  `ffmpeg -c copy` remux workaround for anyone who needs an MKV
  deliverable.

## [1.1.1] - 2026-06-21

### Fixed
- Build failed with `/usr/bin/ld: cannot find -lc++: No such file or directory`
  on fresh installs where `libc++` (LLVM's C++ standard library) wasn't
  separately installed alongside `clang`. The Makefile was passing
  `-stdlib=libc++` at link time, but the code has no dependency on
  libc++-specific behavior -- it's standard C++11 throughout. Removed the
  flag so the build uses the default `libstdc++` already present on every
  supported distro, removing an undocumented dependency entirely rather
  than adding it to the install instructions. (#1)

## [1.1.0] - 2026-06-20

### Fixed
- **Critical**: Codec previously did not declare a supported channel count
  (`pIOPropNumChannels`) or channel layout (`pIOPropAudioChannelLayout`) to
  Resolve, which could cause Resolve to negotiate a 4-channel (LCRS) bus
  even for stereo timelines. Both are now explicitly declared as stereo-only.
- **Critical**: Exported MP4 track timing metadata was incorrect (MediaInfo
  reported 64 kHz / "AAC LTP" instead of the actual encoded rate) because
  `pIOPropTimeBase` was never declared alongside `pIOPropPTS`/`pIOPropDuration`.
  Time base is now explicitly set to `1/sampleRate` so PTS/duration values
  (counted in samples) are interpreted correctly. Note: some tools (MediaInfo)
  may still mis-summarize the profile/rate due to a separate, known MediaInfo
  parsing quirk -- verify with `ffprobe` instead, see README FAQ.
- **Critical**: `TT_MP4_RAW` transport means FDK-AAC emits bitstream frames
  with no in-band ADTS/LATM header. The plugin was never forwarding the
  `AudioSpecificConfig` FDK-AAC computes (`aacEncInfo().confBuf`) to Resolve's
  muxer via `pIOPropMagicCookie`/`pIOPropMagicCookieType`, leaving the muxer
  to infer its own (sometimes incorrect) stream description for the `esds` box.
- User-selected bitrate (via the settings combobox) was applied correctly to
  the FDK-AAC encoder itself, but `pIOPropBitRate` was only ever set once on
  the open-time buffer, never on the actual per-sample output buffers --
  Resolve's muxer wrote a stale/default bitrate into the container regardless
  of the user's selection. Now set on every output buffer.
- Removed `pIOPropBitDepth` from compressed AAC output buffers. Bit depth is
  a PCM concept and is meaningless on compressed packets; publishing it
  could confuse host-side stream description logic (see
  github.com/Toxblh/davinci-linux-aac-codec/issues/13 for a related report
  in a sibling project).
- `DoInit` now validates channel count, bit depth, and sample rate explicitly
  and fails loudly with a logged reason instead of silently encoding
  whatever Resolve happens to send.
- `install.sh` previously embedded its own copy of the plugin source code
  inline, completely independent of `src/`, so installs could silently ship
  outdated/broken behavior even after `src/` was fixed. It now builds
  directly from `src/`, which is the single source of truth.

### Changed
- Bitrate is now a proper dropdown/combobox (96/128/160/192/224/256/320 kbps,
  default 192) instead of a slider, with validation and snap-to-nearest
  handling on load.
- Plugin is now locked to 16-bit PCM input and 2.0 stereo only (mono and
  24-bit input are no longer accepted). This removes ambiguous negotiation
  paths that contributed to the channel/layout bug above. Multi-channel
  support is tracked separately under Planned.
- `Makefile` gained an `install` target that builds, packages the
  `.dvcp.bundle`, and installs it to `/opt/resolve/IOPlugins` in one step.

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
