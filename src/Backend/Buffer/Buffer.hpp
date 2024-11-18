
#pragma once

#include <memory>
#include <string>
#include <cstring>


namespace Infinity {
    struct Buffer {
        void *Data;
        uint64_t Size;

        Buffer() : Data(nullptr), Size(0) {
        }

        Buffer(const void *data, uint64_t size) : Data((void *) data), Size(size) {
        }

        Buffer(const Buffer &other, uint64_t size) : Data(other.Data), Size(size) {
        }

        static Buffer Copy(const Buffer &other) {
            Buffer buffer;
            buffer.Allocate(other.Size);
            memcpy(buffer.Data, other.Data, other.Size);
            return buffer;
        }

        static Buffer Copy(const void *data, uint64_t size) {
            Buffer buffer;
            buffer.Allocate(size);
            memcpy(buffer.Data, data, size);
            return buffer;
        }

        void Allocate(uint64_t size) {
            delete[] (uint8_t *) Data;
            Data = nullptr;

            if (size == 0)
                return;

            Data = new uint8_t[size];
            Size = size;
        }

        void Release() {
            delete[] (uint8_t *) Data;
            Data = nullptr;
            Size = 0;
        }

        void ZeroInitialize() const {
            if (Data)
                memset(Data, 0, Size);
        }

        template<typename T>
        T &Read(uint64_t offset = 0) {
            return *(T *) ((uint32_t *) Data + offset);
        }

        template<typename T>
        const T &Read(uint64_t offset = 0) const {
            return *(T *) ((uint32_t *) Data + offset);
        }

        [[nodiscard]] uint8_t *ReadBytes(uint64_t size, uint64_t offset) const {
            auto *buffer = new uint8_t[size];
            memcpy(buffer, (uint8_t *) Data + offset, size);
            return buffer;
        }

        void Write(const void *data, uint64_t size, uint64_t offset = 0) {
            memcpy((uint8_t *) Data + offset, data, size);
        }

        explicit operator bool() const { return Data; }

        uint8_t &operator[](int index) { return ((uint8_t *) Data)[index]; }

        uint8_t operator[](int index) const { return ((uint8_t *) Data)[index]; }

        template<typename T>
        T *As() const {
            return (T *) Data;
        }

        [[nodiscard]] inline uint64_t GetSize() const { return Size; }
    };
} // namespace InfinityRenderer
