// Provided as part of glug under MIT license, (c) 2026 Dominik Kaszewski
#pragma once

#include <memory>
#include <tuple>
#include <type_traits>

namespace glug::backport {

template <typename S, typename P, typename... A>
class out_ptr_t {
    public:
    explicit out_ptr_t(S& smart, A... args) :
        smart{ smart },
        args{ std::forward<A>(args)... },
        pp{ &p } {
        smart.reset();
    }

    ~out_ptr_t() {
        // auto reset = [this](auto&& args) {
        //     smart.reset(p, std::forward<A>(args)...);
        // };
        // if (p) {
        //     std::apply(reset, args);
        // }
        if (p) {
            smart.reset(p);
        }
    }

    out_ptr_t(const out_ptr_t&) = delete;
    out_ptr_t& operator=(const out_ptr_t&) = delete;

    operator P*() const noexcept { return pp; }
    operator void**() const noexcept { return pp; }

    private:
    S& smart;
    std::tuple<A...> args{};
    P p{};
    P* pp{};
};

template <typename P = void, typename S, typename... A>
auto out_ptr(S& smart_pointer, A&&... args) {
    if constexpr (!std::is_void_v<P>) {
        using T = P;
        return out_ptr_t<S, T, A&&...>(smart_pointer, std::forward<A>(args)...);
    } else if constexpr (requires { typename S::pointer; }) {
        using T = typename S::pointer;
        return out_ptr_t<S, T, A&&...>(smart_pointer, std::forward<A>(args)...);
    }
}

}  // namespace glug::backport

