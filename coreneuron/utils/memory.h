/*
# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#pragma once

#include <cstdint>
#include <cstring>
#include <memory>

#include "coreneuron/utils/nrn_assert.h"
#include "coreneuron/nrniv/nrniv_decl.h"

#if !defined(NRN_SOA_BYTE_ALIGN)
// for layout 0, every range variable array must be aligned by at least 16 bytes (the size of the
// simd memory bus)
#define NRN_SOA_BYTE_ALIGN (8 * sizeof(double))
#endif

namespace coreneuron {
/**
 * @brief Check if GPU support is enabled.
 *
 * This returns true if GPU support was enabled at compile time and at runtime
 * via coreneuron.gpu = True and/or --gpu, otherwise it returns false.
 */
bool gpu_enabled();

/** @brief Allocate host memory using new
 */
void* allocate_host(size_t num_bytes, size_t alignment = NRN_SOA_BYTE_ALIGN);

/** @brief Deallocate memory allocated by `allocate_host`.
 */
void deallocate_host(void* pointer, std::size_t num_bytes);

/** @brief Allocate unified memory in GPU builds iff GPU enabled, otherwise new
 */
void* allocate_unified(std::size_t num_bytes, std::size_t alignment = NRN_SOA_BYTE_ALIGN);

/** @brief Deallocate memory allocated by `allocate_unified`.
 */
void deallocate_unified(void* pointer, std::size_t num_bytes);

/** @brief C++ allocator that uses [de]allocate_unified.
 */

template <typename T, std::size_t alignment = NRN_SOA_BYTE_ALIGN>
struct host_allocator {
    using value_type = T;

    host_allocator() = default;

    template <typename U>
    host_allocator(host_allocator<U, alignment> const&) noexcept {}

    value_type* allocate(std::size_t n) {
        return static_cast<value_type*>(allocate_host(n * sizeof(value_type), alignment));
    }

    void deallocate(value_type* p, std::size_t n) noexcept {
        deallocate_host(p, n);
    }
};

template <typename T, typename U>
bool operator==(host_allocator<T> const&, host_allocator<U> const&) noexcept {
    return true;
}

template <typename T, typename U>
bool operator!=(host_allocator<T> const& x, host_allocator<U> const& y) noexcept {
    return !(x == y);
}


template <typename T, bool force>
struct unified_allocator {
    using value_type = T;

    unified_allocator() = default;

    template <typename U>
    unified_allocator(unified_allocator<U, force> const&) noexcept {}

    value_type* allocate(std::size_t n) {
        void* ptr = nullptr;
        // TODO: implement here the "always unified" semantics
        if constexpr (force) {
            ptr = allocate_unified(n * sizeof(value_type), 0);
        } else {
            ptr = allocate_unified(n * sizeof(value_type), 0);
        }
        return static_cast<value_type*>(ptr);
    }

    void deallocate(value_type* p, std::size_t n) noexcept {
        deallocate_unified(p, n);
    }
};

template <typename T, bool Tforce, typename U, bool Uforce>
bool operator==(unified_allocator<T, Tforce> const&, unified_allocator<U, Uforce> const&) noexcept {
    return true;
}

template <typename T, bool Tforce, typename U, bool Uforce>
bool operator!=(unified_allocator<T, Tforce> const& x,
                unified_allocator<U, Uforce> const& y) noexcept {
    return !(x == y);
}

/** @brief Allocator-aware deleter for use with std::unique_ptr.
 *
 * The deleter was extended to also support unique_ptr<T[]> types.
 *
 *  This is copied from https://stackoverflow.com/a/23132307. See also
 *  http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0316r0.html,
 *  http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p0211r3.html, and
 *  boost::allocate_unique<...>.
 *  Hopefully std::allocate_unique will be included in C++23.
 */

template <typename Alloc>
struct alloc_deleter {
    alloc_deleter() = default;  // OL210813 addition
    alloc_deleter(const Alloc& a)
        : a(a)
        , size(1) {}

    alloc_deleter(const Alloc& a, std::size_t N)
        : a(a)
        , size(N) {}

    using pointer = typename std::allocator_traits<Alloc>::pointer;

    void operator()(pointer p) const {
        Alloc aa(a);
        auto pp = &p[size - 1];
        do {
            std::allocator_traits<Alloc>::destroy(aa, std::addressof(*pp--));
        } while (pp >= p);
        std::allocator_traits<Alloc>::deallocate(aa, p, size);
    }

  private:
    Alloc a;
    std::size_t size;
};

template <typename T, typename Alloc, typename... Args>
auto allocate_unique(const Alloc& alloc, Args&&... args) {
    using AT = std::allocator_traits<Alloc>;
    static_assert(std::is_same<typename AT::value_type, std::remove_cv_t<T>>{}(),
                  "Allocator has the wrong value_type");

    Alloc a(alloc);
    auto p = AT::allocate(a, 1);
    try {
        AT::construct(a, std::addressof(*p), std::forward<Args>(args)...);
        using D = alloc_deleter<Alloc>;
        return std::unique_ptr<T, D>(p, D(a));
    } catch (...) {
        AT::deallocate(a, p, 1);
        throw;
    }
}


template <typename T, typename Alloc>
auto allocate_unique(const Alloc& alloc, std::size_t N) {
    using AT = std::allocator_traits<Alloc>;
    using ElemType = typename std::remove_extent<T>::type;
    static_assert(std::is_same<typename AT::value_type, std::remove_cv_t<ElemType>>{}(),
                  "Allocator has the wrong value_type");

    Alloc a(alloc);
    auto p = AT::allocate(a, N);
    auto ep = reinterpret_cast<ElemType*>(p);
    try {
        for (std::size_t i = 0; i < N; ++i) {
            AT::construct(a, std::addressof(*ep++));
        }
        using D = alloc_deleter<Alloc>;
        return std::unique_ptr<T, D>(p, D(a, N));
    } catch (...) {
        AT::deallocate(a, p, N);
        throw;
    }
}


class [[deprecated]] MemoryManaged {};

class HostMemManaged {
  protected:
    template <typename T>
    using allocator = host_allocator<T>;

    template <typename U>
    using unified_uniq_ptr =
        std::unique_ptr<U, alloc_deleter<allocator<typename std::remove_extent<U>::type>>>;

    template <typename T>
    void grow_buf(unified_uniq_ptr<T[]>& buf, std::size_t size, std::size_t new_size) {
        auto new_buf = allocate_unique<T[]>(allocator<T>{}, new_size);
        std::copy(buf.get(), buf.get() + size, new_buf.get());
        buf.swap(new_buf);
    }

    /**
     * Initialize new dest buffer from src
     *
     */
    template <typename T>
    void initialize_from_other(unified_uniq_ptr<T>& dest,
                               unified_uniq_ptr<T>& src,
                               std::size_t size) {
        dest = allocate_unique<T[]>(allocator<T>{}, size);
        std::copy(src.get(), src.get() + size, dest.get);
    }
};

template <bool force>
class UnifiedMemManaged {
  public:
    template <typename T>
    using allocator = unified_allocator<T, force>;

    template <typename U>
    using unified_uniq_ptr =
        std::unique_ptr<U, alloc_deleter<allocator<typename std::remove_extent<U>::type>>>;

  protected:
    template <typename T>
    void grow_buf(unified_uniq_ptr<T[]>& buf, std::size_t size, std::size_t new_size) {
#ifdef CORENEURON_ENABLE_GPU
        if (force || corenrn_param.gpu) {
            int cannot_reallocate_on_device = 0;
            assert(cannot_reallocate_on_device);
        }
#endif
        auto new_buf = allocate_unique<T[]>(allocator<T>{}, new_size);
        std::copy(buf.get(), buf.get() + size, new_buf.get());
        buf.swap(new_buf);
    }

    /**
     * Initialize new dest buffer from src
     *
     */
    template <typename T>
    void initialize_from_other(unified_uniq_ptr<T>& dest,
                               const unified_uniq_ptr<T>& src,
                               std::size_t size) {
        dest = allocate_unique<T[]>(allocator<T>{}, size);
        std::copy(src.get(), src.get() + size, dest.get);
    }
};


}  // namespace coreneuron

/// for gpu builds with unified memory support
#ifdef CORENEURON_UNIFIED_MEMORY

////////// to be removed
#include <cuda_runtime_api.h>

// TODO : error handling for CUDA routines
inline void alloc_memory(void*& pointer, size_t num_bytes, size_t /*alignment*/) {
    cudaMallocManaged(&pointer, num_bytes);
}

inline void calloc_memory(void*& pointer, size_t num_bytes, size_t /*alignment*/) {
    alloc_memory(pointer, num_bytes, 64);
    cudaMemset(pointer, 0, num_bytes);
}

inline void free_memory(void* pointer) {
    cudaFree(pointer);
}
////////////////////
/**
 * A base class providing overloaded new and delete operators for CUDA allocation
 *
 * Classes that should be allocated on the GPU should inherit from this class. Additionally they
 * may need to implement a special copy-construtor. This is documented here:
 * \link: https://devblogs.nvidia.com/unified-memory-in-cuda-6/
 */
/*class MemoryManaged {
  public:
    void* operator new(size_t len) {
        void* ptr;
        cudaMallocManaged(&ptr, len);
        cuyydaDeviceSynchronize();
        return ptr;
    }

    void* operator new[](size_t len) {
        void* ptr;
        cudaMallocManaged(&ptr, len);
        cudaDeviceSynchronize();
        return ptr;
    }

    void operator delete(void* ptr) {
        cudaDeviceSynchronize();
        cudaFree(ptr);
    }

    void operator delete[](void* ptr) {
        cudaDeviceSynchronize();
        cudaFree(ptr);
    }
};*/


/// for cpu builds use posix memalign
#else
/*class MemoryManaged {
    // does nothing by default
};*/

//////// to be removed
inline void alloc_memory(void*& pointer, size_t num_bytes, size_t alignment) {
    size_t fill = 0;
    if (num_bytes % alignment != 0) {
        size_t multiple = num_bytes / alignment;
        fill = alignment * (multiple + 1) - num_bytes;
    }
    nrn_assert((pointer = std::aligned_alloc(alignment, num_bytes + fill)) != nullptr);
}

inline void calloc_memory(void*& pointer, size_t num_bytes, size_t alignment) {
    alloc_memory(pointer, num_bytes, alignment);
    memset(pointer, 0, num_bytes);
}

inline void free_memory(void* pointer) {
    free(pointer);
}
/////////////////

#endif

namespace coreneuron {

/** Independent function to compute the needed chunkding,
    the chunk argument is the number of doubles the chunk is chunkded upon.
*/
template <int chunk>
inline int soa_padded_size(int cnt, int layout) {
    int imod = cnt % chunk;
    if (layout == Layout::AoS)
        return cnt;
    if (imod) {
        int idiv = cnt / chunk;
        return (idiv + 1) * chunk;
    }
    return cnt;
}

/** Check for the pointer alignment.
 */
inline bool is_aligned(void* pointer, std::size_t alignment) {
    return (reinterpret_cast<std::uintptr_t>(pointer) % alignment) == 0;
}

/**
 * Allocate aligned memory. This will be unified memory if the corresponding
 * CMake option is set. This must be freed with the free_memory method.
 */
inline void* emalloc_align(size_t size, size_t alignment = NRN_SOA_BYTE_ALIGN) {
    void* memptr;
    memptr = allocate_unified(size, alignment);
    nrn_assert(is_aligned(memptr, alignment));
    return memptr;
}

/**
 * Allocate the aligned memory and set it to 0. This will be unified memory if
 * the corresponding CMake option is set. This must be freed with the
 * free_memory method.
 */
inline void* ecalloc_align(size_t n, size_t size, size_t alignment = NRN_SOA_BYTE_ALIGN) {
    void* p;
    if (n == 0) {
        return nullptr;
    }
    p = allocate_unified(n * size, alignment);
    // TODO: Maybe allocate_unified should do this when asked (and use cuda API when available)
    memset(p, 0, n * size);
    nrn_assert(is_aligned(p, alignment));
    return p;
}
}  // namespace coreneuron
