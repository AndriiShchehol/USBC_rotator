#include "general_utilities.h"
#include "usbc_sector_utilities.h"

#include <Windows.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>

using namespace std;

// Function to list available physical drives
vector<string> ListAvailableDisks() {
    vector<string> disks;
    for (int i = 0; i < 10; ++i) { // Check for drives 0-9
        string diskPath = "\\\\.\\PhysicalDrive" + to_string(i);
        wstring wideDiskPath = wstring(diskPath.begin(), diskPath.end()); // Convert to wide string for CreateFile function

        HANDLE diskHandle = CreateFile(
            wideDiskPath.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
        );

        if (diskHandle != INVALID_HANDLE_VALUE) {
            disks.push_back(diskPath);
            CloseHandle(diskHandle);
        }
    }
    return disks;
}

// Function to process an image of a disk
void ProcessImage(const string& imagePath, ofstream& logFile) {

    // Open the image file (regular file) for read/write using Win32 API
    wstring wideImagePath = wstring(imagePath.begin(), imagePath.end());

    HANDLE fileHandle = CreateFile(
        wideImagePath.c_str(),                  // Image file path
        GENERIC_READ | GENERIC_WRITE,           // Access mode
        FILE_SHARE_READ | FILE_SHARE_WRITE,     // Share mode
        NULL,                                   // Security attributes
        OPEN_EXISTING,                          // Open existing file
        FILE_ATTRIBUTE_NORMAL,                  // Flags and attributes for a normal file
        NULL                                    // Template file
    );

    if (fileHandle == INVALID_HANDLE_VALUE) {
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
    if (!GetFileSizeEx(fileHandle, &imageSize)) {
        cerr << "Failed to get image size. Error: " << GetLastError() << endl;
        logFile << "Failed to get image size for: " << imagePath << ". Error: " << GetLastError() << endl;
        delete[] buffer;
        CloseHandle(fileHandle);
        return;
    }

    while (offset < static_cast<unsigned long long>(imageSize.QuadPart)) {

        // Position file pointer
        offsetLow32 = GetLower32Bits(offset);
        offsetHigh32 = GetUpper32Bits(offset);
        SetFilePointer(fileHandle, offsetLow32, &offsetHigh32, FILE_BEGIN);

        // Read a single sector
        if (!ReadFile(fileHandle, buffer, sectorSize, &bytesRead, NULL) || bytesRead != sectorSize) {
            // Read error or EOF
            if (GetLastError() != ERROR_HANDLE_EOF) {
                cerr << endl << "Failed to read sector at offset " << offset << ". Error: " << GetLastError() << endl << endl;
                logFile << endl << "Failed to read sector at offset " << offset << ". Error: " << GetLastError() << endl << endl;
            }
            // Move on (break to avoid infinite loop) - advance by sector to attempt continuation
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
            unsigned int blockLength = FindDiskSectorsAmount(buffer);


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
            SetFilePointer(fileHandle, offsetLow32, &offsetHigh32, FILE_BEGIN);

            // In case of read error, skip this block and continue scanning
            if (!ReadFile(fileHandle, blockBuffer, blockLength * sectorSize, &bytesRead, NULL) || bytesRead != blockLength * sectorSize) {
                cerr << "Failed to read USBC block. Error: " << GetLastError() << endl;
                logFile << "Failed to read USBC block at offset " << offset << ". Error: " << GetLastError() << endl;
                delete[] blockBuffer;

                offset += sectorSize;
                continue;
            }

            // Change the signature of the rotated block to "DONE" so we will not rotate it again in case of crashes or errors
            memcpy(blockBuffer, "DONE", 4);

            // Perform circular rotation
            BYTE* rotatedBuffer = new BYTE[blockLength * sectorSize];
            if (blockLength > 1) {
                memcpy(rotatedBuffer, blockBuffer + sectorSize, (blockLength - 1) * sectorSize); // Shift all sectors up
                memcpy(rotatedBuffer + (blockLength - 1) * sectorSize, blockBuffer, sectorSize); // Move first sector to the end
            }
            else {
                // If blockLength == 1 sector, rotation is not needed, so we just copy the original block
                memcpy(rotatedBuffer, blockBuffer, sectorSize);
            }

            // Write the rotated block back to the image
            offsetLow32 = GetLower32Bits(offset);
            offsetHigh32 = GetUpper32Bits(offset);
            SetFilePointer(fileHandle, offsetLow32, &offsetHigh32, FILE_BEGIN);

            BOOL writeResult = WriteFile(fileHandle, rotatedBuffer, blockLength * sectorSize, &bytesWritten, NULL);
            if (!writeResult || bytesWritten != blockLength * sectorSize) {
                cerr << "Failed to write rotated block at offset " << offset << ". Error: " << GetLastError() << endl;
                logFile << "Failed to write rotated block at offset " << offset << ". Error: " << GetLastError() << endl;
            }
            else {
                FlushFileBuffers(fileHandle); // Ensure data is flushed
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
        SetFilePointer(fileHandle, offsetLow32, &offsetHigh32, FILE_BEGIN);
    }

    // Check for errors after the loop (e.g., if we exited due to an error rather than EOF)
    if (GetLastError() != ERROR_HANDLE_EOF && GetLastError() != 0) {
        cerr << "Error reading image. Error: " << GetLastError() << endl;
        logFile << "Error reading image. Error: " << GetLastError() << endl;
    }

    delete[] buffer;
    CloseHandle(fileHandle);
}


// Function to process the physical disk directly
void ProcessDisk(const string& diskPath, ofstream& logFile) {
    wstring wideDiskPath = wstring(diskPath.begin(), diskPath.end());

    HANDLE diskHandle = CreateFile(
        wideDiskPath.c_str(),                   // Physical disk path (e.g., "\\\\.\\PhysicalDrive0")
        GENERIC_READ | GENERIC_WRITE,           // Access mode
        FILE_SHARE_READ | FILE_SHARE_WRITE,     // Share mode
        NULL,                                   // Security attributes
        OPEN_EXISTING,                          // Open existing disk
        0,                                      // Flags and attributes
        NULL                                    // Template file
    );

    if (diskHandle == INVALID_HANDLE_VALUE) {
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
    GetFileSizeEx(diskHandle, &diskSize);


    while (offset < static_cast<unsigned long long>(diskSize.QuadPart)) {

        // Move the file pointer to the current offset
        offsetLow32 = GetLower32Bits(offset);
        offsetHigh32 = GetUpper32Bits(offset);
        SetFilePointer(diskHandle, offsetLow32, &offsetHigh32, FILE_BEGIN);

        // Attempt to read the current sector
        if (!ReadFile(diskHandle, buffer, sectorSize, &bytesRead, NULL) || bytesRead != sectorSize) {
            // Read error or EOF        
            cerr << endl << "Failed to read sector at offset " << offset << ". " << endl << endl;
            logFile << endl << "Failed to read sector at offset " << offset << ". " << endl << endl;

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
            unsigned int blockLength = FindDiskSectorsAmount(buffer);

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
            SetFilePointer(diskHandle, offsetLow32, &offsetHigh32, FILE_BEGIN);


            if (!ReadFile(diskHandle, blockBuffer, blockLength * sectorSize, &bytesRead, NULL) || bytesRead != blockLength * sectorSize) {
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

            // Write the rotated block back to the disk
            offsetLow32 = GetLower32Bits(offset);
            offsetHigh32 = GetUpper32Bits(offset);
            SetFilePointer(diskHandle, offsetLow32, &offsetHigh32, FILE_BEGIN);


            BOOL writeResult = WriteFile(diskHandle, rotatedBuffer, blockLength * sectorSize, &bytesWritten, NULL);
            if (!writeResult || bytesWritten != blockLength * sectorSize) {
                cerr << "Failed to write rotated block at offset " << offset << ". Error: " << GetLastError() << endl;
                logFile << "Failed to write rotated block at offset " << offset << ". Error: " << GetLastError() << endl;
            }
            else {
                FlushFileBuffers(diskHandle); // Ensure data is written
                cout << "Successfully wrote rotated block to offset " << offset << " and marked it as 'DONE'" << endl << endl;
                logFile << "Successfully wrote rotated block to offset " << offset << " and marked it as 'DONE'" << endl << endl;
            }

            // Clear the buffer
            ZeroMemory(buffer, sectorSize);

            delete[] blockBuffer;
            delete[] rotatedBuffer;

            // Skip to the end of the block
            offset += blockLength * sectorSize;
        }
        else {

            // Move to the next sector
            offset += sectorSize;
        }

        // Move the file pointer to the next sector
        offsetLow32 = GetLower32Bits(offset);
        offsetHigh32 = GetUpper32Bits(offset);
        SetFilePointer(diskHandle, offsetLow32, &offsetHigh32, FILE_BEGIN);
    }

    if (GetLastError() != ERROR_HANDLE_EOF && GetLastError() != 0) {
        cerr << "Error reading disk. Error: " << GetLastError() << endl;
        logFile << "Error reading disk. Error: " << GetLastError() << endl;
    }

    delete[] buffer;
    CloseHandle(diskHandle);
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

    ofstream logFile("usbc_rotator.log");

    // Asking user is it a disk or an image file so we will know how to work with it and if we need to ask for the path or not
    cout << "Is it a [d]isk or an [i]mage?: ";
    cin >> diskOrImg;
    cout << endl;


    // Different input options of "disk" for user convenience, all of them will be treated as the same answer
    if (diskOrImg == "d" || diskOrImg == "D" || diskOrImg == "disc" || diskOrImg == "Disc" || diskOrImg == "DISC" || diskOrImg == "disk" || diskOrImg == "Disk" || diskOrImg == "DISK") {

        logFile << "Storage type: Disk" << endl;

        logFile << "Chosen to work with a disk." << endl;
        logFile << "Start time: " << CurrentTime() << endl;

        ProcessUserSelectedDisk(logFile);

        logFile << "Finished editing a disk." << endl;
        logFile << "End time: " << CurrentTime() << endl;
        if (logFile.is_open()) { logFile.close(); }

        system("pause");
        return 0;
    } // Different input options of "image" for user convenience, all of them will be treated as the same answer
    else if (diskOrImg == "i" || diskOrImg == "I" || diskOrImg == "img" || diskOrImg == "Img" || diskOrImg == "IMG" || diskOrImg == "image" || diskOrImg == "Image" || diskOrImg == "IMAGE") {

        logFile << "Storage type: Image" << endl;

        ifstream pathFileInput, diskImageInput;
        ofstream diskImageOutput;
        filesystem::path path; // IMPORTANT NOTE: "!" symbol is not available inside of path type variable 
        string fileIsCorrect;


        logFile << "Chosen to work with an image file." << endl;

        // Opening the path file to get usbc-corrupted file address and name
        pathFileInput.open("Path.txt");

        if (pathFileInput.is_open()) {
            pathFileInput >> path;
            pathFileInput.close();
        }
        else {
            cout << "Path.txt not found in the program directory. Please check the file." << endl;
            logFile << "Path.txt not found in the program directory." << endl;
            if (logFile.is_open()) { logFile.close(); }

            system("pause");
            return 0;
        }


        // Outputting path of the file we're going to work with so the user could doublecheck
        cout << endl << "Path in work: " << path << endl;
        cout << endl << "Is this the correct path? [y/n]: ";
        cin >> fileIsCorrect;
        cout << endl;


        if (fileIsCorrect == "y" || fileIsCorrect == "Y" || fileIsCorrect == "yes" || fileIsCorrect == "Yes" || fileIsCorrect == "YES") {
            logFile << "Working with image file at path: " << path << endl;
            logFile << "Start time: " << CurrentTime() << endl;


            ProcessImage(path.string(), logFile);

            logFile << "Finished editing an image with path " << path << endl;
            logFile << "End time: " << CurrentTime() << endl;
            if (logFile.is_open()) { logFile.close(); }

            system("pause");
            return 0;
        }
        else if (fileIsCorrect == "n" || fileIsCorrect == "N" || fileIsCorrect == "no" || fileIsCorrect == "No" || fileIsCorrect == "NO") {

            logFile << "Wrong path." << path << endl;
            if (logFile.is_open()) { logFile.close(); }

            system("pause");
            return 0;
        }

    } // If user input does not match any of the options, we will output an error message and exit the program
    else {
        cerr << "Source format not supported";
        logFile << "Source format not supported" << endl;
        if (logFile.is_open()) { logFile.close(); }

        system("pause");
        return 0;
    }

}
