#ifndef QLEVER_SRC_UTIL_ALLOCATOR_AWARE_CONTAINERS_H
#define QLEVER_SRC_UTIL_ALLOCATOR_AWARE_CONTAINERS_H

#include "util/Allocator.h"

namespace qlever {

template <typename T>
using vector = std::vector<T, Allocator<T>>;

using string = std::basic_string<char, std::char_traits<char>, Allocator<char>>;

template <typename T, typename Compare = std::less<T>>
using set = std::set<T, Compare, Allocator<T>>;

template <typename K, typename V, typename Hash = std::hash<K>,
          typename KeyEqual = std::equal_to<K>>
using unordered_map =
    std::unordered_map<K, V, Hash, KeyEqual, Allocator<std::pair<const K, V>>>;

template <typename T, typename Hash = std::hash<T>,
          typename KeyEqual = std::equal_to<T>>
using unordered_set = std::unordered_set<T, Hash, KeyEqual, Allocator<T>>;

}  // namespace qlever

#endif  // QLEVER_SRC_UTIL_ALLOCATOR_AWARE_CONTAINERS_H
