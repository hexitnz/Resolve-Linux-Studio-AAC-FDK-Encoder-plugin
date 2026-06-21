#!/bin/bash
# AAC Encoder Plugin - Automated installer
#
# Builds the plugin from this repository's src/ directory (the single
# source of truth) rather than embedding its own copy of the source code.
# wrapper/ and include/ are still pulled fresh from the DaVinci Resolve
# SDK's bundled example each run, since those need to track whatever
# Resolve SDK version is actually installed on this machine.

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}============================================${NC}"
echo -e "${BLUE}AAC Encoder Plugin - Installer${NC}"
echo -e "${BLUE}============================================${NC}"
echo ""

# Resolve this script's own directory so it can be run from anywhere,
# and locate the repo's src/ directory relative to it.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="$SCRIPT_DIR/src"

if [ ! -d "$SRC_DIR" ]; then
    echo -e "${RED}[✗]${NC} Could not find src/ directory next to this script ($SRC_DIR)"
    echo "    Make sure you're running install.sh from inside the cloned repository."
    exit 1
fi

if [ ! -f "$SRC_DIR/aac_encoder.cpp" ] || [ ! -f "$SRC_DIR/Makefile" ]; then
    echo -e "${RED}[✗]${NC} src/ directory is missing expected plugin source files"
    exit 1
fi

if [ ! -d "/opt/resolve/Developer/CodecPlugin" ]; then
    echo -e "${RED}[✗]${NC} DaVinci Resolve SDK not found!"
    echo "    Expected to find: /opt/resolve/Developer/CodecPlugin"
    echo "    Make sure DaVinci Resolve Studio is installed (free version does not include the SDK)."
    exit 1
fi

if ! pkg-config --exists fdk-aac; then
    echo -e "${RED}[✗]${NC} libfdk-aac not found!"
    echo ""
    echo "    Install it first, e.g.:"
    echo "      Ubuntu/Debian/Mint : sudo apt install libfdk-aac-dev"
    echo "      Arch               : sudo pacman -S libfdk-aac"
    echo "      Fedora             : sudo dnf install libfdk-aac-devel"
    exit 1
fi

if ! command -v clang++ >/dev/null 2>&1 && ! command -v g++ >/dev/null 2>&1; then
    echo -e "${RED}[✗]${NC} No C++ compiler found (need clang++ or g++)"
    exit 1
fi

echo -e "${GREEN}[✓]${NC} Prerequisites OK"

WORK_DIR="$HOME/aac_working_plugin"
rm -rf "$WORK_DIR"
mkdir -p "$WORK_DIR"
cd "$WORK_DIR"

# wrapper/ and include/ come from the SDK bundled with the locally
# installed DaVinci Resolve Studio, not from the repo -- these need to
# match whatever Resolve version is actually on this machine.
cp -r /opt/resolve/Developer/CodecPlugin/Examples/x264_encoder_plugin/wrapper .
cp -r /opt/resolve/Developer/CodecPlugin/Examples/x264_encoder_plugin/include .

# Everything plugin-specific comes from the repo's src/ directory --
# this is the only place this code should ever be edited. Previous
# versions of this installer embedded their own copy of these files
# inline, which silently drifted out of sync with src/ and caused
# installs to ship outdated/broken behavior even after src/ was fixed.
cp "$SRC_DIR/aac_encoder.h" .
cp "$SRC_DIR/aac_encoder.cpp" .
cp "$SRC_DIR/plugin.h" .
cp "$SRC_DIR/plugin.cpp" .
cp "$SRC_DIR/Makefile" .

echo -e "${BLUE}[*]${NC} Building from $SRC_DIR ..."
make clean >/dev/null 2>&1 || true
make

if [ ! -f "bin/aac_fdk_plugin.dvcp" ]; then
    echo -e "${RED}[✗]${NC} Build failed!"
    exit 1
fi

echo -e "${GREEN}[✓]${NC} Build successful"

BUNDLE_DIR="aac_fdk_plugin.dvcp.bundle"
rm -rf "$BUNDLE_DIR"
mkdir -p "$BUNDLE_DIR/Contents/Linux-x86-64"
cp bin/aac_fdk_plugin.dvcp "$BUNDLE_DIR/Contents/Linux-x86-64/"
chmod 755 "$BUNDLE_DIR/Contents/Linux-x86-64/aac_fdk_plugin.dvcp"

echo -e "${BLUE}[*]${NC} Installing to /opt/resolve/IOPlugins (requires sudo) ..."
sudo mkdir -p /opt/resolve/IOPlugins
sudo rm -rf /opt/resolve/IOPlugins/aac_fdk_plugin.dvcp.bundle
sudo cp -r "$BUNDLE_DIR" /opt/resolve/IOPlugins/
sudo chmod -R 755 /opt/resolve/IOPlugins/aac_fdk_plugin.dvcp.bundle

echo ""
echo -e "${GREEN}============================================${NC}"
echo -e "${GREEN}Installation Complete!${NC}"
echo -e "${GREEN}============================================${NC}"
echo ""
echo "Built from: $SRC_DIR"
echo "Installed to: /opt/resolve/IOPlugins/aac_fdk_plugin.dvcp.bundle"
echo ""
echo "Restart DaVinci Resolve completely for the change to take effect:"
echo "  killall resolve && /opt/resolve/bin/resolve"
echo ""
