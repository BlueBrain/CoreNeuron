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

#include <iostream>
#include <sstream>
#include <string.h>
#include <stdexcept>  // std::lenght_error
#include <vector>
#include <algorithm>
#include <numeric>
#include "coreneuron/nrnconf.h"
#include "coreneuron/nrniv/nrniv_decl.h"
#include "coreneuron/nrniv/output_spikes.h"
#include "coreneuron/nrnmpi/nrnmpi.h"
#include "coreneuron/nrniv/nrnmutdec.h"
#include "coreneuron/nrnmpi/nrnmpi_impl.h"
#include "coreneuron/nrnmpi/nrnmpidec.h"

namespace coreneuron {
std::vector<double> spikevec_time;
std::vector<int> spikevec_gid;

static MUTDEC

    void
    mk_spikevec_buffer(int sz) {
    try {
        spikevec_time.reserve(sz);
        spikevec_gid.reserve(sz);
    } catch (const std::length_error& le) {
        std::cerr << "Lenght error" << le.what() << std::endl;
    }
    MUTCONSTRUCT(1);
}

void spikevec_lock() {
    MUTLOCK
}

void spikevec_unlock() {
    MUTUNLOCK
}

#if NRNMPI

void sort_spikes() {
    double min_time, max_time;
    double lmin_time = *(std::min_element(spikevec_time.begin(), spikevec_time.end()));
    double lmax_time = *(std::max_element(spikevec_time.begin(), spikevec_time.end()));
    MPI_Allreduce(&lmin_time, &min_time, 1,
                  MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);
    MPI_Allreduce(&lmax_time, &max_time, 1,
                  MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

    int nranks;
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);

    // Note: the vectors are allocated one elements too large to be able to
    // use them also as offset buffers in MPI_Alltoallv
    std::vector<int> snd_cnts = std::vector<int>(nranks+1, 0);
    std::vector<int> rcv_cnts = std::vector<int>(nranks+1, 0);

    double bin_t = (max_time - min_time) / nranks;
    // first find number of spikes in each time window
    for (const auto& st : spikevec_time) {
        int idx = (int)(st - min_time)/bin_t;
        snd_cnts[idx+1]++;
    }

    // now let each rank know how many spikes they will receive
    // and get in turn all the buffer sizes to receive
    MPI_Alltoall(&snd_cnts[1], 1, MPI_INTEGER,
                 &rcv_cnts[1], 1, MPI_INTEGER, MPI_COMM_WORLD);

    // prepare new sorted vectors
    std::vector<double> svt_buf = std::vector<double>(spikevec_time.size());
    std::vector<int> svg_buf = std::vector<int>(spikevec_gid.size());

    // now exchange data
    MPI_Alltoallv(&spikevec_time[0], &snd_cnts[1], &snd_cnts[0], MPI_DOUBLE,
                  &svt_buf[0], &rcv_cnts[1], &rcv_cnts[0], MPI_DOUBLE,
                  MPI_COMM_WORLD);
    MPI_Alltoallv(&spikevec_gid[0], &snd_cnts[1], &snd_cnts[0], MPI_INTEGER,
                  &svg_buf[0], &rcv_cnts[1], &rcv_cnts[0], MPI_INTEGER,
                  MPI_COMM_WORLD);

    // first build a permutation vector
    std::vector<std::size_t> perm(spikevec_time.size());
    std::iota(perm.begin(), perm.end(), 0);
    // sort by gid (second predicate first)
    std::stable_sort(perm.begin(), perm.end(),
        [&](std::size_t i, std::size_t j){
            return svg_buf[i] < svg_buf[j];
        });
    // then sort by time
    std::stable_sort(perm.begin(), perm.end(),
        [&](std::size_t i, std::size_t j){
            return svt_buf[i] < svt_buf[j];
        });
    // now apply permutation to time and gid into original vectors
    std::transform(perm.begin(), perm.end(), spikevec_time.begin(),
        [&](std::size_t i) {
            return svt_buf[i];
        });
    std::transform(perm.begin(), perm.end(), spikevec_gid.begin(),
        [&](std::size_t i) {
            return svg_buf[i];
        });
}


/** Write generated spikes to out.dat using mpi parallel i/o.
 *  \todo : MPI related code should be factored into nrnmpi.c
 *          Check spike record length which is set to 64 chars
 */
void output_spikes_parallel(const char* outpath) {
    std::stringstream ss;
    ss << outpath << "/out.dat";
    std::string fname = ss.str();

    // remove if file already exist
    if (nrnmpi_myid == 0) {
        remove(fname.c_str());
    }
    sort_spikes();
    nrnmpi_barrier();

    // each spike record in the file is time + gid (64 chars sufficient)
    const int SPIKE_RECORD_LEN = 64;
    unsigned num_spikes = spikevec_gid.size();
    unsigned num_bytes = (sizeof(char) * num_spikes * SPIKE_RECORD_LEN);
    char* spike_data = (char*)malloc(num_bytes);

    if (spike_data == NULL) {
        printf("Error while writing spikes due to memory allocation\n");
        return;
    }

    // empty if no spikes
    strcpy(spike_data, "");

    // populate buffer with all spike entries
    char spike_entry[SPIKE_RECORD_LEN];
    for (unsigned i = 0; i < num_spikes; i++) {
        snprintf(spike_entry, 64, "%.8g\t%d\n", spikevec_time[i], spikevec_gid[i]);
        strcat(spike_data, spike_entry);
    }

    // calculate offset into global file. note that we don't write
    // all num_bytes but only "populated" buffer
    unsigned long num_chars = strlen(spike_data);
    unsigned long offset = 0;

    // global offset into file
    MPI_Exscan(&num_chars, &offset, 1, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD);

    // write to file using parallel mpi i/o
    MPI_File fh;
    MPI_Status status;

    // ibm mpi (bg-q) expects char* instead of const char* (even though it's standard)
    int op_status = MPI_File_open(MPI_COMM_WORLD, (char*)fname.c_str(),
                                  MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &fh);
    if (op_status != MPI_SUCCESS && nrnmpi_myid == 0) {
        std::cerr << "Error while opening spike output file " << fname << std::endl;
        abort();
    }

    op_status = MPI_File_write_at_all(fh, offset, spike_data, num_chars, MPI_BYTE, &status);
    if (op_status != MPI_SUCCESS && nrnmpi_myid == 0) {
        std::cerr << "Error while writing spike output " << std::endl;
        abort();
    }

    MPI_File_close(&fh);
}
#endif

void output_spikes_serial(const char* outpath) {
    std::stringstream ss;
    ss << outpath << "/out.dat";
    std::string fname = ss.str();

    // remove if file already exist
    remove(fname.c_str());

    FILE* f = fopen(fname.c_str(), "w");
    if (!f && nrnmpi_myid == 0) {
        std::cout << "WARNING: Could not open file for writing spikes." << std::endl;
        return;
    }

    for (unsigned i = 0; i < spikevec_gid.size(); ++i)
        if (spikevec_gid[i] > -1)
            fprintf(f, "%.8g\t%d\n", spikevec_time[i], spikevec_gid[i]);

    fclose(f);
}

void output_spikes(const char* outpath) {
#if NRNMPI
    if (nrnmpi_initialized()) {
        output_spikes_parallel(outpath);
    } else {
        output_spikes_serial(outpath);
    }
#else
    output_spikes_serial(outpath);
#endif
}

void validation(std::vector<std::pair<double, int> >& res) {
    for (unsigned i = 0; i < spikevec_gid.size(); ++i)
        if (spikevec_gid[i] > -1)
            res.push_back(std::make_pair(spikevec_time[i], spikevec_gid[i]));
}
}  // namespace coreneuron
