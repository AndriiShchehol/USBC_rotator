#pragma once

#include <Windows.h>
#include <fstream>
#include <string>

/// <summary>
/// Gets the size of a file stream in bytes.
/// </summary>
/// <param name="file">Input file stream to inspect.</param>
/// <returns>The size of the file in bytes.</returns>
unsigned long long GetFileSize(std::ifstream& file);

/// <summary>
/// Gets the current system time formatted as "YYYY-MM-DD HH:MM:SS"
/// </summary>
/// <returns>Formatted time string</returns>
std::string CurrentTime();

/// <summary>
/// Converts a binary buffer to a hexadecimal string representation
/// </summary>
/// <param name="buffer">Pointer to the binary data</param>
/// <param name="size">Size of the buffer in bytes</param>
/// <returns>Hex string (lowercase, without 0x prefix)</returns>
std::string BufferToHex(const BYTE* buffer, DWORD size);

/// <summary>
/// Converts a hexadecimal string back to a binary buffer
/// </summary>
/// <param name="hexString">Hex string to convert (must be even length)</param>
/// <param name="buffer">Output buffer to store binary data</param>
/// <param name="size">Size of output buffer in bytes</param>
void HexToBuffer(const std::string& hexString, BYTE* buffer, DWORD size);

/// <summary>
/// Converts a hexadecimal string to binary format suitable for file output
/// </summary>
/// <param name="hexString">Hex string to convert</param>
/// <returns>Binary string with actual byte values</returns>
std::string ConvertToBytes(std::string hexString);

/// <summary>
/// Extracts the lower 32 bits of a 64-bit value
/// </summary>
/// <param name="value">64-bit value</param>
/// <returns>Lower 32 bits as uint32_t</returns>
uint32_t GetLower32Bits(uint64_t value);

/// <summary>
/// Extracts the upper 32 bits of a 64-bit value
/// </summary>
/// <param name="value">64-bit value</param>
/// <returns>Upper 32 bits as uint32_t</returns>
uint32_t GetUpper32Bits(uint64_t value);

/// <summary>
/// Displays a "Press Enter to exit..." prompt and waits for user input
/// </summary>
void HoldWindowBeforeExit();
