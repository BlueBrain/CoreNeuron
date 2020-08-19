/*
Copyright (c) 2020, Blue Brain Project
All rights reserved.
     
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "coreneuron/coreneuron.hpp"
#include "coreneuron/io/nrn2core_direct.h"
#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/nrniv/nrniv_decl.h"
#include "coreneuron/io/core2nrn_data_return.hpp"

size_t (*nrn2core_type_return_)(int type, int tid,
  double*& data, double**& mdata);

namespace coreneuron {

static void inverse_permute_copy(size_t n, double* permuted_src, double* dest,
                                 int* permute)
{
  if (permute) {
    for (size_t i = 0; i < n; ++i) {
      dest[permute[i]] = permuted_src[i];
    }
  }else{
    std::copy(permuted_src, permuted_src + n, dest);
  }
}

static void soa2aos_inverse_permute_copy(size_t n, int sz, int stride,
  double* src, double** dest, int* permute)
{
  // src is soa and permuted. dest is n pointers to sz doubles (aos).
  for (size_t instance = 0; instance < n; ++instance) {
    double* d = dest[permute[instance]];
    double* s = src + instance;
    for (int i = 0; i < sz; ++i) {
      d[i] = s[i*stride];
    }
  }
}

static void soa2aos_inverse_copy(size_t n, int sz, int stride,
  double* src, double** dest)
{
  // src is soa and permuted. dest is n pointers to sz doubles (aos).
  for (size_t instance = 0; instance < n; ++instance) {
    double* d = dest[instance];
    double* s = src + instance;
    for (int i = 0; i < sz; ++i) {
      d[i] = s[i*stride];
    }
  }
}

void core2nrn_data_return() {
  if (!nrn2core_type_return_) {
    return;
  }
  for (int tid = 0; tid < nrn_nthread; ++tid) {
    size_t n = 0;
    double* data = nullptr;
    double** mdata = nullptr;
    NrnThread& nt = nrn_threads[tid];

    if (nt.end) { // transfer voltage and possibly i_membrane_
      n = (*nrn2core_type_return_)(voltage, tid, data, mdata);
      assert(n == size_t(nt.end) && data);
      inverse_permute_copy(n, nt._actual_v, data, nt._permute);

      if (nt.nrn_fast_imem) {
        n = (*nrn2core_type_return_)(i_membrane_, tid, data, mdata);
        assert(n == size_t(nt.end) && data);
        inverse_permute_copy(n, nt.nrn_fast_imem->nrn_sav_rhs, data, nt._permute);
      }
    }

    for (NrnThreadMembList* tml = nt.tml; tml; tml = tml->next) {
      int mtype = tml->index;
      Memb_list* ml = tml->ml;
      n = (*nrn2core_type_return_)(mtype, tid, data, mdata);
      assert(n == size_t(ml->nodecount) && mdata);
      if (n == 0) {
        continue;
      }
      // NEURON is AoS, CoreNEURON may be SoA and may be permuted.
      // On the NEURON side, the data is actually contiguous because of
      // cache_efficient, but that may not be the case for ARTIFICIAL_CELL.
      // For initial implementation simplicity, use the mdata info which gives
      // a double* for each param_size mech instance.
      int* permute = ml->_permute;
      double* cndat = ml->data;
      int layout = corenrn.get_mech_data_layout()[mtype];
      int sz = corenrn.get_prop_param_size()[mtype];
      if (layout == 0) { /* SoA */
        int stride = ml->_nodecount_padded;
        if (permute) {
          soa2aos_inverse_permute_copy(n, sz, stride, cndat, mdata, permute);
        } else {
          soa2aos_inverse_copy(n, sz, stride, cndat, mdata);
        }
      } else { /* AoS */
        printf("%s is AoS\n", corenrn.get_memb_func(mtype).sym);
        assert(0);
      }
    }
  }
}

}  // namespace coreneuron
