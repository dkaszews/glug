// Provided as part of glug under MIT license, (c) 2025-2026 Dominik Kaszewski
#include "glug/filesystem.hpp"

#include "glug/filter.hpp"
#include "glug/glob.hpp"

#include <algorithm>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace glug::filesystem {

// Allows adding helpers with private access without modifying header
class explorer_impl {
    public:
    // Transient helper, storing references to avoid passing to all functions
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
    decltype(explorer::stack)& stack;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
    const explorer_options& options;

    // `const` methods cannot construct instance without dropping qualifiers
    [[nodiscard]] static const auto&
    front(const std::vector<explorer::level>& stack) noexcept {
        return stack.back().entries.front();
    }
    [[nodiscard]] const auto& front() const noexcept { return front(stack); }
    void add_outer_filters(const fs::path& path);
    void populate(const fs::path& path);
    void recurse();
    [[nodiscard]] bool filter_entry(const fs::directory_entry& entry) const;
    void filter_and_sort();
    void next();
};

namespace {

auto getline(std::ifstream& input, std::string& s) {
    if (!std::getline(input, s, '\n')) {
        return false;
    }
    if (!s.empty() && s.back() == '\r') {
        s.pop_back();
    }
    return true;
}

auto read_lines(const fs::path& path) {
    auto stream = std::ifstream{ path };
    auto lines = std::vector<std::string>{};
    auto line = std::string{};
    while (getline(stream, line)) {
        lines.emplace_back(std::move(line));
    }
    return lines;
}

filter::ignore make_filter(const fs::path& path) {
    auto globs = std::vector<glob::decomposition>{};
    auto lines = read_lines(path);
    for (const auto& line : lines) {
        globs.emplace_back(glob::decompose(line));
        if (globs.back().pattern.empty()) {
            globs.pop_back();
        }
    }
    return { globs, path.parent_path() };
}

auto is_root(const fs::path& path) {
#if defined(UNIT_TEST)
    if (path.parent_path() == fs::canonical(fs::temp_directory_path())) {
        return true;
    }
#endif
    // GCOVR_EXCL_START: Spurious missing branch on Windows
    return path == path.parent_path();
    // GCOVR_EXCL_STOP
}

}  // namespace

void explorer_impl::add_outer_filters(const fs::path& path) {
    if (fs::is_directory(path / ".git")) {
        return;
    }

    auto current = fs::canonical(path);
    while (!is_root(current)) {
        current = current.parent_path();
        const auto gitignore = current / ".gitignore";
        const auto has_gitignore = fs::is_regular_file(gitignore);
        const auto is_root = fs::is_directory(current / ".git");
        if (!has_gitignore && !is_root) {
            continue;
        }

        auto filter = has_gitignore ? make_filter(gitignore) : filter::ignore{};
        stack.emplace_back(
                std::move(filter),
                std::deque<std::filesystem::directory_entry>{},
                is_root
        );
        if (is_root) {
            break;
        }
    }
    std::ranges::reverse(stack);
};

void explorer_impl::populate(const fs::path& path) {
    auto entries = std::deque<fs::directory_entry>{
        fs::directory_iterator{ path },
        fs::directory_iterator{},
    };
    if (entries.empty()) {
        return;
    }

    const auto is_named = [](const auto& filename) {
        return [&filename](const auto& entry) {
            return entry.path().filename() == filename;
        };
    };
    const bool is_root = std::ranges::any_of(entries, is_named(".git"));
    const bool already_rooted = std::ranges::any_of(
            stack, std::mem_fn(&explorer::level::is_root)
    );
    if (is_root && already_rooted) {
        return;
    }

    const auto gitignore
            = std::ranges::find_if(entries, is_named(".gitignore"));
    auto filter = gitignore != entries.end() ? make_filter(gitignore->path())
                                             : filter::ignore{};
    stack.emplace_back(std::move(filter), std::move(entries), is_root);
    filter_and_sort();
}

bool explorer_impl::filter_entry(const fs::directory_entry& entry) const {
    // GCOVR_EXCL_START: Special file types not testable on all OS
    // `is_directory() || is_file()` returns value for symlink target
    if (entry.is_symlink()) {
        return true;
    }

    if (!entry.is_directory() && !entry.is_regular_file()) {
        return true;
    }
    // GCOVR_EXCL_STOP

    if (entry.path().filename() == ".git") {
        return true;
    }

    if (options.select(entry) == filter::decision::excluded) {
        return true;
    }

    for (const auto& level : std::ranges::reverse_view(stack)) {
        const auto decision = level.filter(entry);
        if (level.is_root || decision != filter::decision::undecided) {
            return decision == filter::decision::excluded;
        }
    }
    return false;
};

// TODO: Maybe can be removed?
// NOLINTNEXTLINE(readability-function-size): Nesting counts lambda as level
void explorer_impl::filter_and_sort() {
    auto& entries = stack.back().entries;
    std::erase_if(entries, [this](const auto& entry) {
        return filter_entry(entry);
    });
    if (entries.empty()) {
        stack.pop_back();
        return;
    }

    static constexpr auto files_first = [](const auto& lhs, const auto& rhs) {
        return lhs.is_directory() != rhs.is_directory() ? !lhs.is_directory()
                                                        : lhs < rhs;
    };
    std::ranges::sort(entries, files_first);
    recurse();
}

void explorer_impl::recurse() {
    if (stack.empty() || !front().is_directory()) {
        return;
    }

    const auto dir = front().path();
    stack.back().entries.pop_front();
    populate(dir);

    while (!stack.empty() && stack.back().entries.empty()) {
        stack.pop_back();
    }
    recurse();
}

void explorer_impl::next() {
    stack.back().entries.pop_front();
    while (!stack.empty() && stack.back().entries.empty()) {
        stack.pop_back();
    }
    recurse();
}

explorer::explorer(
        const std::filesystem::path& root, const explorer_options& options
) :
    options{ options } {
    auto impl = explorer_impl{ .stack = stack, .options = options };
    impl.add_outer_filters(root);
    impl.populate(root);
}

explorer::reference explorer::operator*() const {
    return explorer_impl::front(stack);
}

explorer::pointer explorer::operator->() const { return &**this; }

explorer& explorer::operator++() {
    explorer_impl{ .stack = stack, .options = options }.next();
    return *this;
}

explorer explorer::operator++(int) {
    auto copy = *this;
    ++(*this);
    return copy;
}  // GCOVR_EXCL_LINE: Unknown branch, probably missing nothrow RVO

bool explorer::operator==(const explorer& other) const noexcept {
    // Omit options, as filters are not comparable
    return stack == other.stack;
}

bool explorer::level::operator==(const level& other) const noexcept {
    // Don't compare filters as they are transient cache.
    // Identical entries must have encountered the same filters,
    // which could be found again from a set of entry parents.
    // `is_root` is also ignored as more of a filter property.
    return entries == other.entries;
}

}  // namespace glug::filesystem

