//  Copyright 2023, University of Freiburg,
//                  Chair of Algorithms and Data Structures.
//  Author: Johannes Kalmbach <kalmbach@cs.uni-freiburg.de>

#ifndef QLEVER_TEST_UTIL_ALLOCATORTESTHELPERS_H
#define QLEVER_TEST_UTIL_ALLOCATORTESTHELPERS_H

#include "global/Id.h"
#include "util/Allocator.h"
#include "util/AllocatorWithLimit.h"
#include "util/MemorySize/MemorySize.h"

namespace ad_utility::testing {
// Create an (unlimited by default) allocator routed through the `qlever`
// allocator seam, so the whole test suite follows the selected backend.
inline qlever::Allocator<Id> makeAllocator(
    MemorySize memorySize = MemorySize::max()) {
  return qlever::makeAllocatorWithLimit<Id>(memorySize);
}
}  // namespace ad_utility::testing

#endif  // QLEVER_TEST_UTIL_ALLOCATORTESTHELPERS_H
