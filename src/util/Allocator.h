#ifndef QLEVER_SRC_UTIL_ALLOCATOR_H
#define QLEVER_SRC_UTIL_ALLOCATOR_H

#include "util/AllocatorWithLimit.h"

namespace qlever {

template <typename T>
using Allocator = ad_utility::AllocatorWithLimit<T>;

template <typename T>
Allocator<T> makeAllocator() {
  return ad_utility::makeUnlimitedAllocator<T>();
}

template <typename T, typename... Args>
Allocator<T> makeAllocator(Args&&... args) {
  return ad_utility::AllocatorWithLimit<T>{std::forward<Args>(args)...};
}

}  // namespace qlever

#endif  // QLEVER_SRC_UTIL_ALLOCATOR_H
