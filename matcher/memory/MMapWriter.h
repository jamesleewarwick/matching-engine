#pragma once
#define NOMINMAX
#include <windows.h>
#include <cstddef>

class MMapWriter {
public:
    MMapWriter(const char* filename, size_t size);
    ~MMapWriter();

    bool write(const void* data, size_t size);

private:
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hMap = NULL;
    char* buffer = nullptr;

    size_t capacity;
    size_t offset{ 0 };
};