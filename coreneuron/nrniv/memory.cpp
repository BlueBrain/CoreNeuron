/*
Copyright (c) 2016, Blue Brain Project
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

#include "coreneuron/nrniv/memory.h"

/// for gpu builds with unified memory support
#if (defined(__CUDACC__) || defined(UNIFIED_MEMORY))
void *operator new(size_t len) {
  void *ptr;
  cudaMallocManaged(&ptr, len);
  cudaDeviceSynchronize();
  return ptr;
}

void *operator new[](size_t len) {
  void *ptr;
  cudaMallocManaged(&ptr, len);
  cudaDeviceSynchronize();
  return ptr;
}

void operator delete(void *ptr) {
  cudaDeviceSynchronize();
  cudaFree(ptr);
}

void operator delete[](void *ptr) {
  cudaDeviceSynchronize();
  cudaFree(ptr);
}


#else

#include <stdlib.h>

void *operator new(size_t len) {
  void *ptr;
#if defined(MINGW)
  nrn_assert( (ptr = _aligned_malloc(len, NRN_SOA_BYTE_ALIGN)) != nullptr);
#else
  nrn_assert(posix_memalign(&ptr, NRN_SOA_BYTE_ALIGN, len) == 0);
#endif
  return ptr;
}

void *operator new[](size_t len) {
  void *ptr;
#if defined(MINGW)
  nrn_assert( (ptr = _aligned_malloc(len, NRN_SOA_BYTE_ALIGN)) != nullptr);
#else
  nrn_assert(posix_memalign(&ptr, NRN_SOA_BYTE_ALIGN, len) == 0);
#endif
  memset(ptr, 0, len);
  return ptr;
}

void operator delete(void *ptr) {
  free(ptr);
}

void operator delete[](void *ptr) {
  free(ptr);
}

#endif
