#pragma once

#include <functional>
#include <optional>
#include <unordered_map>
#include <utility>

template <typename K, typename I>
std::optional<I&> tryAt(const K& key, const std::unordered_map<K, I>& map) {
    auto it = map.find(key);
    if (it != map.end()) {
        return it->second;
    }
    return std::nullopt;
}

namespace std {
template <typename T1, typename T2>
struct hash<std::pair<T1, T2>> {
    std::size_t operator()(const std::pair<T1, T2>& p) const {
        return std::hash<T1>()(p.first) ^ (std::hash<T2>()(p.second) << 1);
    }
};
}  // namespace std