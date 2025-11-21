#include "memory_resource.hpp"
#include "pmr_queue.hpp"

#include <gtest/gtest.h>
#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <string>
#include <vector>

// Проверяет стандартный FIFO-порядок очереди.
TEST(PmrQueueTest, PreservesFifoOrder) {
    CustomBlockMemoryResource resource(512);
    PmrQueue<int> queue(&resource);

    queue.push(1);
    queue.push(2);
    queue.push(3);

    EXPECT_EQ(queue.front(), 1);
    queue.pop();
    EXPECT_EQ(queue.front(), 2);
    queue.pop();
    EXPECT_EQ(queue.front(), 3);
    queue.pop();
    EXPECT_TRUE(queue.empty());
}

// Проверяет, что итератор проходит элементы в порядке вставки.
TEST(PmrQueueTest, IteratesOverElements) {
    CustomBlockMemoryResource resource(512);
    PmrQueue<int> queue(&resource);
    queue.emplace(5);
    queue.emplace(6);
    queue.emplace(7);

    std::vector<int> collected;
    for (int value : queue) {
        collected.push_back(value);
    }

    std::vector<int> expected{5, 6, 7};
    EXPECT_EQ(collected, expected);
}

// Проверяет работу очереди с составными типами и PMR-строками.
TEST(PmrQueueTest, HandlesComplexTypes) {
    struct Record {
        std::pmr::string name;
        int count;
        double weight;
    };

    CustomBlockMemoryResource resource(2048);
    std::pmr::polymorphic_allocator<char> string_alloc(&resource);
    PmrQueue<Record> queue(&resource);

    queue.emplace(Record{std::pmr::string("Alpha", string_alloc), 10, 1.5});
    queue.emplace(Record{std::pmr::string("Beta", string_alloc), 12, 2.5});

    EXPECT_EQ(queue.front().name, "Alpha");
    queue.pop();
    EXPECT_EQ(queue.front().name, "Beta");
}

// Проверяет, что память ресурсом переиспользуется при push/pop циклах.
TEST(PmrQueueTest, ReusesFreedMemory) {
    CustomBlockMemoryResource resource(128);
    PmrQueue<int> queue(&resource);

    for (int cycle = 0; cycle < 20; ++cycle) {
        queue.push(cycle);
        queue.pop();
    }

    EXPECT_TRUE(queue.empty());
    queue.push(42);
    EXPECT_EQ(queue.front(), 42);
}

// Проверяет, что pop на пустой очереди выбрасывает исключение.
TEST(PmrQueueTest, PopOnEmptyThrows) {
    CustomBlockMemoryResource resource(64);
    PmrQueue<int> queue(&resource);
    EXPECT_THROW(queue.pop(), std::out_of_range);
}

// Проверяет, что front на пустой очереди выбрасывает исключение.
TEST(PmrQueueTest, FrontOnEmptyThrows) {
    CustomBlockMemoryResource resource(64);
    PmrQueue<int> queue(&resource);
    EXPECT_THROW(queue.front(), std::out_of_range);
}

// Проверяет уважение выравнивания при выделении.
TEST(FixedMemoryResourceTest, RespectsAlignment) {
    struct alignas(32) Aligned {
        std::byte data[32];
    };

    CustomBlockMemoryResource resource(512);
    std::pmr::polymorphic_allocator<Aligned> alloc(&resource);
    Aligned* p = alloc.allocate(1);
    auto address = reinterpret_cast<std::uintptr_t>(p);
    EXPECT_EQ(address % alignof(Aligned), 0u);
    alloc.deallocate(p, 1);
}

// Проверяет переиспользование освобожденного блока по адресу.
TEST(FixedMemoryResourceTest, ReusesSameOffset) {
    CustomBlockMemoryResource resource(128);
    std::pmr::polymorphic_allocator<std::byte> alloc(&resource);

    std::byte* first = alloc.allocate(16);
    std::byte* second = alloc.allocate(16);
    alloc.deallocate(first, 16);

    std::byte* reused = alloc.allocate(8);
    EXPECT_EQ(reused, first);  // должен занять первую дыру

    alloc.deallocate(second, 16);
    alloc.deallocate(reused, 8);
}

// Проверяет, что переполнение фиксированного буфера приводит к bad_alloc.
TEST(FixedMemoryResourceTest, ThrowsOnOverflow) {
    CustomBlockMemoryResource resource(32);
    std::pmr::polymorphic_allocator<std::byte> alloc(&resource);

    std::byte* a = alloc.allocate(16);
    std::byte* b = alloc.allocate(16);
    EXPECT_NE(a, nullptr);
    EXPECT_NE(b, nullptr);
    EXPECT_THROW(static_cast<void>(alloc.allocate(1)), std::bad_alloc);

    alloc.deallocate(a, 16);
    alloc.deallocate(b, 16);
}
