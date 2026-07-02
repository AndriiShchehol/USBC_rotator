#pragma once

#include <Windows.h>
#include <fstream>
#include <string>

/// <summary>
/// Checks whether a specific byte position in the file matches the expected USBC signature byte.
/// </summary>
/// <param name="file">Input file stream positioned at the start of the target block.</param>
/// <param name="counter">Absolute file offset of the current block.</param>
/// <param name="formatSample">Byte value used as a temporary read buffer.</param>
/// <param name="bytePosition">The byte position to validate (1-4).</param>
/// <returns>True if the signature byte matches the expected USBC marker.</returns>
bool CheckNth(std::ifstream& file, unsigned long long counter, unsigned char formatSample, int bytePosition);

/// <summary>
/// Reads one byte from the specified offset within a file-based USBC block and returns it as hex.
/// </summary>
/// <param name="file">Input file stream positioned at the start of the target block.</param>
/// <param name="counter">Absolute file offset of the current block.</param>
/// <param name="offset">Byte offset relative to the block start.</param>
/// <param name="formatSample">Byte value used as a temporary read buffer.</param>
/// <returns>Hex string representation of the read byte.</returns>
std::string GetByteAtOffset(std::ifstream& file, unsigned long long counter, int offset, unsigned char formatSample);

/// <summary>
/// Reads the byte-count portion of a USBC header from the file stream.
/// </summary>
/// <param name="file">Input file stream positioned at the start of the target block.</param>
/// <param name="counter">Absolute file offset of the current block.</param>
/// <param name="formatSample">Byte value used as a temporary read buffer.</param>
/// <returns>Hex string representing the header byte count.</returns>
std::string GetBytesSectorAmount(std::ifstream& file, unsigned long long counter, unsigned char formatSample);

/// <summary>
/// Reads the sector-count portion of a USBC header from the file stream.
/// </summary>
/// <param name="file">Input file stream positioned at the start of the target block.</param>
/// <param name="counter">Absolute file offset of the current block.</param>
/// <param name="formatSample">Byte value used as a temporary read buffer.</param>
/// <returns>Hex string representing the header sector count.</returns>
std::string GetHexSectorAmount(std::ifstream& file, unsigned long long counter, unsigned char formatSample);

/// <summary>
/// Determines the USBC block size in sectors by comparing the header byte count and sector count.
/// </summary>
/// <param name="file">Input file stream positioned at the start of the target block.</param>
/// <param name="counter">Absolute file offset of the current block.</param>
/// <param name="formatSample">Byte value used as a temporary read buffer.</param>
/// <returns>The validated sector count, or 0 if the values do not match.</returns>
unsigned int FindImageSectorsAmount(std::ifstream& file, unsigned long long counter, unsigned char formatSample);

/// <summary>
/// Determines the USBC block size in sectors from an in-memory sector buffer.
/// </summary>
/// <param name="sector">Pointer to the first byte of the USBC sector.</param>
/// <returns>The validated sector count, or 0 if the values do not match.</returns>
unsigned int FindDiskSectorsAmount(const BYTE* sector);
