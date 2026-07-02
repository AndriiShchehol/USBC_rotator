#include "usbc_sector_utilities.h"
#include "general_utilities.h"

#include <cstdio>

using namespace std;

bool CheckNth(ifstream& file, unsigned long long counter, unsigned char formatSample, int bytePosition) {
    char nth[3] = { 0 };

    file.seekg(counter + (bytePosition - 1));
    file.read(reinterpret_cast<char*>(&formatSample), sizeof(formatSample));
    sprintf_s(nth, sizeof(nth), "%02x", formatSample);
    file.seekg(counter);

    switch (bytePosition) {
    case 1: return strncmp(nth, "55", 2) == 0;
    case 2: return strncmp(nth, "53", 2) == 0;
    case 3: return strncmp(nth, "42", 2) == 0;
    case 4: return strncmp(nth, "43", 2) == 0;
    default: return false;
    }
}

string GetByteAtOffset(ifstream& file, unsigned long long counter, int offset, unsigned char formatSample) {
    char workingByte[3] = { 0 };

    file.seekg(counter + offset);
    file.read(reinterpret_cast<char*>(&formatSample), sizeof(formatSample));
    sprintf_s(workingByte, sizeof(workingByte), "%02x", formatSample);
    return string(workingByte);
}

string GetBytesSectorAmount(ifstream& file, unsigned long long counter, unsigned char formatSample) {
    string bytesAmountHex;

    bytesAmountHex += GetByteAtOffset(file, counter, 11, formatSample);
    bytesAmountHex += GetByteAtOffset(file, counter, 10, formatSample);
    bytesAmountHex += GetByteAtOffset(file, counter, 9, formatSample);
    bytesAmountHex += GetByteAtOffset(file, counter, 8, formatSample);

    return bytesAmountHex;
}

string GetHexSectorAmount(ifstream& file, unsigned long long counter, unsigned char formatSample) {
    string sectorsAmountHex;

    sectorsAmountHex += GetByteAtOffset(file, counter, 22, formatSample);
    sectorsAmountHex += GetByteAtOffset(file, counter, 23, formatSample);

    return sectorsAmountHex;
}

unsigned int FindImageSectorsAmount(ifstream& file, unsigned long long counter, unsigned char formatSample) {
    string bytesAmountHex = GetBytesSectorAmount(file, counter, formatSample);
    string sectorsAmountHex = GetHexSectorAmount(file, counter, formatSample);

    file.seekg(counter);

    if ((stoul(bytesAmountHex, nullptr, 16) / 512) == stoul(sectorsAmountHex, nullptr, 16) && !bytesAmountHex.empty() && !sectorsAmountHex.empty()) {
        return stoul(sectorsAmountHex, nullptr, 16);
    }

    return 0;
}

unsigned int FindDiskSectorsAmount(const BYTE* sector) {
    string sectorsAmountHex;
    string bytesAmountHex;

    sectorsAmountHex += BufferToHex(&sector[22], 1);
    sectorsAmountHex += BufferToHex(&sector[23], 1);

    bytesAmountHex += BufferToHex(&sector[11], 1);
    bytesAmountHex += BufferToHex(&sector[10], 1);
    bytesAmountHex += BufferToHex(&sector[9], 1);
    bytesAmountHex += BufferToHex(&sector[8], 1);

    if ((stoul(bytesAmountHex, nullptr, 16) / 512) == stoul(sectorsAmountHex, nullptr, 16) && !bytesAmountHex.empty() && !sectorsAmountHex.empty()) {
        return stoul(sectorsAmountHex, nullptr, 16);
    }

    return 0;
}
