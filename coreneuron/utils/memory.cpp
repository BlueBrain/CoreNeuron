/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/
#include "coreneuron/apps/corenrn_parameters.hpp"
#include "coreneuron/utils/memory.h"

#ifdef CORENEURON_ENABLE_GPU
#include <cuda_runtime_api.h>
#endif

#include <cassert>
#include <cstdlib>

namespace coreneuron {
bool gpu_enabled() {
#ifdef CORENEURON_ENABLE_GPU
    return corenrn_param.gpu;
#else
    return false;
#endif
}

void* allocate_host(size_t num_bytes, std::size_t alignment) {
    size_t fill = 0;
    void* pointer = nullptr;
    if (num_bytes % alignment != 0) {
        size_t multiple = num_bytes / alignment;
        fill = alignment * (multiple + 1) - num_bytes;
    }
    nrn_assert((pointer = std::aligned_alloc(alignment, num_bytes + fill)) != nullptr);
    return pointer;
}

void deallocate_host(void* pointer, std::size_t num_bytes) {
    free(pointer);
}

void* allocate_unified(std::size_t num_bytes, std::size_t alignment) {
#ifdef CORENEURON_ENABLE_GPU
    // The build supports GPU execution, check if --gpu was passed to actually
    // enable it. We should not call CUDA APIs in GPU builds if --gpu was not passed.
    if (corenrn_param.gpu) {
        void* pointer = nullptr;
        // Allocate managed/unified memory.
        auto const code = cudaMallocManaged(&pointer, num_bytes);
        assert(code == cudaSuccess);
        return pointer;
    }
#endif
    // Either the build does not have GPU support or --gpu was not passed.
    // Allocate using host allocator.
    return allocate_host(num_bytes, alignment);
}

void deallocate_unified(void* pointer, std::size_t num_bytes) {
    // See comments in allocate_unified to understand the different branches.
#ifdef CORENEURON_ENABLE_GPU
    if (corenrn_param.gpu) {
        // Deallocate managed/unified memory.
        auto const code = cudaFree(pointer);
        assert(code == cudaSuccess);
        return;
    }
#endif
    deallocate_host(pointer, num_bytes);
}
}  // namespace coreneuron
