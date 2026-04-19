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
        const auto reset = [this](A... args) {
            smart = S{ p, std::forward<A>(args)... };
        };

        if (p) {
            std::apply(reset, std::move(args));
        }
    }

    out_ptr_t(const out_ptr_t&) = delete;
    out_ptr_t(out_ptr_t&&) = delete;
    out_ptr_t& operator=(const out_ptr_t&) = delete;
    out_ptr_t& operator=(out_ptr_t&&) = delete;

    // NOLINTNEXTLINE(google-explicit-constructor): implicit by design
    operator P*() const noexcept { return pp; }
    // NOLINTNEXTLINE(google-explicit-constructor): implicit by design
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
        return out_ptr_t<S, P, A&&...>(smart_pointer, std::forward<A>(args)...);
    } else if constexpr (requires { typename S::pointer; }) {
        return out_ptr_t<S, typename S::pointer, A&&...>(
                smart_pointer, std::forward<A>(args)...
        );
    }
}

}  // namespace glug::backport

