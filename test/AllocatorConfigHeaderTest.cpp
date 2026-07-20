// Copyright 2026, QLever contributors.
//
// End-to-end test of the platform-config-header mechanism. This target is
// compiled with
//   -DQLEVER_ALLOCATOR_CONFIG_HEADER="QleverAllocatorConfigExample.h"
// (set in `test/CMakeLists.txt`). The example header selects the PMR backend
// and provides Hi/Lo memory-resource hooks, so this test verifies that:
//   * `qlever::Allocator` resolves to the PMR backend purely because the config
//     header defined `QLEVER_USE_PMR_ALLOCATOR` (no other flag is passed here),
//   * the Hi and Lo hooks yield distinct, usable resources, and
//   * Hi and Lo allocators can be constructed and used simultaneously.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <type_traits>

#include "util/Allocator.h"
#include "util/AllocatorPmr.h"

// The config header must have been included transitively by `util/Allocator.h`.
#ifndef QLEVER_USE_PMR_ALLOCATOR
#error \
    "The example config header should have defined QLEVER_USE_PMR_ALLOCATOR. " \
    "Is QLEVER_ALLOCATOR_CONFIG_HEADER set for this target?"
#endif

// The config header selected the PMR backend, so `qlever::Allocator<T>` must be
// the PMR allocator type.
TEST(AllocatorConfigHeader, SelectsPmrBackend) {
  static_assert(
      std::is_same_v<qlever::Allocator<int>,
                     ad_utility::PmrAllocatorWithLimit<int>>,
      "config header must select the PMR backend");
  SUCCEED();
}

// The Hi/Lo hooks defined by the config header return distinct, non-null pools.
TEST(AllocatorConfigHeader, HiAndLoResourcesAreDistinct) {
  std::pmr::memory_resource* hi = QLEVER_PLATFORM_HI_RESOURCE();
  std::pmr::memory_resource* lo = QLEVER_PLATFORM_LO_RESOURCE();
  ASSERT_NE(hi, nullptr);
  ASSERT_NE(lo, nullptr);
  EXPECT_NE(hi, lo);
}

// Hi and Lo allocators coexist and route allocations to their own resource.
TEST(AllocatorConfigHeader, HiAndLoAllocatorsCoexist) {
  auto hiAlloc =
      qlever::makeAllocatorFromResource<int>(QLEVER_PLATFORM_HI_RESOURCE());
  auto loAlloc =
      qlever::makeAllocatorFromResource<int>(QLEVER_PLATFORM_LO_RESOURCE());

  EXPECT_EQ(hiAlloc.resource(), QLEVER_PLATFORM_HI_RESOURCE());
  EXPECT_EQ(loAlloc.resource(), QLEVER_PLATFORM_LO_RESOURCE());
  EXPECT_NE(hiAlloc.resource(), loAlloc.resource());

  int* pHi = hiAlloc.allocate(16);
  int* pLo = loAlloc.allocate(16);
  ASSERT_NE(pHi, nullptr);
  ASSERT_NE(pLo, nullptr);
  hiAlloc.deallocate(pHi, 16);
  loAlloc.deallocate(pLo, 16);
}

// The documented default strategy resolves to one of the two pools.
TEST(AllocatorConfigHeader, DefaultStrategyIsUsable) {
  std::pmr::memory_resource* def = QLEVER_DEFAULT_MEMORY_STRATEGY;
  ASSERT_NE(def, nullptr);
  auto alloc = qlever::makeAllocatorFromResource<char>(def);
  char* p = alloc.allocate(8);
  ASSERT_NE(p, nullptr);
  alloc.deallocate(p, 8);
}
