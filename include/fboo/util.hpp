#pragma once
#include <ostream>
#include <ranges>
#include <vector>

template <typename T>
concept hasToString = requires(T t) {
    { t.to_string() } -> std::same_as<std::string>;
};

template <hasToString T>
std::ostream &operator<<(std::ostream &os, const T &t) {
    return os << t.to_string();
}

template <class T>
std::ostream &operator<<(std::ostream &os, const std::vector<T> &l) {
    if (l.empty()) {
        return os;
    }

    os << l.front();
    for (const auto &i : l | std::views::drop(1)) {
        os << ", " << i;
    }
    return os;
}

template <class Subclass, std::ranges::range It>
auto extract_subclass(const It &orig) {
    return orig | std::views::filter([](const auto &e) {
               //return e.get_type() == Target::type;
               return dynamic_cast<const Subclass *>(&e);
           })
           | std::views::transform(
               [](const auto &e) { return dynamic_cast<const Subclass &>(e); });
}
