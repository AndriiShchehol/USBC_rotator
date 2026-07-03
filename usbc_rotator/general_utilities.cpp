#include "general_utilities.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>

using namespace std;

string CurrentTime() {
	time_t now = time(0);
	tm localTime;
	localtime_s(&localTime, &now);

	char buffer[80];
	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &localTime);

	return string(buffer);
}

string BufferToHex(const BYTE* buffer, DWORD size) {
	stringstream hexStream;
	for (DWORD i = 0; i < size; ++i) {
		hexStream << hex << setw(2) << setfill('0') << static_cast<int>(buffer[i]);
	}
	return hexStream.str();
}

void HexToBuffer(const string& hexString, BYTE* buffer, DWORD size) {
	for (DWORD i = 0; i < size; ++i) {
		string byteString = hexString.substr(i * 2, 2);
		buffer[i] = static_cast<BYTE>(stoi(byteString, nullptr, 16));
	}
}

string ConvertToBytes(string hexString) {
	basic_string<uint8_t> bytes;

	for (size_t i = 0; i < hexString.length(); i += 2) {
		uint16_t byte;
		string nextByte = hexString.substr(i, 2);
		istringstream(nextByte) >> hex >> byte;
		bytes.push_back(static_cast<uint8_t>(byte));
	}

	return string(begin(bytes), end(bytes));
}

uint32_t GetLower32Bits(uint64_t value) {
	return static_cast<uint32_t>(value & 0xFFFFFFFFLL);
}

uint32_t GetUpper32Bits(uint64_t value) {
	return static_cast<uint32_t>((value & 0xFFFFFFFF00000000LL) >> 32);
}

void HoldWindowBeforeExit() {
	cout << "Press Enter to exit...";
	cin.get();
}
