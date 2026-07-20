// Copyright 2026, QLever contributors.
//
// ============================================================================
// EXAMPLE platform allocator configuration header.
// ============================================================================
//
// This is a *working example* of the platform configuration header described in
// `src/util/AllocatorPlatformConfig.h`. It demonstrates how a target platform
// plugs its own memory-allocation strategy into QLever without touching engine
// code, by:
//
//   1. Selecting the PMR allocator backend (`QLEVER_USE_PMR_ALLOCATOR`).
//   2. Providing "Hi" and "Lo" `std::pmr::memory_resource` pools and exposing
//      them through the `QLEVER_PLATFORM_HI_RESOURCE()` /
//      `QLEVER_PLATFORM_LO_RESOURCE()` hooks.
//
// It is wired into the build via
//   -DQLEVER_ALLOCATOR_CONFIG_HEADER="QleverAllocatorConfigExample.h"
// (see the `AllocatorConfigHeaderTest` target in `test/CMakeLists.txt`).
//
// The two pools model a typical "high / low priority" strategy:
//   * Hi: a pooling resource optimised for fast reuse of many small blocks
//         (here: `std::pmr::synchronized_pool_resource`, thread-safe because
//         QLever allocates from multiple threads).
//   * Lo: an unbounded fallback backed directly by new/delete.
//
// A real platform would replace the bodies below with accessors for its actual
// hardware / OS memory regions. The returned resources have static storage
// duration so they outlive every `Qlever` / `QueryExecutionContext` that uses
// them.

#ifndef QLEVER_EXAMPLES_ALLOCATOR_QLEVERALLOCATORCONFIGEXAMPLE_H
#define QLEVER_EXAMPLES_ALLOCATOR_QLEVERALLOCATORCONFIGEXAMPLE_H

// (1) Select the PMR backend on this platform.
#define QLEVER_USE_PMR_ALLOCATOR

#include <memory_resource>

namespace qlever_platform_example {

// High-priority pool: pooling resource for fast small-block reuse.
inline std::pmr::memory_resource* hiResource() {
  static std::pmr::synchronized_pool_resource pool{};
  return &pool;
}

// Low-priority pool: unbounded new/delete fallback.
inline std::pmr::memory_resource* loResource() {
  return std::pmr::new_delete_resource();
}

}  // namespace qlever_platform_example

// (2) Strategy hooks consumed by the platform integration.
#define QLEVER_PLATFORM_HI_RESOURCE() ::qlever_platform_example::hiResource()
#define QLEVER_PLATFORM_LO_RESOURCE() ::qlever_platform_example::loResource()

// Default strategy when no resource is injected explicitly.
#define QLEVER_DEFAULT_MEMORY_STRATEGY QLEVER_PLATFORM_HI_RESOURCE()

#endif  // QLEVER_EXAMPLES_ALLOCATOR_QLEVERALLOCATORCONFIGEXAMPLE_H
