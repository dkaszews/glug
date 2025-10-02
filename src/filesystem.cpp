#include "glug/filesystem.hpp"

#include <algorithm>
#include <fstream>

namespace glug::filesystem {

class explorer_impl {
    public:
    using storage = decltype(explorer::stack);

    static explorer::reference front(const storage& stack) noexcept;
    static void populate(storage& stack, const std::filesystem::path& path);
    static void recurse(storage& stack);
    static void filter_and_sort(storage& stack);
    static void next(storage& stack);
};

explorer::reference explorer_impl::front(const storage& stack) noexcept {
    return stack.back().entries.front();
}

static auto read_lines(const std::filesystem::path& path) {
    auto stream = std::ifstream{ path };
    auto lines = std::vector<std::string>{};
    auto line = std::string{};
    while (std::getline(stream, line)) {
        lines.push_back(std::move(line));
    }
    return lines;
}

static glob::filter make_filter(const std::filesystem::path& path) {
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

void explorer_impl::populate(
        storage& stack, const std::filesystem::path& path
) {
    auto entries = std::deque<std::filesystem::directory_entry>{
        std::filesystem::directory_iterator{ path },
        std::filesystem::directory_iterator{},
    };
    if (entries.empty()) {
        return;
    }

    const auto gitignore = std::find_if(
            entries.begin(), entries.end(), [](const auto& entry) {
                return entry.path().filename() == ".gitignore";
            }
    );
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
            // TODO: Better place to put it?
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

    if (front(stack).is_directory()) {
        recurse(stack);
    }
}

void explorer_impl::recurse(storage& stack) {
    const auto dir = front(stack).path();
    stack.back().entries.pop_front();
    populate(stack, dir);

    while (!stack.empty() && stack.back().entries.empty()) {
        stack.pop_back();
    }
}

void explorer_impl::next(storage& stack) {
    stack.back().entries.pop_front();
    while (!stack.empty() && stack.back().entries.empty()) {
        stack.pop_back();
    }

    if (!stack.empty() && front(stack).is_directory()) {
        recurse(stack);
    }
}

explorer::explorer(const std::filesystem::path& root) {
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

