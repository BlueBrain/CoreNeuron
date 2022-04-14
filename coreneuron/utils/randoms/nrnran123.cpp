/*
# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/
#include "coreneuron/mpi/core/nrnmpi.hpp"
#include "coreneuron/utils/memory.h"
#include "coreneuron/utils/nrnmutdec.hpp"
#include "coreneuron/utils/randoms/nrnran123.h"

#include <cmath>
#include <iostream>
#include <memory>
#include <mutex>

#ifdef CORENEURON_USE_BOOST_POOL
#include <boost/pool/pool_alloc.hpp>
#include <unordered_map>
#endif

// Defining these attributes seems to help nvc++ in OpenMP target offload mode.
#if defined(CORENEURON_ENABLE_GPU) && defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && \
    defined(_OPENMP) && defined(__CUDACC__)
#define CORENRN_HOST_DEVICE __host__ __device__
#else
#define CORENRN_HOST_DEVICE
#endif

namespace {
#ifdef CORENEURON_USE_BOOST_POOL
/** Tag type for use with boost::fast_pool_allocator that forwards to
 *  coreneuron::[de]allocate_unified(). Using a Random123-specific type here
 *  makes sure that allocations do not come from the same global pool as other
 *  usage of boost pools for objects with sizeof == sizeof(nrnran123_State).
 *
 *  The messy m_block_sizes map is just because `deallocate_unified` uses sized
 *  deallocations, but the Boost pool allocators don't. Because this is hidden
 *  behind the pool mechanism, these methods are not called very often and the
 *  overhead is minimal.
 */
struct random123_allocate_unified {
    using size_type = std::size_t;
    using difference_type = std::size_t;
    static char* malloc(const size_type bytes) {
        std::lock_guard<std::mutex> const lock{m_mutex};
        static_cast<void>(lock);
        auto* buffer = coreneuron::allocate_unified(bytes);
        m_block_sizes[buffer] = bytes;
        return reinterpret_cast<char*>(buffer);
    }
    static void free(char* const block) {
        std::lock_guard<std::mutex> const lock{m_mutex};
        static_cast<void>(lock);
        auto const iter = m_block_sizes.find(block);
        assert(iter != m_block_sizes.end());
        auto const size = iter->second;
        m_block_sizes.erase(iter);
        return coreneuron::deallocate_unified(block, size);
    }
    static std::mutex m_mutex;
    static std::unordered_map<void*, std::size_t> m_block_sizes;
};

std::mutex random123_allocate_unified::m_mutex{};
std::unordered_map<void*, std::size_t> random123_allocate_unified::m_block_sizes{};

using random123_allocator =
    boost::fast_pool_allocator<coreneuron::nrnran123_State, random123_allocate_unified>;
#else
using random123_allocator = coreneuron::unified_allocator<coreneuron::nrnran123_State>;
#endif
/* Global data structure per process. Using a unique_ptr here causes [minor]
 * problems because its destructor can be called very late during application
 * shutdown. If the destructor calls cudaFree and the CUDA runtime has already
 * been shut down then tools like cuda-memcheck reports errors.
 */
nrn_pragma_omp(declare target)
philox4x32_key_t g_k{};
nrn_pragma_omp(end declare target)
nrn_pragma_acc(declare create(g_k))

OMP_Mutex g_instance_count_mutex;

std::size_t g_instance_count{};

constexpr double SHIFT32 = 1.0 / 4294967297.0; /* 1/(2^32 + 1) */

/** @brief Provide a helper function in global namespace that is declared target for OpenMP
 * offloading to function correctly with NVHPC
 */
CORENRN_HOST_DEVICE philox4x32_ctr_t philox4x32_helper(coreneuron::nrnran123_State* s) {
    return philox4x32(s->c, g_k);
}
}  // namespace

namespace coreneuron {
std::size_t nrnran123_instance_count() {
    return g_instance_count;
}

/* if one sets the global, one should reset all the stream sequences. */
uint32_t nrnran123_get_globalindex() {
    return g_k.v[0];
}

void nrnran123_getseq(nrnran123_State* s, uint32_t* seq, char* which) {
    *seq = s->c.v[0];
    *which = s->which_;
}

void nrnran123_setseq(nrnran123_State* s, uint32_t seq, char which) {
    if (which > 3) {
        s->which_ = 0;
    } else {
        s->which_ = which;
    }
    s->c.v[0] = seq;
    s->r = philox4x32_helper(s);
}

void nrnran123_getids(nrnran123_State* s, uint32_t* id1, uint32_t* id2) {
    *id1 = s->c.v[2];
    *id2 = s->c.v[3];
}

void nrnran123_getids3(nrnran123_State* s, uint32_t* id1, uint32_t* id2, uint32_t* id3) {
    *id3 = s->c.v[1];
    *id1 = s->c.v[2];
    *id2 = s->c.v[3];
}

uint32_t nrnran123_ipick(nrnran123_State* s) {
    uint32_t rval;
    char which = s->which_;
    rval = s->r.v[int{which++}];
    if (which > 3) {
        which = 0;
        s->c.v[0]++;
        s->r = philox4x32_helper(s);
    }
    s->which_ = which;
    return rval;
}

double nrnran123_dblpick(nrnran123_State* s) {
    return nrnran123_uint2dbl(nrnran123_ipick(s));
}

double nrnran123_negexp(nrnran123_State* s) {
    /* min 2.3283064e-10 to max 22.18071 */
    return -std::log(nrnran123_dblpick(s));
}

/* at cost of a cached  value we could compute two at a time. */
double nrnran123_normal(nrnran123_State* s) {
    double w, x, y;
    double u1, u2;

    do {
        u1 = nrnran123_dblpick(s);
        u2 = nrnran123_dblpick(s);
        u1 = 2. * u1 - 1.;
        u2 = 2. * u2 - 1.;
        w = (u1 * u1) + (u2 * u2);
    } while (w > 1);

    y = std::sqrt((-2. * log(w)) / w);
    x = u1 * y;
    return x;
}

double nrnran123_uint2dbl(uint32_t u) {
    /* 0 to 2^32-1 transforms to double value in open (0,1) interval */
    /* min 2.3283064e-10 to max (1 - 2.3283064e-10) */
    return ((double) u + 1.0) * SHIFT32;
}

/* nrn123 streams are created from cpu launcher routine */
void nrnran123_set_globalindex(uint32_t gix) {
    // If the global seed is changing then we shouldn't have any active streams.
    {
        std::lock_guard<OMP_Mutex> _{g_instance_count_mutex};
        if (g_instance_count != 0 && nrnmpi_myid == 0) {
            std::cout
                << "nrnran123_set_globalindex(" << gix
                << ") called when a non-zero number of Random123 streams (" << g_instance_count
                << ") were active. This is not safe, some streams will remember the old value ("
                << g_k.v[0] << ')' << std::endl;
        }
    }
    g_k.v[0] = gix;
    nrn_pragma_acc(update device(g_k))
    nrn_pragma_omp(target update to(g_k))
}

/** @brief Allocate a new Random123 stream.
 *  @todo  It would be nicer if the API return type was
 *  std::unique_ptr<nrnran123_State, ...not specified...>, so we could use a
 *  custom allocator/deleter and avoid the (fragile) need for matching
 *  nrnran123_deletestream calls.
 */
nrnran123_State* nrnran123_newstream3(uint32_t id1,
                                      uint32_t id2,
                                      uint32_t id3,
                                      bool use_unified_memory) {
    // The `use_unified_memory` argument is an implementation detail to keep the
    // old behaviour that some Random123 streams that are known to only be used
    // from the CPU are allocated using new/delete instead of unified memory.
    // See OPENACC_EXCLUDED_FILES in coreneuron/CMakeLists.txt. If we dropped
    // this feature then we could always use coreneuron::unified_allocator.
#ifndef CORENEURON_ENABLE_GPU
    if (use_unified_memory) {
        throw std::runtime_error("Tried to use CUDA unified memory in a non-GPU build.");
    }
#endif
    nrnran123_State* s{nullptr};
    if (use_unified_memory) {
        s = coreneuron::allocate_unique<nrnran123_State>(random123_allocator{}).release();
    } else {
        s = new nrnran123_State{};
    }
    s->c.v[0] = 0;
    s->c.v[1] = id3;
    s->c.v[2] = id1;
    s->c.v[3] = id2;
    nrnran123_setseq(s, 0, 0);
    {
        std::lock_guard<OMP_Mutex> _{g_instance_count_mutex};
        ++g_instance_count;
    }
    return s;
}

/* nrn123 streams are destroyed from cpu launcher routine */
void nrnran123_deletestream(nrnran123_State* s, bool use_unified_memory) {
#ifndef CORENEURON_ENABLE_GPU
    if (use_unified_memory) {
        throw std::runtime_error("Tried to use CUDA unified memory in a non-GPU build.");
    }
#endif
    {
        std::lock_guard<OMP_Mutex> _{g_instance_count_mutex};
        --g_instance_count;
    }
    if (use_unified_memory) {
        std::unique_ptr<nrnran123_State, coreneuron::alloc_deleter<random123_allocator>> _{s};
    } else {
        delete s;
    }
}
}  // namespace coreneuron
