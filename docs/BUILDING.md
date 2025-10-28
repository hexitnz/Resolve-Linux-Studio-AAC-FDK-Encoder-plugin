# Building from Source

This guide provides detailed instructions for building the FDK-AAC encoder plugin from source on various Linux distributions.

## Prerequisites

### System Requirements

- Linux distribution (Ubuntu, Fedora, Arch, Debian, or compatible)
- DaVinci Resolve Studio (version 18.x or later)
- C++ compiler with C++11 support
- CMake 3.10 or higher
- Git (for cloning the repository)

### Required Dependencies

The plugin requires the following libraries:

1. **fdk-aac** - High-quality AAC encoder library
2. **fdk-aac development headers** - Required for compilation

## Quick Build (Recommended)

The easiest way to build and install is using the automated installer:

```bash
# Clone the repository
git clone https://github.com/hexitnz/Resolve-Linux-Studio-AAC-FDK-Encoder-plugin.git
cd Resolve-Linux-Studio-AAC-FDK-Encoder-plugin

# Make installer executable
chmod +x install.sh

# Run installer (will check dependencies and build)
sudo ./install.sh
```

The installer will:
- Detect your Linux distribution
- Check for and install missing dependencies
- Build the plugin
