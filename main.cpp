#include "memory_resource.hpp"
#include "pmr_queue.hpp"

#include <iostream>
#include <memory_resource>
#include <string_view>

struct Task {
    std::pmr::string title;
    int priority;
    double weight;
};

void demonstrate_int_queue(CustomBlockMemoryResource& resource) {
    PmrQueue<int> queue(&resource);
    for (int value = 0; value < 5; ++value) {
        queue.push(value);
    }

    std::cout << "Integer queue contents: ";
    for (int value : queue) {
        std::cout << value << " ";
    }
    std::cout << "\nFront element: " << queue.front() << "\n";

    queue.pop();
    std::cout << "After pop, new front: " << queue.front() << "\n";
}

void demonstrate_task_queue(CustomBlockMemoryResource& resource) {
    std::pmr::polymorphic_allocator<char> string_allocator(&resource);
    PmrQueue<Task> queue(&resource);

    queue.emplace(Task{std::pmr::string("Alpha", string_allocator), 1, 3.5});
    queue.emplace(Task{std::pmr::string("Beta", string_allocator), 2, 1.2});
    queue.emplace(Task{std::pmr::string("Gamma", string_allocator), 3, 4.8});

    std::cout << "\nTask queue contents:\n";
    for (const Task& task : queue) {
        std::cout << " - " << task.title << " (priority " << task.priority
                  << ", weight " << task.weight << ")\n";
    }
}

int main() {
    constexpr std::size_t buffer_size = 4096;
    CustomBlockMemoryResource resource(buffer_size);

    std::cout << "Demonstrating PMR queue with a fixed memory resource\n";
    demonstrate_int_queue(resource);
    demonstrate_task_queue(resource);
    std::cout << "\nDone.\n";
    return 0;
}
