/*
# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/
#include "coreneuron/gpu/nrn_acc_manager.hpp"
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
OMP_Mutex g_instance_count_mutex;
std::size_t g_instance_count{};
bool gpu_enabled{false};
}  // namespace

namespace coreneuron {
void init_nrnran123() {
    std::cout << "Initialising Random123 global state" << std::endl;
    gpu_enabled = coreneuron::unified_memory_enabled();
    nrn_pragma_acc(enter data copyin(random123::global_state) if(gpu_enabled))
}

std::size_t nrnran123_instance_count() {
    return g_instance_count;
}

/* if one sets the global, one should reset all the stream sequences. */
uint32_t nrnran123_get_globalindex() {
    return random123::global_state.v[0];
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
                << random123::global_state.v[0] << ')' << std::endl;
        }
    }
    // TODO: should this have a mutex?
    if(gix != random123::global_state.v[0]) {
        random123::global_state.v[0] = gix;
        nrn_pragma_acc(update device(random123::global_state) if(gpu_enabled))
        nrn_pragma_omp(target update to(random123::global_state) if(gpu_enabled))
    }
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
