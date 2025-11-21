#pragma once

#include <cstddef>
#include <algorithm>
#include <memory_resource>
#include <new>
#include <stdexcept>
#include <vector>

class CustomBlockMemoryResource : public std::pmr::memory_resource {
public:
    explicit CustomBlockMemoryResource(std::size_t capacity_bytes, std::size_t buffer_alignment = 64)
        : capacity_(capacity_bytes), buffer_alignment_(buffer_alignment) {
        if (capacity_bytes == 0) {
            throw std::invalid_argument("Capacity must be greater than zero");
        }
        if ((buffer_alignment_ & (buffer_alignment_ - 1)) != 0) {
            throw std::invalid_argument("Alignment must be a power of two");
        }
        buffer_ = static_cast<std::byte*>(::operator new(capacity_bytes, std::align_val_t(buffer_alignment_)));
    }

    ~CustomBlockMemoryResource() override {
        ::operator delete(buffer_, std::align_val_t(buffer_alignment_));
    }

    std::size_t capacity() const noexcept { return capacity_; }

private:
    struct Block {
        std::size_t offset;
        std::size_t size;
    };

    std::size_t capacity_;
    std::size_t buffer_alignment_;
    std::byte* buffer_;
    std::vector<Block> blocks_;

    static std::size_t align_offset(std::size_t offset, std::size_t alignment) {
        if (alignment == 0) {
            return offset;
        }
        const std::size_t remainder = offset % alignment;
        return remainder == 0 ? offset : offset + (alignment - remainder);
    }

    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        if (bytes == 0) {
            bytes = 1;
        }
        const std::size_t required_alignment = alignment == 0 ? alignof(std::max_align_t) : alignment;
        if (required_alignment > buffer_alignment_) {
            throw std::bad_alloc();
        }

        std::size_t current_offset = 0;
        for (const auto& block : blocks_) {
            const std::size_t aligned_offset = align_offset(current_offset, required_alignment);
            if (aligned_offset + bytes <= block.offset) {
                return commit_block(aligned_offset, bytes);
            }
            current_offset = block.offset + block.size;
        }

        const std::size_t aligned_offset = align_offset(current_offset, required_alignment);
        if (aligned_offset + bytes > capacity_) {
            throw std::bad_alloc();
        }
        return commit_block(aligned_offset, bytes);
    }

    void do_deallocate(void* ptr, std::size_t, std::size_t) override {
        if (ptr == nullptr) {
            return;
        }

        const auto byte_ptr = static_cast<std::byte*>(ptr);
        if (byte_ptr < buffer_ || byte_ptr >= buffer_ + capacity_) {
            throw std::logic_error("Pointer does not belong to this resource");
        }

        const std::size_t offset = static_cast<std::size_t>(byte_ptr - buffer_);
        for (auto it = blocks_.begin(); it != blocks_.end(); ++it) {
            if (it->offset == offset) {
                blocks_.erase(it);
                return;
            }
        }
        throw std::logic_error("Attempt to deallocate unmanaged block");
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }

    void* commit_block(std::size_t offset, std::size_t size) {
        Block new_block{offset, size};
        const auto insert_pos = std::lower_bound(
            blocks_.begin(),
            blocks_.end(),
            new_block.offset,
            [](const Block& lhs, std::size_t rhs) { return lhs.offset < rhs; });
        blocks_.insert(insert_pos, new_block);
        return buffer_ + offset;
    }
};
