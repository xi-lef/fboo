#pragma once
#include <map>
#include <memory>
#include <ostream>
#include <ranges>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "entity.hpp"
#include "event.hpp"

template <class T>
requires requires(T t) {
    { t.to_string() } -> std::same_as<std::string>;
}
std::ostream &operator<<(std::ostream &os, const T &t) {
    return os << t.to_string();
}

template <class T>
requires std::derived_from<T, Event> || std::derived_from<T, Entity>
std::ostream &operator<<(std::ostream &os, const T *t) {
    return os << *t;
}

// https://stackoverflow.com/a/51032862/9505725
template <class, template <class...> class>
inline constexpr bool is_specialization = false;
template <template <class...> class T, class... Args>
inline constexpr bool is_specialization<T<Args...>, T> = true;

template <typename T>
requires is_specialization<T, std::shared_ptr>
      || is_specialization<T, std::unique_ptr>
std::ostream &operator<<(std::ostream &os, const T &t) {
    return os << *t;
}

template <class It>
requires is_specialization<It, std::vector>
std::ostream &operator<<(std::ostream &os, const It &it) {
    if (it.empty()) {
        return os << "[]";
    }

    os << "[";
    os << it.front();
    for (const auto &i : it | std::views::drop(1)) {
        os << ", " << i;
    }
    os << "]";
    return os;
}

template <class M>
requires is_specialization<M, std::map>
      || is_specialization<M, std::unordered_map>
std::ostream &operator<<(std::ostream &os, const M &m) {
    if (m.empty()) {
        return os << "{}";
    }

    auto pr = [&](const auto &e) {
        const auto &[k, v] = e;
        os << k << ": " << v;
    };

    os << "{";
    pr(*m.begin());
    for (const auto &e : m | std::views::drop(1)) {
        os << ", ";
        pr(e);
    }
    os << "}";
    return os;
}

template <class S>
requires is_specialization<S, std::set>
      || is_specialization<S, std::unordered_set>
std::ostream &operator<<(std::ostream &os, const S &s) {
    if (s.empty()) {
        return os << "{}";
    }

    auto pr = [&](const auto &k) {
        os << "\"" << k << "\"";
    };

    os << "{";
    pr(*s.begin());
    for (const auto &k : s | std::views::drop(1)) {
        os << ", ";
        pr(k);
    }
    os << "}";
    return os;
}

template <class Subclass, std::ranges::range It>
requires is_specialization<typename It::value_type, std::shared_ptr>
      || is_specialization<typename It::value_type, std::unique_ptr>
auto extract_subclass(const It &it) {
    return it | std::views::transform([](const auto &e) {
               return dynamic_cast<const Subclass *>(e.get());
           })
           | std::views::filter([](const auto *e) { return e; });
}

template <class Subclass, std::ranges::range It>
requires std::derived_from<Subclass,
                           std::remove_pointer_t<typename It::value_type>>
auto extract_subclass(const It &it) {
    return it | std::views::transform([](const auto &e) {
               return dynamic_cast<const Subclass *>(e);
           })
           | std::views::filter([](const auto *e) { return e; });
}

template <class T, class U>
const T *cast(const U *u) {
    return dynamic_cast<const T *>(u);
}
template <class T>
constexpr auto cast_event = cast<T, Event>;
