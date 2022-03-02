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
std::ostream &operator<<(std::ostream &os, const std::shared_ptr<T> &t) {
    return os << *t;
}

// https://stackoverflow.com/a/51032862/9505725
template <class, template <class...> class>
inline constexpr bool is_specialization = false;
template <template <class...> class T, class... Args>
inline constexpr bool is_specialization<T<Args...>, T> = true;

template <class T>
concept is_vector = is_specialization<T, std::vector>;

template <is_vector It>
std::ostream &operator<<(std::ostream &os, const It &it) {
    if (it.empty()) {
        return os;
    }

    os << it.front();
    for (const auto &i : it | std::views::drop(1)) {
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

template <class Subclass, std::ranges::range It>
requires requires(It it) {
    { it[0] } -> std::assignable_from<Subclass *>;
}
auto extract_subclass(const It &it) {
    return it | std::views::transform([](const auto *e) {
               return dynamic_cast<const Subclass *>(e);
           })
           | std::views::filter([](const auto *e) { return e; });
}
