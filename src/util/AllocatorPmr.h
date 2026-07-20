// Copyright 2026, QLever contributors.
// Author: Copilot (BMW allocator support)
//
// A `std::pmr`-based allocator backend for QLever. This mirrors the public
// surface of `ad_utility::AllocatorWithLimit` (see `AllocatorWithLimit.h`) so
// that it can be selected as a drop-in replacement behind the
// `qlever::Allocator<T>` seam (see `Allocator.h`).
//
// The design goals (see `foresight-notes/qlever-allocator`):
//   * Route all allocations through a `std::pmr::memory_resource` so the target
//     platform can inject its own memory pools (e.g. "Hi" / "Lo").
//   * Keep QLever's existing *memory-limit* semantics by default: a
//     `LimitedMemoryResource` counts bytes and throws when a configured limit
//     is exceeded, exactly like `AllocatorWithLimit`.
//   * Allow the platform to instead supply a plain upstream resource with no
//     limit enforcement.

#ifndef QLEVER_SRC_UTIL_ALLOCATORPMR_H
#define QLEVER_SRC_UTIL_ALLOCATORPMR_H

#include <cstddef>
#include <memory>
#include <memory_resource>

#include "util/AllocatorWithLimit.h"  // for AllocationExceedsLimitException, MemorySize
#include "util/MemorySize/MemorySize.h"
#include "util/Synchronized.h"

namespace ad_utility {

// A `std::pmr::memory_resource` that forwards all allocations to an *upstream*
// resource while enforcing a global memory limit. When an allocation would
// exceed the remaining budget, an optional `ClearOnAllocation` callback is
// invoked (e.g. to evict caches) and, if that does not free enough memory, an
// `AllocationExceedsLimitException` is thrown - identical behaviour to
// `AllocatorWithLimit`.
//
// Instances are shared (via `std::shared_ptr`, see the factory functions
// below) so that all allocators referring to the same resource count against
// the same budget. Access to the counter is synchronized.
class LimitedMemoryResource : public std::pmr::memory_resource {
 public:
  LimitedMemoryResource(
      MemorySize limit, std::pmr::memory_resource* upstream,
      ClearOnAllocation clearOnAllocation = noClearOnAllocation)
      : memoryLeft_{std::make_shared<
            ad_utility::Synchronized<detail::AllocationMemoryLeft, SpinLock>>(
            limit)},
        clearOnAllocation_{std::move(clearOnAllocation)},
        upstream_{upstream == nullptr ? std::pmr::new_delete_resource()
                                      : upstream} {}

  // Number of bytes still available for allocation.
  [[nodiscard]] MemorySize amountMemoryLeft() const {
    return memoryLeft_->wlock()->amountMemoryLeft();
  }

  std::pmr::memory_resource* upstream() const { return upstream_; }

 protected:
  void* do_allocate(std::size_t bytes, std::size_t alignment) override {
    const auto needed = MemorySize::bytes(bytes);
    const bool wasEnoughLeft =
        memoryLeft_->wlock()->decrease_if_enough_left_or_return_false(needed);
    if (!wasEnoughLeft) {
      AD_CORRECTNESS_CHECK(clearOnAllocation_);
      clearOnAllocation_(needed);
      memoryLeft_->wlock()->decrease_if_enough_left_or_throw(needed);
    }
    return upstream_->allocate(bytes, alignment);
  }

  void do_deallocate(void* p, std::size_t bytes,
                     std::size_t alignment) override {
    upstream_->deallocate(p, bytes, alignment);
    memoryLeft_->wlock()->increase(MemorySize::bytes(bytes));
  }

  bool do_is_equal(
      const std::pmr::memory_resource& other) const noexcept override {
    return this == &other;
  }

 private:
  std::shared_ptr<ad_utility::Synchronized<detail::AllocationMemoryLeft,
                                           SpinLock>>
      memoryLeft_;
  ClearOnAllocation clearOnAllocation_;
  std::pmr::memory_resource* upstream_;
};

// A `std::pmr`-based allocator that mirrors the public surface of
// `AllocatorWithLimit<T>` so it can be swapped in behind `qlever::Allocator`.
//
// It behaves like `std::pmr::polymorphic_allocator<T>` (it holds a *pointer* to
// a `memory_resource` and does not propagate on container copy/move/swap), but
// additionally
//   * keeps the referenced resource alive via an optional owning
//     `shared_ptr` (so factory-created resources are not dangling), and
//   * exposes `amountMemoryLeft()` and `.as<U>()`, which parts of the engine
//     rely on.
template <typename T>
class PmrAllocatorWithLimit {
 public:
  using value_type = T;

  // Unlike `AllocatorWithLimit`, `polymorphic_allocator` semantics do NOT
  // propagate on copy/move/swap: a copied container keeps using the same
  // resource pointer, which is exactly what we want for injected platform
  // pools.
  using propagate_on_container_copy_assignment = std::false_type;
  using propagate_on_container_move_assignment = std::false_type;
  using propagate_on_container_swap = std::false_type;

 private:
  // Optional owner keeping `resource_` alive. Null for platform-provided
  // resources with static lifetime.
  std::shared_ptr<std::pmr::memory_resource> owner_;
  std::pmr::memory_resource* resource_;

  template <typename U>
  friend class PmrAllocatorWithLimit;

 public:
  // Construct from a non-owning resource pointer (e.g. a platform Hi/Lo pool).
  explicit PmrAllocatorWithLimit(std::pmr::memory_resource* resource)
      : owner_{nullptr}, resource_{resource} {}

  // Construct from an owning resource (keeps it alive for all copies).
  explicit PmrAllocatorWithLimit(
      std::shared_ptr<std::pmr::memory_resource> resource)
      : owner_{std::move(resource)}, resource_{owner_.get()} {}

  PmrAllocatorWithLimit() = delete;

  // Rebinding copy constructor (STL requirement + `default_init_allocator`).
  template <typename U>
  PmrAllocatorWithLimit(const PmrAllocatorWithLimit<U>& other) noexcept
      : owner_{other.owner_}, resource_{other.resource_} {}

  PmrAllocatorWithLimit(const PmrAllocatorWithLimit&) = default;
  PmrAllocatorWithLimit& operator=(const PmrAllocatorWithLimit&) = default;
  PmrAllocatorWithLimit(PmrAllocatorWithLimit&&) noexcept = default;
  PmrAllocatorWithLimit& operator=(PmrAllocatorWithLimit&&) noexcept = default;

  // Obtain an allocator for another type sharing the same resource.
  template <typename U>
  PmrAllocatorWithLimit<U> as() const {
    PmrAllocatorWithLimit<U> result{resource_};
    result.owner_ = owner_;
    return result;
  }

  T* allocate(std::size_t n) {
    return static_cast<T*>(
        resource_->allocate(n * sizeof(T), alignof(T)));
  }

  void deallocate(T* p, std::size_t n) {
    resource_->deallocate(p, n * sizeof(T), alignof(T));
  }

  // Bytes still available. If the underlying resource is a
  // `LimitedMemoryResource` its budget is reported, otherwise "unlimited".
  [[nodiscard]] MemorySize amountMemoryLeft() const {
    if (auto* limited = dynamic_cast<LimitedMemoryResource*>(resource_)) {
      return limited->amountMemoryLeft();
    }
    return MemorySize::max();
  }

  std::pmr::memory_resource* resource() const { return resource_; }

  // Two allocators are equal iff they refer to the same resource.
  template <typename V>
  bool operator==(const PmrAllocatorWithLimit<V>& v) const {
    return resource_ == v.resource_;
  }
  template <typename V>
  bool operator!=(const PmrAllocatorWithLimit<V>& v) const {
    return !(*this == v);
  }
};

// Create a `PmrAllocatorWithLimit` backed by a fresh `LimitedMemoryResource`
// with the given limit over the given upstream (default: new/delete).
template <typename T>
PmrAllocatorWithLimit<T> makePmrAllocatorWithLimit(
    MemorySize limit,
    std::pmr::memory_resource* upstream = std::pmr::new_delete_resource(),
    ClearOnAllocation clearOnAllocation = noClearOnAllocation) {
  auto resource = std::make_shared<LimitedMemoryResource>(
      limit, upstream, std::move(clearOnAllocation));
  return PmrAllocatorWithLimit<T>{
      std::static_pointer_cast<std::pmr::memory_resource>(std::move(resource))};
}

// Create an unlimited `PmrAllocatorWithLimit` over new/delete.
template <typename T>
PmrAllocatorWithLimit<T> makeUnlimitedPmrAllocator() {
  return makePmrAllocatorWithLimit<T>(MemorySize::max());
}

// Create a `PmrAllocatorWithLimit` from a platform-provided resource without
// any limit enforcement (non-owning).
template <typename T>
PmrAllocatorWithLimit<T> makePmrAllocatorFromResource(
    std::pmr::memory_resource* resource) {
  return PmrAllocatorWithLimit<T>{resource};
}

}  // namespace ad_utility

#endif  // QLEVER_SRC_UTIL_ALLOCATORPMR_H
