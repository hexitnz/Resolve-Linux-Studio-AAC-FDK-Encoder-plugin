# Quick Start Guide

Get the FDK-AAC plugin running in DaVinci Resolve Studio in 5 minutes or less!

## Prerequisites Check

✅ **DaVinci Resolve Studio** (not the free version) - version 18.0 or later  
✅ **Linux** - Ubuntu, Fedora, Arch, or similar  
✅ **Terminal access** with sudo privileges

## Installation (3 Easy Steps)

### Step 1: Clone the Repository

```bash
# Clone the repository
git clone https://github.com/hexitnz/Resolve-Linux-Studio-AAC-FDK-Encoder-plugin.git
cd Resolve-Linux-Studio-AAC-FDK-Encoder-plugin
```

### Step 2: Run the Installer

```bash
chmod +x install.sh
sudo ./install.sh
```

The installer will:
- ✓ Detect your Linux distribution
- ✓ Check and install FDK-AAC if needed
- ✓ Build the plugin
- ✓ Install to the correct location
- ✓ Verify everything works

**This takes about 1-2 minutes.**

### Step 3: Restart DaVinci Resolve

Close DaVinci Resolve completely and reopen it.

**That's it! Installation complete.** 🎉

## Using the Plugin

### In DaVinci Resolve:

1. Go to the **Deliver** page (bottom of screen)
2. Click **Format** → Select **MP4**
3. **Video Codec** → Keep your preferred video codec (H.264, H.265, etc.)
4. **Audio Codec** → Select **AAC (FDK-AAC)** ← This is the plugin!
5. **Bitrate slider** → Set to **192 kbps** (good quality) or adjust as needed
6. Click **Add to Render Queue**
7. Click **Start Render**

### Recommended Settings

| Use Case | Bitrate | Quality |
|----------|---------|---------|
| YouTube/Social Media | 128-192 kbps | Good |
| Professional Projects | 192-256 kbps | High |
| Archive/Master | 256-320 kbps | Maximum |

## Verify It's Working

After export, check your file:

```bash
mediainfo your_exported_file.mp4 | grep -A 5 "Audio"
```

You should see:
```
Audio
Format                : AAC LTP
Bit rate              : 192 kb/s
Channel(s)            : 2 channels
Sampling rate         : 48.0 kHz
```

**Success!** Your exports now use high-quality FDK-AAC audio. 🎊

## Quick Troubleshooting

### Plugin doesn't appear in Resolve?

```bash
# Check it's installed
ls /opt/resolve/IOPlugins/aac_fdk_plugin.dvcp.bundle/

# If not found, reinstall
cd Resolve-Linux-Studio-AAC-FDK-Encoder-plugin
sudo ./install.sh
```

Then **completely restart Resolve** (close and reopen).

### Export fails?

```bash
# Check FDK-AAC is installed
ldconfig -p | grep fdk-aac

# Should show: libfdk-aac.so.2 => /usr/lib/...
# If not:
sudo apt install libfdk-aac2    # Ubuntu/Debian/Mint
sudo dnf install fdk-aac        # Fedora
sudo pacman -S libfdk-aac       # Arch
```

### Exported file has no audio?

1. Check **"Export Audio"** is ticked on Deliver page
2. Verify your timeline has audio tracks
3. Update to latest plugin version: `git pull && sudo ./install.sh`

## Need More Help?

- 📖 **Detailed docs:** See [README.md](README.md)
- 🔧 **Problems:** See [TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md)
- ❓ **Questions:** Check [FAQ.md](docs/FAQ.md)
- 🐛 **Bug reports:** Open an issue on GitHub

## Tips for Best Results

### Audio Quality Tips

- ✅ Use 192 kbps or higher for professional work
- ✅ Match timeline sample rate to source audio (usually 48 kHz)
- ✅ Don't re-encode already compressed audio multiple times

### Performance Tips

- ✅ Export to SSD for faster rendering
- ✅ Close other applications during export
- ✅ Lower bitrate = faster encoding (but lower quality)

### File Size Tips

- 📦 128 kbps: ~1 MB per minute of stereo audio
- 📦 192 kbps: ~1.4 MB per minute
- 📦 256 kbps: ~1.9 MB per minute
- 📦 320 kbps: ~2.4 MB per minute

## What Next?

Now that you have high-quality AAC encoding working:

1. **Export your projects** with superior audio quality
2. **Star the repository** ⭐ if you find it useful
3. **Share with others** who use Resolve on Linux
4. **Report issues** or **contribute** to make it even better

## Distribution-Specific Notes

### Ubuntu/Debian Users
Everything should work out of the box. The installer handles everything.

**Linux Mint 22 Users:** Since Mint 22 is based on Ubuntu 24.04, the installation process is identical and fully supported. This plugin has been tested and verified on Linux Mint 22.

### Fedora Users
The installer automatically enables RPM Fusion to get FDK-AAC. You may see a prompt - just accept.

### Arch Users
FDK-AAC is in the official repos. Super straightforward.

### Other Distributions
The installer attempts to detect and handle your package manager. If it doesn't work automatically, see [BUILDING.md](docs/BUILDING.md) for manual installation.

## Example Workflow

Here's a complete export workflow:

```bash
# 1. Edit your video in Resolve Timeline
# 2. Go to Deliver page
# 3. Configure:
#    - Format: MP4
#    - Video: H.264, CRF 23 (or your preference)
#    - Audio: AAC (FDK-AAC), 192 kbps
#    - Resolution: 1920×1080 (or your preference)
# 4. Add to queue and render
# 5. Done! High-quality video with excellent AAC audio
```

## Uninstall

If you ever need to remove the plugin:

```bash
sudo rm -rf /opt/resolve/IOPlugins/aac_fdk_plugin.dvcp.bundle
```

Then restart Resolve.

## Success Stories

This plugin provides the same AAC encoder quality used by:
- Professional broadcast equipment
- Major streaming platforms
- High-end audio production software

You're now using industry-standard audio encoding! 🎵

## 🐧 Supported Distributions
- Ubuntu 20.04, 22.04, 24.04
- Linux Mint 22 (tested - based on Ubuntu 24.04)
- Debian 11, 12
- Fedora 38, 39, 40
- Arch Linux
- openSUSE

---

**Enjoy your professional-quality AAC exports on Linux!**

For detailed documentation, see the [full README](README.md).

Questions? Check the [FAQ](docs/FAQ.md) or open an issue!
