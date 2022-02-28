#pragma once
#include <memory>
#include <ostream>
#include <ranges>
#include <vector>

template <class T>
requires requires(T t) {
    { t.to_string() } -> std::same_as<std::string>;
}
std::ostream &operator<<(std::ostream &os, const T &t) {
    return os << t.to_string();
}

template <typename T>
std::ostream &operator<<(std::ostream &os, const std::unique_ptr<T> &t) {
    return os << *t;
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
requires requires(It it) {
    { it[0].get() } -> std::convertible_to<void *>;
    // TODO is unique_ptr?
    //{ it.at(0) } -> std::same_as<std::unique_ptr<decltype(it.at(0).get())>>;
    //{ it.at(0) } -> std::convertible_to<std::unique_ptr<Subclass>>;
}
auto extract_subclass(const It &it) {
    return it | std::views::transform([](const auto &e) {
               return dynamic_cast<const Subclass *>(e.get());
           })
           | std::views::filter([](const auto &e) { return e; });
}
