#ifndef QLEVER_SRC_UTIL_ALLOCATOR_H
#define QLEVER_SRC_UTIL_ALLOCATOR_H

// QLever allocator seam.
//
// `qlever::Allocator<T>` and `qlever::makeAllocator<T>()` are the single point
// through which the engine's allocations are routed. The concrete allocator
// backend is selected at *compile time*:
//
//   * LIMIT (default): `ad_utility::AllocatorWithLimit<T>` - the historical
//     behaviour, a stateful allocator that enforces a global memory limit.
//   * PMR: a `std::pmr::memory_resource`-based allocator
//     (`ad_utility::PmrAllocatorWithLimit<T>`) that lets the target platform
//     inject its own memory pools (e.g. "Hi" / "Lo"). By default it keeps the
//     same memory-limit semantics via a `LimitedMemoryResource`.
//
// Selection:
//   * Define `QLEVER_USE_PMR_ALLOCATOR` (e.g. via the CMake option
//     `QLEVER_ALLOCATOR_BACKEND=pmr`) to select the PMR backend.
//   * Optionally point `QLEVER_ALLOCATOR_CONFIG_HEADER` at a platform header
//     (as a string usable in `#include`) which may itself define
//     `QLEVER_USE_PMR_ALLOCATOR` and platform Hi/Lo resource hooks. See
//     `util/AllocatorPlatformConfig.h` for the documented template.

// Optional platform configuration header. When defined it is included first so
// that a platform can drive the backend selection and provide its Hi/Lo memory
// resources.
#ifdef QLEVER_ALLOCATOR_CONFIG_HEADER
#include QLEVER_ALLOCATOR_CONFIG_HEADER
#endif

#ifdef QLEVER_USE_PMR_ALLOCATOR

#include "util/AllocatorPmr.h"

namespace qlever {

template <typename T>
using Allocator = ad_utility::PmrAllocatorWithLimit<T>;

// Unlimited allocator over new/delete (default). This is the factory used by
// the default `Qlever` construction path.
template <typename T>
Allocator<T> makeAllocator() {
  return ad_utility::makeUnlimitedPmrAllocator<T>();
}

// Create an allocator from a platform-provided `std::pmr::memory_resource`
// (e.g. a Hi or Lo pool). No limit is enforced by QLever; the resource owns the
// strategy.
template <typename T>
Allocator<T> makeAllocatorFromResource(std::pmr::memory_resource* resource) {
  return ad_utility::makePmrAllocatorFromResource<T>(resource);
}

}  // namespace qlever

#else  // LIMIT backend (default)

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

#endif  // QLEVER_USE_PMR_ALLOCATOR

#endif  // QLEVER_SRC_UTIL_ALLOCATOR_H
