#include "MMapWriter.h"
#include <cstring>
#include <iostream>

MMapWriter::MMapWriter(const char* filename, size_t size)
    : capacity(size) {

    hFile = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE,
        0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    std::cout << "CreateFile: " << (hFile != INVALID_HANDLE_VALUE ? "OK" : "FAILED") << std::endl;
    if (hFile == INVALID_HANDLE_VALUE) return;

    hMap = CreateFileMappingA(hFile, NULL, PAGE_READWRITE, 0, (DWORD)capacity, NULL);
    std::cout << "CreateFileMapping: " << (hMap ? "OK" : "FAILED") << std::endl;
    if (!hMap) return;

    buffer = (char*)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, capacity);
    std::cout << "MapViewOfFile: " << (buffer ? "OK" : "FAILED") << std::endl;
}

MMapWriter::~MMapWriter() {
    if (buffer) UnmapViewOfFile(buffer);
    if (hMap) CloseHandle(hMap);
    if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
}

bool MMapWriter::write(const void* data, size_t size) {
    if (offset + size > capacity) return false;

    std::memcpy(buffer + offset, data, size);
    offset += size;
    return true;
}