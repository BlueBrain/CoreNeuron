/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

/**
 * @brief Definition of print_gpu_memory_usage() for non GPU builds
 *
 */
#if !defined(__CUDACC__) && !defined(_OPENACC)
void print_gpu_memory_usage(){};
#endif
