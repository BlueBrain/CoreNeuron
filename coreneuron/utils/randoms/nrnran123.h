/*
# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/
#pragma once

/* interface to Random123 */
/* http://www.thesalmons.org/john/random123/papers/random123sc11.pdf */

/*
The 4x32 generators utilize a uint32x4 counter and uint32x4 key to transform
into an almost cryptographic quality uint32x4 random result.
There are many possibilites for balancing the sharing of the internal
state instances while reserving a uint32 counter for the stream sequence
and reserving other portions of the counter vector for stream identifiers
and global index used by all streams.

We currently provide a single instance by default in which the policy is
to use the 0th counter uint32 as the stream sequence, words 2 and 3 as the
stream identifier, and word 0 of the key as the global index. Unused words
are constant uint32 0.

It is also possible to use Random123 directly without reference to this
interface. See Random123-1.02/docs/html/index.html
of the full distribution available from
http://www.deshawresearch.com/resources_random123.html
*/

#ifdef __bgclang__
#define R123_USE_MULHILO64_MULHI_INTRIN 0
#define R123_USE_GNU_UINT128            1
#endif

#include "coreneuron/utils/offload.hpp"

#include <Random123/philox.h>
#include <inttypes.h>

#include <cmath>

// Some files are compiled with DISABLE_OPENACC, and some builds have no GPU
// support at all. In these two cases, request that the random123 state is
// allocated using new/delete instead of CUDA unified memory.
#if defined(CORENEURON_ENABLE_GPU) && !defined(DISABLE_OPENACC)
#define CORENRN_RAN123_USE_UNIFIED_MEMORY true
#else
#define CORENRN_RAN123_USE_UNIFIED_MEMORY false
#endif

namespace coreneuron {

struct nrnran123_State {
    philox4x32_ctr_t c;
    philox4x32_ctr_t r;
    char which_;
};

struct nrnran123_array4x32 {
    uint32_t v[4];
};

namespace random123 {
nrn_pragma_omp(declare target)
inline philox4x32_key_t global_state;
nrn_pragma_omp(end declare target)

#if defined(CORENEURON_ENABLE_GPU) && defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && \
    defined(_OPENMP) && defined(__CUDACC__)
// Defining these attributes seems to help nvc++ in OpenMP target offload mode.
#define CORENRN_HOST_DEVICE __host__ __device__
#else
#define CORENRN_HOST_DEVICE
#endif
inline CORENRN_HOST_DEVICE philox4x32_ctr_t philox4x32_helper(nrnran123_State* s) {
    return philox4x32(s->c, global_state);
}
#undef CORENRN_HOST_DEVICE

}  // namespace random123

/* global index. eg. run number */
/* all generator instances share this global index */
void nrnran123_set_globalindex(uint32_t gix);
uint32_t nrnran123_get_globalindex();

// Utilities used for calculating model size, only called from the CPU.
std::size_t nrnran123_instance_count();
constexpr std::size_t nrnran123_state_size() {
    return sizeof(nrnran123_State);
}

/* routines for creating and deleting streams are called from cpu */
nrnran123_State* nrnran123_newstream3(uint32_t id1,
                                      uint32_t id2,
                                      uint32_t id3,
                                      bool use_unified_memory = CORENRN_RAN123_USE_UNIFIED_MEMORY);
inline nrnran123_State* nrnran123_newstream(
    uint32_t id1,
    uint32_t id2,
    bool use_unified_memory = CORENRN_RAN123_USE_UNIFIED_MEMORY) {
    return nrnran123_newstream3(id1, id2, 0, use_unified_memory);
}
void nrnran123_deletestream(nrnran123_State* s,
                            bool use_unified_memory = CORENRN_RAN123_USE_UNIFIED_MEMORY);

// Routines that get called from device code. These are all declared inline to
// give the OpenACC/OpenMP compilers an easier time.

inline void nrnran123_getseq(nrnran123_State* s, uint32_t* seq, char* which) {
    *seq = s->c.v[0];
    *which = s->which_;
}

inline void nrnran123_getids(nrnran123_State* s, uint32_t* id1, uint32_t* id2) {
    *id1 = s->c.v[2];
    *id2 = s->c.v[3];
}

inline void nrnran123_getids3(nrnran123_State* s, uint32_t* id1, uint32_t* id2, uint32_t* id3) {
    *id3 = s->c.v[1];
    *id1 = s->c.v[2];
    *id2 = s->c.v[3];
}

// uniform 0 to 2^32-1
inline uint32_t nrnran123_ipick(nrnran123_State* s) {
    uint32_t rval;
    char which = s->which_;
    rval = s->r.v[int{which++}];
    if (which > 3) {
        which = 0;
        s->c.v[0]++;
        s->r = random123::philox4x32_helper(s);
    }
    s->which_ = which;
    return rval;
}

// 0 to 2^32-1 transforms to double value in open (0,1) interval, min
// 2.3283064e-10 to max (1 - 2.3283064e-10)
inline double nrnran123_uint2dbl(uint32_t u) {
    constexpr double SHIFT32 = 1.0 / 4294967297.0; /* 1/(2^32 + 1) */
    return (static_cast<double>(u) + 1.0) * SHIFT32;
}

// uniform open interval (0,1), nrnran123_dblpick minimum value is 2.3283064e-10 and max value is
// 1-min
inline double nrnran123_dblpick(nrnran123_State* s) {
    return nrnran123_uint2dbl(nrnran123_ipick(s));
}

/* this could be called from openacc parallel construct (in INITIAL block) */
inline void nrnran123_setseq(nrnran123_State* s, uint32_t seq, char which) {
    if (which > 3) {
        s->which_ = 0;
    } else {
        s->which_ = which;
    }
    s->c.v[0] = seq;
    s->r = random123::philox4x32_helper(s);
}

// mean 1.0, min value is 2.3283064e-10, max is 22.18071
inline double nrnran123_negexp(nrnran123_State* s) {
    /* min 2.3283064e-10 to max 22.18071 */
    return -std::log(nrnran123_dblpick(s));
}

// at cost of a cached  value we could compute two at a time
inline double nrnran123_normal(nrnran123_State* s) {
    double w, x, y;
    double u1, u2;
    do {
        u1 = nrnran123_dblpick(s);
        u2 = nrnran123_dblpick(s);
        u1 = 2. * u1 - 1.;
        u2 = 2. * u2 - 1.;
        w = (u1 * u1) + (u2 * u2);
    } while (w > 1);
    y = std::sqrt((-2. * std::log(w)) / w);
    x = u1 * y;
    return x;
}

// OL 220414: deleted declarations for nrnran123_gauss, nrnran123_iran that were
// never defined

}  // namespace coreneuron
