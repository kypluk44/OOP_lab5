#pragma once

#include <cstddef>
#include <iterator>
#include <memory>
#include <memory_resource>
#include <stdexcept>
#include <utility>

// Queue container that uses std::pmr::polymorphic_allocator for memory management.
template <class T>
class PmrQueue {
private:
    struct Node {
        template <class... Args>
        explicit Node(Args&&... args) : value(std::forward<Args>(args)...), next(nullptr) {}
        T value;
        Node* next;
    };

    using allocator_type = std::pmr::polymorphic_allocator<Node>;

public:
    using value_type = T;
    class iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        iterator() = default;
        explicit iterator(Node* node) : node_(node) {}

        reference operator*() const { return node_->value; }
        pointer operator->() const { return std::addressof(node_->value); }

        iterator& operator++() {
            if (node_ != nullptr) {
                node_ = node_->next;
            }
            return *this;
        }

        iterator operator++(int) {
            iterator copy(*this);
            ++(*this);
            return copy;
        }

        friend bool operator==(const iterator& lhs, const iterator& rhs) {
            return lhs.node_ == rhs.node_;
        }

        friend bool operator!=(const iterator& lhs, const iterator& rhs) {
            return !(lhs == rhs);
        }

    private:
        friend class const_iterator;
        Node* node_{nullptr};
    };

    explicit PmrQueue(std::pmr::memory_resource* resource = std::pmr::get_default_resource())
        : allocator_(resource) {}

    PmrQueue(const PmrQueue&) = delete;
    PmrQueue& operator=(const PmrQueue&) = delete;

    PmrQueue(PmrQueue&& other) noexcept
        : allocator_(other.allocator_),
          head_(other.head_),
          tail_(other.tail_),
          size_(other.size_) {
        other.head_ = nullptr;
        other.tail_ = nullptr;
        other.size_ = 0;
    }

    PmrQueue& operator=(PmrQueue&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        destroy_all();
        allocator_ = other.allocator_;
        head_ = other.head_;
        tail_ = other.tail_;
        size_ = other.size_;
        other.head_ = nullptr;
        other.tail_ = nullptr;
        other.size_ = 0;
        return *this;
    }

    ~PmrQueue() {
        destroy_all();
    }

    template <class... Args>
    void emplace(Args&&... args) {
        Node* new_node = allocator_.allocate(1);
        std::allocator_traits<allocator_type>::construct(allocator_, new_node, std::forward<Args>(args)...);

        if (tail_ == nullptr) {
            head_ = tail_ = new_node;
        } else {
            tail_->next = new_node;
            tail_ = new_node;
        }
        ++size_;
    }

    void push(const T& value) { emplace(value); }
    void push(T&& value) { emplace(std::move(value)); }

    void pop() {
        if (empty()) {
            throw std::out_of_range("Queue is empty");
        }

        Node* old_head = head_;
        head_ = head_->next;
        if (head_ == nullptr) {
            tail_ = nullptr;
        }
        std::allocator_traits<allocator_type>::destroy(allocator_, old_head);
        allocator_.deallocate(old_head, 1);
        --size_;
    }

    T& front() {
        if (empty()) {
            throw std::out_of_range("Queue is empty");
        }
        return head_->value;
    }

    const T& front() const {
        if (empty()) {
            throw std::out_of_range("Queue is empty");
        }
        return head_->value;
    }

    bool empty() const noexcept { return head_ == nullptr; }

    iterator begin() noexcept { return iterator(head_); }
    iterator end() noexcept { return iterator(nullptr); }

private:
    allocator_type allocator_;
    Node* head_{nullptr};
    Node* tail_{nullptr};
    std::size_t size_{0};

    void destroy_all() noexcept {
        while (!empty()) {
            pop();
        }
    }
};
