# USBC Rotator (Low-Level Disk Utility)

A specialized C++ system utility designed for data recovery and raw sector manipulation. This tool identifies, analyzes, and repairs corrupted USBC (USB Bulk-Only Transport) protocol blocks by performing direct disk I/O and circular data rotation.

## 🛠 Features

- **Direct Disk Access:** Interfaces directly with physical drives (`\\.\PhysicalDriveX`) or raw disk images using Win32 API.
- **Signature Analysis:** Automatically scans for `USBC` signatures at the sector level (512 bytes).
- **Circular Rotation Algorithm:** Restores data integrity by shifting sectors within a identified block based on protocol-specific offsets.
- **Binary/Hex Handling:** Manual parsing of Little-endian headers to determine block lengths and sector counts.
- **Safe Operations:** Includes verification steps, data flushing (`FlushFileBuffers`), and comprehensive logging.

## 💻 Technical Highlights

- **Win32 System Programming:** Extensive use of `CreateFile`, `ReadFile`, `WriteFile`, and `SetFilePointer` for non-buffered hardware interaction.
- **Memory Management:** Efficient use of dynamic buffers and byte-level manipulation (`memcpy`, `memcmp`).
- **Data Recovery Logic:** Implements logic to bypass standard OS file system limitations to reach "raw" data.

## 🚀 How It Works

1. **Scan:** The tool reads the disk sector by sector looking for the `USBC` signature bytes.
2. **Analyze:** Once found, it parses sector header to calculate the expected block length.
3. **Transform:** It performs a circular shift (moving the header to the end or realignment) to restore the original data order.
4. **Log:** All actions, including offsets and block lengths, are recorded in a `.log` file for audit.

## ⚠️ Requirements & Safety

- **Platform:** Windows (requires Administrator privileges for physical drive access).
- **Warning:** This tool performs direct writes to storage media. Use with extreme caution on production drives.

## 🔧 Build

Compiled using **MSVC** (Microsoft Visual Studio) or **GCC** (MinGW).
