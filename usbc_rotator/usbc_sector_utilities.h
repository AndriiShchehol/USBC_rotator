#pragma once

#include <Windows.h>
#include <fstream>
#include <string>

/// <summary>
/// Extracts the actual byte count from a USBC sector header (little-endian, offsets 8-11).
/// </summary>
/// <param name="sector">Pointer to the first byte of the USBC sector.</param>
/// <returns>Hex string representing the byte count (4 hex digits).</returns>
std::string GetMovedByteCount(const BYTE* sector);

/// <summary>
/// Extracts the sector count from a USBC sector header (big-endian, offsets 22-23).
/// </summary>
/// <param name="sector">Pointer to the first byte of the USBC sector.</param>
/// <returns>Hex string representing the sector count (2 hex digits).</returns>
std::string GetMovedSectorCount(const BYTE* sector);

/// <summary>
/// Determines the USBC block size in sectors from an in-memory sector buffer.
/// </summary>
/// <param name="sector">Pointer to the first byte of the USBC sector.</param>
/// <returns>The validated sector count, or 0 if the values do not match.</returns>
unsigned int GetMovedBlockLength(const BYTE* sector);
