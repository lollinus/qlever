// Copyright 2026, QLever contributors.
//
// ============================================================================
// TEMPLATE: QLever allocator platform configuration header.
// ============================================================================
//
// This file is a *documented template*. It is NOT included by the QLever build
// by default. A target platform integrating QLever provides its own copy of a
// header like this and points the build at it via the CMake option / compile
// define:
//
//     -DQLEVER_ALLOCATOR_CONFIG_HEADER="\"my_platform/qlever_alloc_config.h\""
//
// `util/Allocator.h` includes that header (as `#include
// QLEVER_ALLOCATOR_CONFIG_HEADER`) before it selects the backend, so this
// header can:
//   1. Select the PMR backend by defining `QLEVER_USE_PMR_ALLOCATOR`.
//   2. Provide hooks that return the platform's "Hi" and "Lo"
//      `std::pmr::memory_resource*` pools, and name the default strategy.
//
// The Hi/Lo memory resources themselves are implemented and owned by the
// platform (QLever ships only the seam and these hooks). Because
// `std::pmr::polymorphic_allocator` holds a *pointer* to a resource, Hi and Lo
// pools can be used simultaneously in the same process by injecting the desired
// resource per `Qlever` / `QueryExecutionContext` instance, e.g.:
//
//     qlever::Qlever engine{config,
//         qlever::makeAllocatorFromResource<Id>(QLEVER_PLATFORM_HI_RESOURCE())};
//
// ----------------------------------------------------------------------------

#ifndef QLEVER_SRC_UTIL_ALLOCATORPLATFORMCONFIG_H
#define QLEVER_SRC_UTIL_ALLOCATORPLATFORMCONFIG_H

// (1) Select the PMR backend on this platform.
#define QLEVER_USE_PMR_ALLOCATOR

#include <memory_resource>

// (2) Platform memory-allocation strategy hooks.
//
// Replace the bodies below with accessors for the platform's real Hi/Lo pools.
// The returned resources must outlive every `Qlever`/`QueryExecutionContext`
// that uses them (static storage duration is the simplest guarantee).

namespace qlever_platform {

// High-priority / high-performance memory pool.
inline std::pmr::memory_resource* hiResource() {
  // TODO(platform): return the platform's "Hi" memory resource.
  return std::pmr::new_delete_resource();
}

// Low-priority / spill memory pool.
inline std::pmr::memory_resource* loResource() {
  // TODO(platform): return the platform's "Lo" memory resource.
  return std::pmr::new_delete_resource();
}

}  // namespace qlever_platform

#define QLEVER_PLATFORM_HI_RESOURCE() ::qlever_platform::hiResource()
#define QLEVER_PLATFORM_LO_RESOURCE() ::qlever_platform::loResource()

// Default strategy used when the platform does not inject a resource explicitly.
// One of QLEVER_PLATFORM_HI_RESOURCE() / QLEVER_PLATFORM_LO_RESOURCE().
#define QLEVER_DEFAULT_MEMORY_STRATEGY QLEVER_PLATFORM_HI_RESOURCE()

#endif  // QLEVER_SRC_UTIL_ALLOCATORPLATFORMCONFIG_H
