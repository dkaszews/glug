// Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
#include "glug/filesystem.hpp"

#include <algorithm>
#include <fstream>

namespace fs = std::filesystem;

namespace glug::filesystem {

// Allows adding helpers with private access without modifying header
class explorer_impl {
    public:
    using level = explorer::level;
    using storage = decltype(explorer::stack);

    static explorer::reference front(const storage& stack) noexcept;
    static void add_outer_filters(storage& stack, const fs::path& path);
    static void populate(storage& stack, const fs::path& path);
    static void recurse(storage& stack);
    static void filter_and_sort(storage& stack);
    static void next(storage& stack);
};

explorer::reference explorer_impl::front(const storage& stack) noexcept {
    return stack.back().entries.front();
}

static bool getline(std::istream& input, std::string& s) {
    if (!std::getline(input, s, '\n')) {
        return false;
    } else if (!s.empty() && s.back() == '\r') {
        s.pop_back();
    }
    return true;
}

static auto read_lines(const fs::path& path) {
    auto stream = std::ifstream{ path };
    auto lines = std::vector<std::string>{};
    auto line = std::string{};
    while (getline(stream, line)) {
        lines.push_back(std::move(line));
    }
    return lines;
}

static glob::filter make_filter(const fs::path& path) {
    auto globs = std::vector<glob::decomposition>{};
    auto lines = read_lines(path);
    for (const auto& line : lines) {
        globs.emplace_back(glob::decompose(line));
        if (globs.back().pattern.empty()) {
            globs.pop_back();
        }
    }
    return { globs, path };
}

static bool is_root(const fs::path& path) {
#ifdef UNIT_TEST
    if (path.parent_path() == fs::temp_directory_path()) {
        return true;
    }
#endif
    return !path.has_parent_path();
}

static std::vector<fs::path> gather_gitignores(const fs::path& path) {
    if (fs::is_directory(path / ".git")) {
        return {};
    }

    auto current = fs::absolute(path);
    auto result = std::vector<fs::path>{};

    while (!is_root(current)) {  // GCOVR_EXCL_LINE: duplicate branch in report
        current = current.parent_path();
        const auto gitignore = current / ".gitignore";
        if (fs::exists(gitignore)) {
            result.push_back(gitignore);
        }
        if (fs::is_directory(current / ".git")) {
            return result;
        }
    }
    return result;
}

void explorer_impl::add_outer_filters(storage& stack, const fs::path& path) {
    const auto gitignores = gather_gitignores(path);
    const auto make_storage = [](const auto& path) {
        return level{ make_filter(path), {} };
    };  // GCOVR_EXCL_LINE: Unknown exceptional path
    std::transform(
            gitignores.rbegin(),
            gitignores.rend(),
            std::back_inserter(stack),
            make_storage
    );
}

void explorer_impl::populate(storage& stack, const fs::path& path) {
    auto entries = std::deque<fs::directory_entry>{
        fs::directory_iterator{ path },
        fs::directory_iterator{},
    };
    if (entries.empty()) {
        return;
    }

    const auto is_gitignore = [](const auto& entry) {
        return entry.path().filename() == ".gitignore";
    };
    const auto gitignore
            = std::find_if(entries.begin(), entries.end(), is_gitignore);
    auto filter = gitignore != entries.end() ? make_filter(gitignore->path())
                                             : glob::filter{};
    // GCOVR_EXCL_START: Move cannot throw
    stack.push_back({ std::move(filter), std::move(entries) });
    // GCOVR_EXCL_STOP
    filter_and_sort(stack);
}  // GCOVR_EXCL_LINE: Unknown exceptional branch

void explorer_impl::filter_and_sort(storage& stack) {
    auto& entries = stack.back().entries;
    const auto predicate = [&stack](const auto& entry) {
        for (auto it = stack.crbegin(); it != stack.crend(); ++it) {
            // GCOVR_EXCL_START: Special file types not testable on all OS
            // `is_directory() || is_file()` returns value for symlink target
            if (entry.is_symlink()) {
                return true;
            } else if (!entry.is_directory() && !entry.is_regular_file()) {
                return true;
            }
            // GCOVR_EXCL_STOP

            if (entry.path().filename() == ".git") {
                return true;
            }

            switch (it->filter.is_ignored(entry)) {  // GCOVR_EXCL_LINE: no def
                case glob::decision::ignored:
                    return true;
                case glob::decision::included:
                    return false;
                case glob::decision::undecided:
                    break;
            }
        }
        return false;
    };
    entries.erase(
            std::remove_if(entries.begin(), entries.end(), predicate),
            entries.end()
    );
    if (entries.empty()) {
        stack.pop_back();
        return;
    }

    static constexpr auto files_first = [](const auto& lhs, const auto& rhs) {
        return lhs.is_directory() != rhs.is_directory() ? !lhs.is_directory()
                                                        : lhs < rhs;
    };
    std::sort(entries.begin(), entries.end(), files_first);
    recurse(stack);
}

void explorer_impl::recurse(storage& stack) {
    if (stack.empty() || !front(stack).is_directory()) {
        return;
    }

    const auto dir = front(stack).path();
    stack.back().entries.pop_front();
    populate(stack, dir);

    while (!stack.empty() && stack.back().entries.empty()) {
        stack.pop_back();
    }
    recurse(stack);
}

void explorer_impl::next(storage& stack) {
    stack.back().entries.pop_front();
    while (!stack.empty() && stack.back().entries.empty()) {
        stack.pop_back();
    }
    recurse(stack);
}

explorer::explorer(const std::filesystem::path& root) {
    explorer_impl::add_outer_filters(stack, root);
    explorer_impl::populate(stack, root);
}

explorer::reference explorer::operator*() const {
    return explorer_impl::front(stack);
}

explorer::pointer explorer::operator->() const { return &**this; }

explorer& explorer::operator++() {
    explorer_impl::next(stack);
    return *this;
}

explorer explorer::operator++(int) {
    auto copy = *this;
    ++(*this);
    return copy;
}  // GCOVR_EXCL_LINE: Unknown branch, probably missing nothrow RVO

bool operator==(const explorer& lhs, const explorer& rhs) noexcept {
    return lhs.stack == rhs.stack;
}

bool operator!=(const explorer& lhs, const explorer& rhs) noexcept {
    return !(lhs == rhs);
}

}  // namespace glug::filesystem

