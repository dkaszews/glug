#pragma once

#include <stdexcept>
#include <string>
#include <typeinfo>
#include <utility>

namespace glug::unit_test {

class singleton_exception : public std::exception {
    public:
    explicit singleton_exception(const std::type_info& type) :
        type_name{ type.name() } {}

    const char* what() const noexcept override { return type_name.c_str(); }

    private:
    std::string type_name{};
};

template <typename T>
class singleton {
    public:
    singleton() {
        if (std::exchange(p, static_cast<T*>(this))) {
            throw singleton_exception{ typeid(T) };
        }
    }

    singleton(const singleton&) = delete;
    singleton(singleton&&) = delete;
    singleton& operator=(const singleton&) = delete;
    singleton& operator=(singleton&&) = delete;
    ~singleton() { p = nullptr; }

    static T& instance() {
        if (!p) {
            throw singleton_exception{ typeid(T) };
        }
        return *p;
    }

    private:
    inline static T* p = nullptr;
};

}  // namespace glug::unit_test

