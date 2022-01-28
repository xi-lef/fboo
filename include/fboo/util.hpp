#pragma once
#include <ostream>
#include <ranges>
#include <vector>

template<typename T>
concept hasToString = requires (T t) {
    { t.to_string() } -> std::same_as<std::string>;
};

template<hasToString T>
std::ostream& operator <<(std::ostream& os, const T& t) {
    return os << t.to_string();
}

template<class T>
std::ostream& operator <<(std::ostream& os, const std::vector<T>& l) {
    if (l.empty()) {
        return os;
    }

    os << l.front();
    for (const auto& i : l | std::views::drop(1)) {
        os << ", " << i;
    }
    return os;
}
