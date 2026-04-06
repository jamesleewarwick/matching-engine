#pragma once
#include <cstddef>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

class MMapWriter {
public:
    MMapWriter(const char* filename, size_t size) : capacity(size) {
        fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) return;
        ftruncate(fd, capacity);
        buffer = (char*)mmap(nullptr, capacity, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (buffer == MAP_FAILED) buffer = nullptr;
    }
    ~MMapWriter() {
        if (buffer) munmap(buffer, capacity);
        if (fd >= 0) close(fd);
    }
    bool write(const void* data, size_t size) {
        if (!buffer) return false;
        if (offset + size > capacity) return false;
        std::memcpy(buffer + offset, data, size);
        offset += size;
        return true;
    }
private:
    int fd = -1;
    char* buffer = nullptr;
    size_t capacity;
    size_t offset{0};
};
