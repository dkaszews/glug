#pragma once

#include <memory>
#include <string_view>

namespace glug::regex {

namespace detail {
struct impl;
}

class engine {
    public:
    engine() = default;
    explicit engine(std::string_view pattern);

    bool match(std::string_view s) const;
    bool operator()(std::string_view s) const { return match(s); }

    private:
    std::shared_ptr<detail::impl> pimpl{};
};

};  // namespace glug::regex

