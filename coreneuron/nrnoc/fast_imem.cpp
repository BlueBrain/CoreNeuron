/*
Copyright (c) 2019, Blue Brain Project
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

#include "coreneuron/nrnconf.h"
#include "coreneuron/nrnoc/fast_imem.h"
#include "coreneuron/nrniv/memory.h"
#include "coreneuron/nrnmpi/nrnmpi.h"

namespace coreneuron {

extern int nrn_nthread;
extern NrnThread *nrn_threads;
bool nrn_use_fast_imem;
static int fast_imem_nthread_ = 0;
static int *fast_imem_size_ = nullptr;
static NrnFastImem* fast_imem_;

static void fast_imem_free() {
    int i;
    for (i = 0; i < nrn_nthread; ++i) {
        nrn_threads[i].nrn_fast_imem = NULL;
    }
    for (i = 0; i < fast_imem_nthread_; ++i) {
        if (fast_imem_size_[i] > 0) {
            free(fast_imem_[i].nrn_sav_rhs);
            free(fast_imem_[i].nrn_sav_d);
        }
    }
    if (fast_imem_nthread_) {
        free(fast_imem_size_);
        free(fast_imem_);
        fast_imem_nthread_ = 0;
        fast_imem_size_ = nullptr;
        fast_imem_ = nullptr;
    }
}

/*
Avoid invalidating pointers to i_membrane_ unless the number of compartments
in a thread has changed.
*/

static void fast_imem_alloc() {
    int i;
    if (fast_imem_nthread_ != nrn_nthread) {
        fast_imem_free();
        fast_imem_nthread_ = nrn_nthread;
        fast_imem_size_ = (int*)ecalloc(nrn_nthread, sizeof(int));
        fast_imem_ = (NrnFastImem*)ecalloc(nrn_nthread, sizeof(NrnFastImem));
    }
    for (i=0; i < nrn_nthread; ++i) {
        NrnThread* nt = nrn_threads + i;
        int n = nt->end;
        NrnFastImem* fi = fast_imem_ + i;
        if (n != fast_imem_size_[i]) {
            if (fast_imem_size_[i] > 0) {
                free(fi->nrn_sav_rhs);
                free(fi->nrn_sav_d);
            }
            if (n > 0) {
                fi->nrn_sav_rhs = (double*)emalloc_align(n * sizeof(double));
                fi->nrn_sav_d = (double*)emalloc_align(n * sizeof(double));
            }
            fast_imem_size_[i] = n;
        }
    }
}

void nrn_fast_imem_alloc() {
    if (nrn_use_fast_imem) {
        int i;
        fast_imem_alloc();
        for (i=0; i < nrn_nthread; ++i) {
            nrn_threads[i].nrn_fast_imem = fast_imem_ + i;
        }
    }else{
        fast_imem_free();
    }
}

void use_fast_imem() {
    nrn_use_fast_imem = true;
    nrn_fast_imem_alloc();
}

void nrn_calc_fast_imem(NrnThread* _nt) {
    int i;
    int i1 = 0;
    int i3 = _nt->end;

    double* vec_rhs = &(VEC_RHS(0));
    double* vec_area = &(VEC_AREA(0));

    double* pd = _nt->nrn_fast_imem->nrn_sav_d;
    double* prhs = _nt->nrn_fast_imem->nrn_sav_rhs;
    FILE *fp_rhs, *fp_d;
    char rhs_filename[20], d_filename[20];
    sprintf(rhs_filename,"rhs.CORENEURON.%d",nrnmpi_myid);
    sprintf(d_filename,"d.CORENEURON.%d",nrnmpi_myid);
    fp_rhs = fopen(rhs_filename, "a");
    fp_d = fopen(d_filename, "a");
    fprintf(fp_rhs, "\n%.8e time\n", _nt->_t);
    fprintf(fp_d, "\n%.8e time\n", _nt->_t);
    for (i = i1; i < i3 ; ++i) {
        prhs[i] = (pd[i]*vec_rhs[i] + prhs[i])*vec_area[i]*0.01;
        fprintf(fp_rhs, "%.8e, ", prhs[i]);
    }
    fprintf(fp_rhs, "\n");
    for (i = i1; i < i3 ; ++i) {
        fprintf(fp_d, "%.8e, ", pd[i]);
    }
    fprintf(fp_d, "\n");
    fclose(fp_rhs);
    fclose(fp_d);
}

}