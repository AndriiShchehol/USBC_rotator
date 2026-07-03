#include "usbc_sector_utilities.h"
#include "general_utilities.h"

#include <cstdio>

using namespace std;

string GetMovedByteCount(const BYTE* sector) {
    string byteCountHex;

    // Read offsets 11, 10, 9, 8 because number is written in REVERSE order (little-endian)
    byteCountHex += BufferToHex(&sector[11], 1);
    byteCountHex += BufferToHex(&sector[10], 1);
    byteCountHex += BufferToHex(&sector[9], 1);
    byteCountHex += BufferToHex(&sector[8], 1);

    return byteCountHex;
}

string GetMovedSectorCount(const BYTE* sector) {
    string sectorCountHex;

    // Read offsets 22, 23 because number is written in NORMAL order (big-endian)
    sectorCountHex += BufferToHex(&sector[22], 1);
    sectorCountHex += BufferToHex(&sector[23], 1);

    return sectorCountHex;
}

unsigned int GetMovedBlockLength(const BYTE* sector) {
    string byteCountHex = GetMovedByteCount(sector);
    string sectorCountHex = GetMovedSectorCount(sector);

    // Validate: byte count divided by 512 should equal sector count
    if ((stoul(byteCountHex, nullptr, 16) / 512) == stoul(sectorCountHex, nullptr, 16) && 
        !byteCountHex.empty() && !sectorCountHex.empty()) {
        return stoul(sectorCountHex, nullptr, 16);
    }

    return 0;
}
