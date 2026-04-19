// Provided as part of glug under MIT license, (c) 2025-2026 Dominik Kaszewski
#pragma once

#include <memory>
#include <string_view>

namespace glug::regex {

class engine {
    public:
    engine() = default;
    explicit engine(std::string_view pattern);

    [[nodiscard]] bool match(std::string_view s) const;
    [[nodiscard]] bool operator()(std::string_view s) const { return match(s); }

    static std::string_view license();

    private:
    struct impl;
    std::shared_ptr<impl> pimpl{};
};

};  // namespace glug::regex

