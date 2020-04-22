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

#ifndef _H_NRNSETUP_
#define _H_NRNSETUP_

#include <string>
#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/io/nrn_filehandler.hpp"
#include "coreneuron/io/nrn2core_direct.h"
#include "coreneuron/io/user_params.hpp"
#include "coreneuron/io/mem_layout_util.hpp"

namespace coreneuron {
static void read_phase1(FileHandler& F, int imult, NrnThread& nt);
static void read_phase2(FileHandler& F, int imult, NrnThread& nt, const UserParams& userParams);
static void read_phase3(FileHandler& F, int imult, NrnThread& nt);
static void read_phasegap(FileHandler& F, int imult, NrnThread& nt);
static void setup_ThreadData(NrnThread& nt);

// Functions to load and clean data;
extern void nrn_init_and_load_data(int argc,
                                   char** argv,
                                   bool is_mapping_needed = false,
                                   bool run_setup_cleanup = true);
extern void nrn_setup_cleanup();

extern int nrn_i_layout(int i, int cnt, int j, int size, int layout);

namespace coreneuron {

/// Reading phase number.
enum phase { one = 1, two, three, gap };

/// Get the phase number in form of the string.
template <phase P>
inline std::string getPhaseName();

template <>
inline std::string getPhaseName<one>() {
    return "1";
}

template <>
inline std::string getPhaseName<two>() {
    return "2";
}

template <>
inline std::string getPhaseName<three>() {
    return "3";
}

template <>
inline std::string getPhaseName<gap>() {
    return "gap";
}

/// Reading phase selector.
template <phase P>
inline void read_phase_aux(FileHandler& F, int imult, NrnThread& nt, const UserParams&);

template <>
inline void read_phase_aux<one>(FileHandler& F, int imult, NrnThread& nt, const UserParams&) {
    read_phase1(F, imult, nt);
}

template <>
inline void read_phase_aux<two>(FileHandler& F, int imult, NrnThread& nt, const UserParams& userParams) {
    read_phase2(F, imult, nt, userParams);
}

template <>
inline void read_phase_aux<three>(FileHandler& F, int imult, NrnThread& nt, const UserParams&) {
    read_phase3(F, imult, nt);
}

template <>
inline void read_phase_aux<gap>(FileHandler& F, int imult, NrnThread& nt, const UserParams&) {
    read_phasegap(F, imult, nt);
}

/// Reading phase wrapper for each neuron group.
template <phase P>
inline void* phase_wrapper_w(NrnThread* nt, const UserParams& userParams) {
    int i = nt->id;
    if (i < userParams.ngroup) {
        if (!userParams.in_memory_transfer) {
            const char* data_dir = userParams.path;
            // directory to read could be different for phase 2 if we are restoring
            // all other phases still read from dataset directory because the data
            // is constant
            if (P == 2) {
                data_dir = userParams.restore_path;
            }

            std::string fname = std::string(data_dir) + "/" + std::to_string(userParams.gidgroups[i]) + "_" + getPhaseName<P>() + ".dat";

            // Avoid trying to open the gid_gap.dat file if it doesn't exist when there are no
            // gap junctions in this gid
            if (P == gap && !userParams.file_reader[i].file_exist(fname.c_str())) {
                userParams.file_reader[i].close();
            } else {
                // if no file failed to open or not opened at all
                userParams.file_reader[i].open(fname.c_str());
            }
        }
        read_phase_aux<P>(userParams.file_reader[i], userParams.imult[i], *nt, userParams);
        if (!userParams.in_memory_transfer) {
            userParams.file_reader[i].close();
        }
        if (P == 2) {
            setup_ThreadData(*nt);
        }
    }
    return nullptr;
}

/// Specific phase reading executed by threads.
template <phase P>
inline static void phase_wrapper(UserParams& userParams, int direct = 0) {
    if (direct) {
        userParams.in_memory_transfer = true;
    }
    nrn_multithread_job(phase_wrapper_w<P>, userParams);
    if (direct) {
        userParams.in_memory_transfer = false;
    }
}
}  // namespace coreneuron
}  // namespace coreneuron
#endif
