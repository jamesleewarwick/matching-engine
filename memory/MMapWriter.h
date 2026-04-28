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
    ~MMapWriter() noexcept {
        if (buffer) munmap(buffer, capacity);
        if (fd >= 0) close(fd);
    }

    MMapWriter(const MMapWriter&) = delete;
    MMapWriter& operator=(const MMapWriter&) = delete;

    MMapWriter(MMapWriter&& other) noexcept
        : fd(other.fd), buffer(other.buffer), capacity(other.capacity), offset(other.offset) {
        other.fd = -1;
        other.buffer = nullptr;
        other.offset = 0;
    }

    MMapWriter& operator=(MMapWriter&& other) noexcept {
        if (this != &other) {
            if (buffer) munmap(buffer, capacity);
            if (fd >= 0) close(fd);
            fd = other.fd;
            buffer = other.buffer;
            capacity = other.capacity;
            offset = other.offset;
            other.fd = -1;
            other.buffer = nullptr;
            other.offset = 0;
        }
        return *this;
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
