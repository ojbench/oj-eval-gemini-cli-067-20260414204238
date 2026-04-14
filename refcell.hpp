#ifndef REFCELL_H
#define REFCELL_H

#include <iostream>
#include <optional>
#include <stdexcept>
#include <memory>

class RefCellError : public std::runtime_error {
public:
    explicit RefCellError(const std::string& message) : std::runtime_error(message) {}
    virtual ~RefCellError() = default;
};

class BorrowError : public RefCellError {
public:
    explicit BorrowError(const std::string& message) : RefCellError(message) {}
};

class BorrowMutError : public RefCellError {
public:
    explicit BorrowMutError(const std::string& message) : RefCellError(message) {}
};

class DestructionError : public RefCellError {
public:
    explicit DestructionError(const std::string& message) : RefCellError(message) {}
};

template <typename T>
class RefCell {
private:
    T value;
    std::shared_ptr<int> borrow_count;

public:
    class Ref;
    class RefMut;

    explicit RefCell(const T& initial_value) : value(initial_value), borrow_count(std::make_shared<int>(0)) {}
    explicit RefCell(T&& initial_value) : value(std::move(initial_value)), borrow_count(std::make_shared<int>(0)) {}

    RefCell(const RefCell&) = delete;
    RefCell& operator=(const RefCell&) = delete;
    RefCell(RefCell&&) = delete;
    RefCell& operator=(RefCell&&) = delete;

    Ref borrow() const {
        if (*borrow_count == -1) {
            throw BorrowError("BorrowError");
        }
        (*borrow_count)++;
        return Ref(&value, borrow_count);
    }

    std::optional<Ref> try_borrow() const {
        if (*borrow_count == -1) {
            return std::nullopt;
        }
        (*borrow_count)++;
        return Ref(&value, borrow_count);
    }

    RefMut borrow_mut() {
        if (*borrow_count != 0) {
            throw BorrowMutError("BorrowMutError");
        }
        *borrow_count = -1;
        return RefMut(&value, borrow_count);
    }

    std::optional<RefMut> try_borrow_mut() {
        if (*borrow_count != 0) {
            return std::nullopt;
        }
        *borrow_count = -1;
        return RefMut(&value, borrow_count);
    }

    class Ref {
    private:
        const T* ptr;
        std::shared_ptr<int> count;

    public:
        Ref() : ptr(nullptr), count(nullptr) {}

        Ref(const T* p, std::shared_ptr<int> c) : ptr(p), count(std::move(c)) {}

        ~Ref() {
            if (count) {
                (*count)--;
            }
        }

        const T& operator*() const { return *ptr; }
        const T* operator->() const { return ptr; }

        Ref(const Ref& other) : ptr(other.ptr), count(other.count) {
            if (count) {
                (*count)++;
            }
        }

        Ref& operator=(const Ref& other) {
            if (this == &other) return *this;
            if (count) {
                (*count)--;
            }
            ptr = other.ptr;
            count = other.count;
            if (count) {
                (*count)++;
            }
            return *this;
        }

        Ref(Ref&& other) noexcept : ptr(other.ptr), count(std::move(other.count)) {
            other.ptr = nullptr;
        }

        Ref& operator=(Ref&& other) noexcept {
            if (this == &other) return *this;
            if (count) {
                (*count)--;
            }
            ptr = other.ptr;
            count = std::move(other.count);
            other.ptr = nullptr;
            return *this;
        }
    };

    class RefMut {
    private:
        T* ptr;
        std::shared_ptr<int> count;

    public:
        RefMut() : ptr(nullptr), count(nullptr) {}

        RefMut(T* p, std::shared_ptr<int> c) : ptr(p), count(std::move(c)) {}

        ~RefMut() {
            if (count) {
                *count = 0;
            }
        }

        T& operator*() { return *ptr; }
        T* operator->() { return ptr; }

        RefMut(const RefMut&) = delete;
        RefMut& operator=(const RefMut&) = delete;

        RefMut(RefMut&& other) noexcept : ptr(other.ptr), count(std::move(other.count)) {
            other.ptr = nullptr;
        }

        RefMut& operator=(RefMut&& other) noexcept {
            if (this == &other) return *this;
            if (count) {
                *count = 0;
            }
            ptr = other.ptr;
            count = std::move(other.count);
            other.ptr = nullptr;
            return *this;
        }
    };

    ~RefCell() noexcept(false) {
        if (borrow_count && *borrow_count != 0) {
            throw DestructionError("DestructionError");
        }
    }
};

#endif // REFCELL_H
