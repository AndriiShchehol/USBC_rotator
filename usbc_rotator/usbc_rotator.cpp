#include <Windows.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <ctime>

using namespace std;

string CurrentTime() {
	// Get the current time
	time_t now = time(0);
    tm localTime;
    localtime_s(&localTime, &now);

	// Format the time as a string
	char buffer[80];
	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &localTime);

	return string(buffer);
}


// moving to the end of file and saving its position as a length for our current file so we could know how much bytes our file has
unsigned long long getFileSize(ifstream& file) {
	unsigned long long fileSize = 0; 

    file.seekg(0, ios::end);
    fileSize = file.tellg();
    file.seekg(0, ios::beg);
 
	return fileSize;
}


// Function to convert a buffer to a hexadecimal string
string BufferToHex(const BYTE* buffer, DWORD size) {
    stringstream hexStream;
    for (DWORD i = 0; i < size; ++i) {
        hexStream << hex << setw(2) << setfill('0') << static_cast<int>(buffer[i]);
    }
    return hexStream.str();
}


// Function to convert a hexadecimal string back to a binary buffer
void HexToBuffer(const string& hexString, BYTE* buffer, DWORD size) {
    for (DWORD i = 0; i < size; ++i) {
        string byteString = hexString.substr(i * 2, 2);
        buffer[i] = static_cast<BYTE>(stoi(byteString, nullptr, 16));
    }
}


// using a function to authomatize comparation process for usbc hex values
bool Check_Nth(ifstream& file, unsigned long long counter, unsigned char formatSample, int bytePosition) {

    char Nth[3] = { 0 };


    file.seekg(counter + (bytePosition - 1));
    file.read(reinterpret_cast <char*> (&formatSample), sizeof(formatSample));
    sprintf_s(Nth, "%02x", formatSample);
    file.seekg(counter);

    switch (bytePosition) {
        case 1: return strncmp(Nth, "55", 2) == 0; break;
        case 2: return strncmp(Nth, "53", 2) == 0; break;
        case 3: return strncmp(Nth, "42", 2) == 0; break;
        case 4: return strncmp(Nth, "43", 2) == 0; break;
        default: return false; break;
    }

};


char getByteAtOfcet(ifstream& file, unsigned long long counter, int ofcet, unsigned char formatSample) {
    char workingByte[3] = { 0 };
   
    file.seekg(counter + ofcet);
    file.read(reinterpret_cast <char*> (&formatSample), sizeof(formatSample));
    return sprintf_s(workingByte, "%02x", formatSample);
}


string getBytesSectorAmount(ifstream& file, unsigned long long counter, unsigned char formatSample) {
    string bytesAmountHex = "";
    
	bytesAmountHex += getByteAtOfcet(file, counter, 11, formatSample);
	bytesAmountHex += getByteAtOfcet(file, counter, 10, formatSample);
	bytesAmountHex += getByteAtOfcet(file, counter, 9, formatSample);
	bytesAmountHex += getByteAtOfcet(file, counter, 8, formatSample);
	
	return bytesAmountHex;
}


string getHexSectorAmount(ifstream& file, unsigned long long counter, unsigned char formatSample) {
	string sectorsAmountHex = "";

	sectorsAmountHex += getByteAtOfcet(file, counter, 22, formatSample);
	sectorsAmountHex += getByteAtOfcet(file, counter, 23, formatSample);

	return sectorsAmountHex;
}


// using this function for searching and comparing two different values that may tell us about circular rotation block length (images)
unsigned int findImageSectorsAmount(ifstream& file, unsigned long long counter, unsigned char formatSample) { // 

    /*char sectorsByteHex[3] = { 0 };
    char workingByte[3] = { 0 };
    string sectorsAmountHex = "";
    string bytesAmountHex = "";*/


    /*file.seekg(counter + 22);
    file.read(reinterpret_cast <char*> (&formatSample), sizeof(formatSample));
    sprintf_s(sectorsByteHex, "%02x", formatSample);
    sectorsAmountHex.append(sectorsByteHex);

    file.seekg(counter + 23);
    file.read(reinterpret_cast <char*> (&formatSample), sizeof(formatSample));
    sprintf_s(sectorsByteHex, "%02x", formatSample);
    sectorsAmountHex.append(sectorsByteHex);*/

    /*file.seekg(counter + 11);
    file.read(reinterpret_cast <char*> (&formatSample), sizeof(formatSample));
    sprintf_s(workingByte, "%02x", formatSample);
    bytesAmountHex.append(workingByte);
    
    file.seekg(counter + 10);
    file.read(reinterpret_cast <char*> (&formatSample), sizeof(formatSample));
    sprintf_s(workingByte, "%02x", formatSample);
    bytesAmountHex.append(workingByte);

    file.seekg(counter + 9);
    file.read(reinterpret_cast <char*> (&formatSample), sizeof(formatSample));
    sprintf_s(workingByte, "%02x", formatSample);
    bytesAmountHex.append(workingByte);

    file.seekg(counter + 8);
    file.read(reinterpret_cast <char*> (&formatSample), sizeof(formatSample));
    sprintf_s(workingByte, "%02x", formatSample);
    bytesAmountHex.append(workingByte);*/


    string bytesAmountHex = getBytesSectorAmount(file, counter, formatSample); // using our function to get bytes amount in hex format
	string sectorsAmountHex = getHexSectorAmount(file, counter, formatSample); // using our function to get sectors amount in hex format
    
    file.seekg(counter);


    // checking if both values give us the same result so we will not touch incomplete or wrong sectors to prevent even further image corruption
    if ((stoul(bytesAmountHex, nullptr, 16) / 512) == stoul(sectorsAmountHex, nullptr, 16) && bytesAmountHex != "" && sectorsAmountHex != "") {
        return stoul(sectorsAmountHex, nullptr, 16);
	}
    else {
        return 0;
    }
}


// Function to find the number of sectors in the usbc block
unsigned int findDiskSectorsAmount(const BYTE* sector) {
    string sectorsAmountHex = "";
    string bytesAmountHex = "";

	// Extract the sector count (bytes 22-23) in direct order
    sectorsAmountHex += BufferToHex(&sector[22], 1);
    sectorsAmountHex += BufferToHex(&sector[23], 1);

	// Extract the byte count (bytes 8-11) in reverse order
    bytesAmountHex += BufferToHex(&sector[11], 1);
    bytesAmountHex += BufferToHex(&sector[10], 1);
    bytesAmountHex += BufferToHex(&sector[9], 1);
    bytesAmountHex += BufferToHex(&sector[8], 1);

    // Validate and return the sector count
    if ((stoul(bytesAmountHex, nullptr, 16) / 512) == stoul(sectorsAmountHex, nullptr, 16) && bytesAmountHex != "" && sectorsAmountHex != "") {
        return stoul(sectorsAmountHex, nullptr, 16);
    }

    return 0;
}


// using a function to convert our hex into bytes so we wont lose any data
string ConvertToBytes(string hexString) {

    basic_string<uint8_t> bytes;


    for (size_t i = 0; i < hexString.length(); i += 2)
    {
        uint16_t byte;
        

        // Get current pair and store in nextbyte
        string nextByte = hexString.substr(i, 2);

        // Put the pair into an istringstream and stream it through std::hex for
        // conversion into an integer value.
        // This will calculate the byte value of your string-represented hex value.
        // converting our bytes into hex values 
        istringstream(nextByte) >> hex >> byte;

        // adding our values into our byte array via casting because stream does not work with uint8 directly
        bytes.push_back(static_cast<uint8_t>(byte));
    }

    // creating a string with binary values so we can output it directly into file
    string result(begin(bytes), end(bytes));

    return result;
}


uint32_t GetLower32Bits(uint64_t value) {
	return static_cast<uint32_t>(value & 0xFFFFFFFFLL);
}

uint32_t GetUpper32Bits(uint64_t value) {
	return static_cast<uint32_t>((value & 0xFFFFFFFF00000000LL) >> 32);
}


// main code functionality taken outside of main function for logic construction convenience
//int WorkWithUsbcImage(ifstream& diskImageInput, ofstream& diskImageOutput, ofstream& logFile) {
//
//    char currentByte[3] = { 0 };
//
//    unsigned char charSample;
//    unsigned long long fileSize = 0;
//    unsigned long long i = 0;
//
//    int usbcCounter = 0;                                              
//    int usbcRotationBlockLength = 0;
//    
//    string usbcSector = "";
//    string bytesOfUsbcSector = "";
//    string rotatedSectors = "";
//    string bytesOfRotatedSectors = "";
//
//    //int usbcOutputCounter = 0;
//    //int rotationOutputCounter = 0;
//
//
//    if (diskImageInput.is_open()) {
//
//		fileSize = getFileSize(diskImageInput); // getting file size to know how much bytes we have to work with
//
//        // using a cycle to search for usbc
//        while (fileSize > i) {
//
//            diskImageInput.read(reinterpret_cast <char*> (&charSample), sizeof(charSample));
//            sprintf_s(currentByte, "%02x", charSample);
//
//            // whole file formatted output inside the console window
//            //if (i % 1 == 0) { cout << " "; };                     
//            //if (i % 4 == 0) { cout << " "; };
//            //if (i % 16 == 0) { cout << endl; };
//            //if (i % 512 == 0) { cout << endl; };
//            //cout << currentByte;
//            //diskImageInput.seekg(++i);
//
//
//            // checking each each corresponding byte one after anouther to spend less resources on wrong coincidences
//            if (Check_Nth(diskImageInput, i, charSample, 1) == 0) {
//                if (Check_Nth(diskImageInput, i, charSample, 2) == 0) {
//                    if (Check_Nth(diskImageInput, i, charSample, 3) == 0) {
//                        if (Check_Nth(diskImageInput, i, charSample, 4) == 0) {
//
//                            usbcRotationBlockLength = findImageSectorsAmount(diskImageInput, i, charSample);
//
//                            usbcSector = "";
//                            rotatedSectors = "";
//                            
//
//                            for (int usbcSector_byte = 0; usbcSector_byte < 512; usbcSector_byte++) {
//                                diskImageInput.seekg(i + usbcSector_byte);
//                                diskImageInput.read(reinterpret_cast <char*>(&charSample), sizeof(charSample));
//                                
//                                if (usbcSector_byte == 0) {usbcSector.append("44");}
//                                else if (usbcSector_byte == 1) {usbcSector.append("4F");}
//                                else if (usbcSector_byte == 2) {usbcSector.append("4E");}
//                                else if (usbcSector_byte == 3) {usbcSector.append("45");}
//                                else {
//                                    sprintf_s(currentByte, "%02x", charSample);
//                                    usbcSector.append(currentByte);
//                                }
//
//                            }
//                            diskImageInput.seekg(i);
//                            
//
//                            for (int rotatedSectorNumber = 1; rotatedSectorNumber < usbcRotationBlockLength; rotatedSectorNumber++) {
//                                for (int rotatedSectorByte = 0; rotatedSectorByte < 512; rotatedSectorByte++) {
//                                    diskImageInput.seekg(i + (rotatedSectorNumber * 512) + rotatedSectorByte);
//                                    diskImageInput.read(reinterpret_cast <char*>(&charSample), sizeof(charSample));
//                                    sprintf_s(currentByte, "%02x", charSample);
//                                    rotatedSectors.append(currentByte);
//                                }
//                            }
//                            diskImageInput.seekg(i);
//
//
//                            // each of sectors console output for debugging purposes
//                            /*cout << "\n\n usbc sector " << i << " \n\n";
//
//                            for (auto usbcOutputSymbol : usbcSector) { 
//                                
//                                if (usbcOutputCounter % 2 == 0) { cout << " "; }; 
//                                if (usbcOutputCounter % 8 == 0) { cout << " "; };
//                                if (usbcOutputCounter % 64 == 0) { cout << endl; };
//                                if (usbcOutputCounter % 1024 == 0) { cout << endl; };
//
//                                usbcOutputCounter++;
//
//                                cout << usbcOutputSymbol;
//
//                            }
//
//                            cout << "\n\n rotation sector " << i << " \n\n";
//
//                            for (auto rotationOutputSymbol : rotatedSectors) {
//
//                                if (rotationOutputCounter % 2 == 0) { cout << " "; };
//                                if (rotationOutputCounter % 8 == 0) { cout << " "; };
//                                if (rotationOutputCounter % 64 == 0) { cout << endl; };
//                                if (rotationOutputCounter % 1024 == 0) { cout << endl; };
//
//                                rotationOutputCounter++;
//
//                                cout << rotationOutputSymbol;
//
//                            }*/
//
//
//                            bytesOfUsbcSector = ConvertToBytes(usbcSector);
//                            bytesOfRotatedSectors = ConvertToBytes(rotatedSectors);
//
//                            usbcCounter++;
//                            
//
//                            if (usbcRotationBlockLength > 0) {
//                                diskImageOutput.seekp(i);
//                                diskImageOutput << bytesOfRotatedSectors;
//
//                                diskImageOutput.seekp(i + ((usbcRotationBlockLength - 1) * 512));
//                                diskImageOutput << bytesOfUsbcSector;
//                            }
//                            else if (usbcRotationBlockLength == 0) {
//                                diskImageOutput.seekp(i);
//                                diskImageOutput << bytesOfUsbcSector;
//                            }
//                            
//                          
//                            cout << endl << " usbc sector on byte " << i << " with rotation block length of " << usbcRotationBlockLength << " has been rotated" << endl;
//							logFile << endl << " usbc sector on byte " << i << " with rotation block length of " << usbcRotationBlockLength << " has been rotated" << endl;
//
//                            
//                            // going to the point after current rotation block to prevent wasting time on searching inside of it
//                            diskImageInput.seekg(i + (usbcRotationBlockLength * 512 + 1));
//                        }
//                    }
//                    else {diskImageInput.seekg(i += 512);}
//                }
//                else {diskImageInput.seekg(i += 512);}
//
//            }
//            else {diskImageInput.seekg(i += 512);}
//        }
//        
//
//        cout << endl << endl << "Total USBC counter: " << usbcCounter;
//		logFile << endl << endl << "Total USBC counter: " << usbcCounter;
//
//        
//        return 0;
//    }
//    else {
//        cerr << "Error opening file." << endl;
//		logFile << "Error opening file." << endl;
//        return 1;
//    }
//
//}


// Function to list available physical drives
vector<string> ListAvailableDisks() {
    vector<string> disks;
    for (int i = 0; i < 10; ++i) { // Check for drives 0-9
        string diskPath = "\\\\.\\PhysicalDrive" + to_string(i);
		wstring wideDiskPath = wstring(diskPath.begin(), diskPath.end()); // Convert to wide string for CreateFile function

        HANDLE hDisk = CreateFile(
            wideDiskPath.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
        );

        if (hDisk != INVALID_HANDLE_VALUE) {
            disks.push_back(diskPath);
            CloseHandle(hDisk);
        }
    }
    return disks;
}






void ProcessImage(const string& imagePath, ofstream& logFile) {
    // Open the image file (regular file) for read/write using Win32 API
    wstring wideImagePath = wstring(imagePath.begin(), imagePath.end());

    HANDLE hFile = CreateFile(
        wideImagePath.c_str(),                  // Image file path
        GENERIC_READ | GENERIC_WRITE,           // Access mode
        FILE_SHARE_READ | FILE_SHARE_WRITE,     // Share mode
        NULL,                                   // Security attributes
        OPEN_EXISTING,                          // Open existing file
        FILE_ATTRIBUTE_NORMAL,                  // Flags and attributes for a normal file
        NULL                                    // Template file
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        cerr << endl << "Failed to open image file. Error: " << GetLastError() << endl;
        logFile << endl << "Failed to open image file: " << imagePath << ". Error: " << GetLastError() << endl;
        return;
    }

    const DWORD sectorSize = 512; // Sector size
    BYTE* buffer = new BYTE[sectorSize];
    DWORD bytesRead, bytesWritten;

    unsigned long long offset = 0;
    long offsetLow32 = 0;
    long offsetHigh32 = 0;

    cout << endl << "Processing image: " << imagePath << endl << endl;
    logFile << endl << "Processing image: " << imagePath << endl << endl;

    // Get the total image size
    LARGE_INTEGER imageSize;
    if (!GetFileSizeEx(hFile, &imageSize)) {
        cerr << "Failed to get image size. Error: " << GetLastError() << endl;
        logFile << "Failed to get image size for: " << imagePath << ". Error: " << GetLastError() << endl;
        delete[] buffer;
        CloseHandle(hFile);
        return;
    }

    while (offset <static_cast<unsigned long long>(imageSize.QuadPart)) {

        // Position file pointer
        offsetLow32 = GetLower32Bits(offset);
        offsetHigh32 = GetUpper32Bits(offset);
        SetFilePointer(hFile, offsetLow32, &offsetHigh32, FILE_BEGIN);

        // Read a single sector
        if (!ReadFile(hFile, buffer, sectorSize, &bytesRead, NULL) || bytesRead != sectorSize) {
            // Read error or EOF
            if (GetLastError() != ERROR_HANDLE_EOF) {
                cerr << endl << "Failed to read sector at offset " << offset << ". Error: " << GetLastError() << endl << endl;
                logFile << endl << "Failed to read sector at offset " << offset << ". Error: " << GetLastError() << endl << endl;
            }
            // Move on (break to avoid infinite loop) — advance by sector to attempt continuation
            offset += sectorSize;
            break;
        }

        // Check if the sector starts with "USBC"
        const BYTE expectedSignature[4] = { 'U', 'S', 'B', 'C' };
        if (memcmp(buffer, expectedSignature, 4) == 0) {

            cout << "Found 'USBC' at offset: " << offset << " bytes" << endl;
            logFile << "[" << CurrentTime() << "]" << endl
                << "Found 'USBC' at offset: " << offset << " bytes" << endl;

            // Determine the length of the USBC block
            unsigned int blockLength = findDiskSectorsAmount(buffer);


            cout << "USBC block length: " << blockLength << " sectors" << endl;
            logFile << "USBC block length: " << blockLength << " sectors" << endl;
            
            
            if (blockLength == 0 || blockLength == 1) {
                cerr << "Invalid block length at offset " << offset << ". Skipping..." << endl << endl;
                logFile << "Invalid block length at offset " << offset << ". Skipping..." << endl << endl;
                offset += sectorSize;
                continue;
			}

            // Read the entire block into memory
            BYTE* blockBuffer = new BYTE[blockLength * sectorSize];

            offsetLow32 = GetLower32Bits(offset);
            offsetHigh32 = GetUpper32Bits(offset);
            SetFilePointer(hFile, offsetLow32, &offsetHigh32, FILE_BEGIN);

            if (!ReadFile(hFile, blockBuffer, blockLength * sectorSize, &bytesRead, NULL) || bytesRead != blockLength * sectorSize) {
                cerr << "Failed to read USBC block. Error: " << GetLastError() << endl;
                logFile << "Failed to read USBC block at offset " << offset << ". Error: " << GetLastError() << endl;
                delete[] blockBuffer;
                // Skip this block and continue scanning
                offset += sectorSize;
                continue;
            }

            // Change the signature of the rotated block to "DONE"
            memcpy(blockBuffer, "DONE", 4);

            // Perform circular rotation
            BYTE* rotatedBuffer = new BYTE[blockLength * sectorSize];
            if (blockLength > 1) {
                memcpy(rotatedBuffer, blockBuffer + sectorSize, (blockLength - 1) * sectorSize); // Shift all sectors up
                memcpy(rotatedBuffer + (blockLength - 1) * sectorSize, blockBuffer, sectorSize); // Move first sector to the end
            }
            else {
                // If blockLength == 1, rotation is a no-op but still use the changed signature
                memcpy(rotatedBuffer, blockBuffer, sectorSize);
            }

            // Write the rotated block back to the image
            offsetLow32 = GetLower32Bits(offset);
            offsetHigh32 = GetUpper32Bits(offset);
            SetFilePointer(hFile, offsetLow32, &offsetHigh32, FILE_BEGIN);

            BOOL writeResult = WriteFile(hFile, rotatedBuffer, blockLength * sectorSize, &bytesWritten, NULL);
            if (!writeResult || bytesWritten != blockLength * sectorSize) {
                cerr << "Failed to write rotated block at offset " << offset << ". Error: " << GetLastError() << endl;
                logFile << "Failed to write rotated block at offset " << offset << ". Error: " << GetLastError() << endl;
            }
            else {
                FlushFileBuffers(hFile); // Ensure data is flushed
                cout << "Successfully wrote rotated block to offset " << offset << " and marked it as 'DONE'" << endl << endl;
                logFile << "Successfully wrote rotated block to offset " << offset << " and marked it as 'DONE'" << endl << endl;
            }

            // Clean up
            ZeroMemory(buffer, sectorSize);
            delete[] blockBuffer;
            delete[] rotatedBuffer;

            // Skip to the end of the block
            offset += static_cast<unsigned long long>(blockLength) * sectorSize;
        }
        else {
            // Not a USBC sector -- move to the next sector
            offset += sectorSize;
        }

        // Move the file pointer to the next offset explicitly
        offsetLow32 = GetLower32Bits(offset);
        offsetHigh32 = GetUpper32Bits(offset);
        SetFilePointer(hFile, offsetLow32, &offsetHigh32, FILE_BEGIN);
    }

    if (GetLastError() != ERROR_HANDLE_EOF) {
        cerr << "Error reading image. Error: " << GetLastError() << endl;
        logFile << "Error reading image: " << GetLastError() << endl;
    }

    delete[] buffer;
    CloseHandle(hFile);
}





// function to process the disk directly
void ProcessDisk(const string& diskPath, ofstream& logFile) {
    wstring wideDiskPath = wstring(diskPath.begin(), diskPath.end());

    HANDLE hDisk = CreateFile(
        wideDiskPath.c_str(),                   // Physical disk path (e.g., "\\\\.\\PhysicalDrive0")
        GENERIC_READ | GENERIC_WRITE,           // Access mode
        FILE_SHARE_READ | FILE_SHARE_WRITE,     // Share mode
        NULL,                                   // Security attributes
        OPEN_EXISTING,                          // Open existing disk
        0,                                      // Flags and attributes
        NULL                                    // Template file
    );

    if (hDisk == INVALID_HANDLE_VALUE) {
        cerr << endl << "Failed to open disk. Error: " << GetLastError() << endl;
        logFile << endl << "Failed to open disk: " << diskPath << ". Error: " << GetLastError() << endl;
        return;
    }

    const DWORD sectorSize = 512; // Sector size
    BYTE* buffer = new BYTE[sectorSize];
    DWORD bytesRead, bytesWritten;

    unsigned long long offset = 0;
    long offsetLow32 = 0;
    long offsetHigh32 = 0;
    

    cout << endl << "Processing disk: " << diskPath << endl << endl;
    logFile << endl << "Processing disk: " << diskPath << endl << endl;


    // Get the total disk size to set a clear loop boundary
    LARGE_INTEGER diskSize;
    GetFileSizeEx(hDisk, &diskSize);


    while (offset < static_cast<unsigned long long>(diskSize.QuadPart)) {
    // while (ReadFile(hDisk, buffer, sectorSize, &bytesRead, NULL) && bytesRead > 0) {
        
		// Move the file pointer to the current offset
		offsetLow32 = GetLower32Bits(offset);
		offsetHigh32 = GetUpper32Bits(offset);
        SetFilePointer(hDisk, offsetLow32, &offsetHigh32, FILE_BEGIN);
    
        // Attempt to read the current sector
        if (!ReadFile(hDisk, buffer, sectorSize, &bytesRead, NULL) || bytesRead != sectorSize) {
            // Handle read error or end of file
            
            cerr << endl << "Failed to read sector at offset " << offset << ". " << endl << endl;
            logFile << endl << "Failed to read sector at offset " << offset << ". " << endl << endl;

            offset += sectorSize;
            break;
        }


        // Debugging: Log the first four bytes
        /*cout << "Bytes read from offset " << offset << "are " << hex
        << static_cast<int>(buffer[0]) << " "
        << static_cast<int>(buffer[1]) << " "
        << static_cast<int>(buffer[2]) << " "
        << static_cast<int>(buffer[3]) << endl;*/
        
        /*logFile << "Bytes read from offset " << offset << " are " << hex
        << static_cast<int>(buffer[0]) << " "
        << static_cast<int>(buffer[1]) << " "
        << static_cast<int>(buffer[2]) << " "
        << static_cast<int>(buffer[3]) << endl;*/


        /*cout << "Data read from offset " << offset << ": "; // debugging output for checking if we read the right data from each block
        for (DWORD i = 0; i < min(16, bytesRead); ++i) {
            printf("%02X ", buffer[i]);
        }
        cout << endl;*/



        // Check if the sector starts with "USBC"
        const BYTE expectedSignature[4] = { 'U', 'S', 'B', 'C' };
        if (memcmp(buffer, expectedSignature, 4) == 0) {

            cout << "Found 'USBC' at offset: " << offset << " bytes" << endl;

            logFile << "[" << CurrentTime() << "]" << endl 
                << "Found 'USBC' at offset: " << offset << " bytes" << endl;

            // Determine the length of the USBC block
            unsigned int blockLength = findDiskSectorsAmount(buffer);

            /*if (blockLength == 0 || blockLength == 1) {
                cerr << "Invalid block length at offset " << offset << ". Skipping..." << endl << endl;
                offset += sectorSize;
                continue;
            }*/

            cout << "USBC block length: " << blockLength << " sectors" << endl;
            logFile << "USBC block length: " << blockLength << " sectors" << endl;


            if (blockLength == 0 || blockLength == 1) {
                cerr << "Invalid block length at offset " << offset << ". Skipping..." << endl << endl;
                logFile << "Invalid block length at offset " << offset << ". Skipping..." << endl << endl;
                offset += sectorSize;
                continue;
            }


            // Read the entire block into memory
            BYTE* blockBuffer = new BYTE[blockLength * sectorSize];


            offsetLow32 = GetLower32Bits(offset);
            offsetHigh32 = GetUpper32Bits(offset);
            SetFilePointer(hDisk, offsetLow32, &offsetHigh32, FILE_BEGIN);


            if (!ReadFile(hDisk, blockBuffer, blockLength * sectorSize, &bytesRead, NULL) || bytesRead != blockLength * sectorSize) {
                cout << "Failed to read USBC block. Error: " << GetLastError() << endl;
                cerr << "Failed to read USBC block. Error: " << GetLastError() << endl;
                logFile << "Failed to read USBC block at offset " << offset << ". Error: " << GetLastError() << endl;
                delete[] blockBuffer;
                continue;
            }

            // Change the signature of the rotated block to "DONE"
            memcpy(blockBuffer, "DONE", 4);

            // Perform circular rotation
            BYTE* rotatedBuffer = new BYTE[blockLength * sectorSize];
            memcpy(rotatedBuffer, blockBuffer + sectorSize, (blockLength - 1) * sectorSize); // Shift all sectors up
            memcpy(rotatedBuffer + (blockLength - 1) * sectorSize, blockBuffer, sectorSize); // Move first sector to the end


            // Output the rotated block to the console
            /*cout << "Rotated data (first 16 bytes): ";
            for (DWORD i = 0; i < min(16, blockLength * sectorSize); ++i) {
                printf("%02X ", rotatedBuffer[i]);
            }
            cout << endl;*/



            // Write the rotated block back to the disk
            offsetLow32 = GetLower32Bits(offset);
            offsetHigh32 = GetUpper32Bits(offset);
            SetFilePointer(hDisk, offsetLow32, &offsetHigh32, FILE_BEGIN);


            BOOL writeResult = WriteFile(hDisk, rotatedBuffer, blockLength * sectorSize, &bytesWritten, NULL);
            if (!writeResult || bytesWritten != blockLength * sectorSize) {
                cerr << "Failed to write rotated block at offset " << offset << ". Error: " << GetLastError() << endl;
                logFile << "Failed to write rotated block at offset " << offset << ". Error: " << GetLastError() << endl;
            }
            else {
                FlushFileBuffers(hDisk); // Ensure data is written
                cout << "Successfully wrote rotated block to offset " << offset << " and marked it as 'DONE'" << endl << endl;
                logFile << "Successfully wrote rotated block to offset " << offset << " and marked it as 'DONE'" << endl << endl;
            }


            //        // Re-read Sectors to Verify Written Data
            //        BYTE* verifyBuffer = new BYTE[blockLength * sectorSize];
            // 
            //        offsetLow32 = GetLower32Bits(offset);
            //        offsetHigh32 = GetUpper32Bits(offset);
            //        SetFilePointer(hDisk, offsetLow32, &offsetHigh32, FILE_BEGIN);

            //        DWORD verifyBytesRead = 0;
            //        if (!ReadFile(hDisk, verifyBuffer, blockLength * sectorSize, &verifyBytesRead, NULL) ||
            //            verifyBytesRead != blockLength * sectorSize) {
            //            cerr << "Failed to re-read block for verification at offset " << offset << ". Error: " << GetLastError() << endl;
                        //logFile << "Failed to re-read block for verification at offset " << offset << ". Error: " << GetLastError() << endl;
            //        }
            //        else {
            //            if (memcmp(verifyBuffer, rotatedBuffer, blockLength * sectorSize) != 0) {
            //                cerr << "Verification failed: Data mismatch at offset " << offset << endl << endl;
                        //	logFile << "Verification failed: Data mismatch at offset " << offset << endl << endl;
            //            }
            //            else {
            //                cout << "Verification succeeded: Data matches at offset " << offset << endl << endl;
                        //	logFile << "Verification succeeded: Data matches at offset " << offset << endl << endl;
            //            }
            //        }
            //        delete[] verifyBuffer;


            // Clear the buffer
            ZeroMemory(buffer, sectorSize);

            delete[] blockBuffer;
            delete[] rotatedBuffer;

            // Skip to the end of the block
            offset += blockLength * sectorSize;
        }
        else {
            //cout << "No USBC signature at offset " << offset << ". Skipping sector." << endl;

            // Move to the next sector
            offset += sectorSize;
        }

        // Move the file pointer to the next sector
        offsetLow32 = GetLower32Bits(offset);
        offsetHigh32 = GetUpper32Bits(offset);
        SetFilePointer(hDisk, offsetLow32, &offsetHigh32, FILE_BEGIN);
    }

    if (GetLastError() != ERROR_HANDLE_EOF) {
        cerr << "Error reading disk. Error: " << GetLastError() << endl;
        logFile << "Error reading disk: " << GetLastError() << endl;
    }

    delete[] buffer;
    CloseHandle(hDisk);
}


void ProcessUserSelectedDisk(ofstream& logFile) {
    vector<string> disks = ListAvailableDisks();

    if (disks.empty()) {
        cout << "No physical drives found." << endl;
        logFile << "No physical drives found." << endl;
        return;
    }
    
    cout << "Available disks:" << endl;
    for (size_t i = 0; i < disks.size(); ++i) {
        cout << i << ". " << disks[i] << endl; // Display disk numbers starting from 0
    }

    int choice = -1;
    cout << endl << "Select a disk (0-" << disks.size() - 1 << "): ";
    cin >> choice;

    if (choice < 0 || choice >= static_cast<int>(disks.size())) {
        cout << "Invalid choice." << endl;
        logFile << "Invalid disk choice." << endl;
        return;
    }

    string selectedDisk = disks[choice];
    logFile << "Selected disk: " << selectedDisk << endl;
    ProcessDisk(selectedDisk, logFile);
}


int main() {
    string diskOrImg;
    
	// add time for logging output?

    ofstream logFile("usbc_rotator.log");

    // asking user is it a disk or an image of a disk so we woudl know how to treat pathed object
    cout << "Is it a [d]isk or an [i]mage?: ";
    cin >> diskOrImg;
    cout << endl;

	logFile << "Storage type: " << diskOrImg << endl;

    if (diskOrImg == "d" || diskOrImg == "D" || diskOrImg == "disc" || diskOrImg == "Disc" || diskOrImg == "DISC" || diskOrImg == "disk" || diskOrImg == "Disk" || diskOrImg == "DISK") {
		
		logFile << "Chosen to work with a disk." << endl;
		logFile << "Start time: " << CurrentTime() << endl;

        ProcessUserSelectedDisk(logFile);

		logFile << "Finished editing a disk." << endl;
		logFile << "End time: " << CurrentTime() << endl;
        if (logFile.is_open()) { logFile.close(); }

        return 0;
		getchar();
    }
    else if (diskOrImg == "i" || diskOrImg == "I" || diskOrImg == "img" || diskOrImg == "Img" || diskOrImg == "IMG" || diskOrImg == "image" || diskOrImg == "Image" || diskOrImg == "IMAGE") {
        ifstream pathFileInput, diskImageInput; 
        ofstream diskImageOutput;
        std::filesystem::path path; // ! is not available in file Path 
        string fileIsCorrect;

     
	    logFile << "Chosen to work with an image file." << endl;

        // opening the path file to get usbc-corrupted file address and name
        pathFileInput.open("Path.txt");

        if (pathFileInput.is_open()) {
            pathFileInput >> path;
            pathFileInput.close();
        }
        else {
            cout << "Path.txt not found in the program directory. Please check the file." << endl;
			logFile << "Path.txt not found in the program directory." << endl;
            if (logFile.is_open()) {logFile.close();}

            return 0;
			getchar();
        }


        // outputting path of the file we're going to work with so the user could doublecheck
        cout << endl << "Path in work: " << path << endl;
        cout << endl << "Is this the correct path? [y/n]: ";
        cin >> fileIsCorrect;
        cout << endl;


        if (fileIsCorrect == "y" || fileIsCorrect == "Y" || fileIsCorrect == "yes" || fileIsCorrect == "Yes" || fileIsCorrect == "YES") {
			logFile << "Working with image file at path: " << path << endl;
            logFile << "Start time: " << CurrentTime() << endl;


			ProcessImage(path.string(), logFile);


            /*diskImageInput.open(path, ios::binary | ios::in);
            diskImageOutput.open(path, ios::binary | ios::out | ios::in);

            WorkWithUsbcImage(diskImageInput, diskImageOutput, logFile);
            
            diskImageOutput.close();        
            diskImageInput.close();*/
            
			logFile << "Finished editing an image with path " << path << endl;
			logFile << "End time: " << CurrentTime() << endl;
            if (logFile.is_open()) { logFile.close(); }

            return 0;
            getchar();
        }
        else if (fileIsCorrect == "n" || fileIsCorrect == "N" || fileIsCorrect == "no" || fileIsCorrect == "No" || fileIsCorrect == "NO") {
            
			logFile << "Wrong path." << path << endl;
            if (logFile.is_open()) { logFile.close(); }
            
            return 0;
            getchar();
            //PostMessage(GetConsoleWindow(), WM_CLOSE, 0, 0);
        }

    }
    else
    {
        cerr << "Source format not supported";
		logFile << "Source format not supported" << endl;
        if (logFile.is_open()) { logFile.close(); }

        return 0;
		getchar();
    }

}