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

namespace coreneuron {
struct UserParams {
    bool do_not_open;
    /// Number of local cell groups
    int ngroup;
    /// Array of cell group numbers (indices)
    int* gidgroups;
    /// Array of duplicate indices. Normally, with nrn_setup_multiple=1,
    //   they are ngroup values of 0.
    int* imult;
    const char* path;
    const char* restore_path;
    FileHandler* file_reader;
    bool byte_swap;
};

struct Phase1 {
    public:
    void read_direct(int thread_id);
    void read_file(FileHandler& F);
    void populate(NrnThread& nt, int imult);

    private:
    void shift_gids(int imult);
    void add_extracon(NrnThread& nt, int imult);

    std::vector<int> output_gids;
    std::vector<int> netcon_srcgids;
};

struct Phase2 {
    public:
    void read_direct(int thread_id, const NrnThread& nt);
    void read_file(FileHandler& F, const NrnThread& nt);
    void populate(NrnThread& nt, int imult, const UserParams& userParams);

    private:
    int n_output;
    int n_real_output;
    int n_node;
    int n_diam;  // 0 if not needed, else n_node
    int n_mech;
    std::vector<int> types;
    std::vector<int> nodecounts;
    int n_idata;
    int n_vdata;
    int n_weight;
    std::vector<int> v_parent_index;
    std::vector<double> actual_a;
    std::vector<double> actual_b;
    std::vector<double> actual_area;
    std::vector<double> actual_v;
    std::vector<double> actual_diam;
    struct TML {
        std::vector<int> nodeindices;
        std::vector<double> data;
        std::vector<int> pdata;
        int type;
        std::vector<int> iArray;
        std::vector<double> dArray;
    };
    std::vector<TML> tmls;
    std::vector<int> output_vindex;
    std::vector<double> output_threshold;
    std::vector<int> pnttype;
    std::vector<int> pntindex;
    std::vector<double> weights;
    std::vector<double> delay;
    int npnt;
    struct VecPlayContinuous2 {
        int vtype;
        int mtype;
        int ix;
        std::vector<double> yvec;
        std::vector<double> tvec;
    };
    std::vector<VecPlayContinuous2> vecPlayContinuous;
};

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
    Phase1 p1;
    p1.read_file(F);
    p1.populate(nt, imult);
}

template <>
inline void read_phase_aux<two>(FileHandler& F, int imult, NrnThread& nt, const UserParams& userParams) {
    Phase2 p2;
    if (corenrn_embedded) {
        p2.read_direct(nt.id, nt);
    } else {
        p2.read_file(F, nt);
    }
    p2.populate(nt, imult, userParams);
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
    char fnamebuf[1000];
    if (i < userParams.ngroup) {
        if (!userParams.do_not_open) {
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
                userParams.file_reader[i].open(fname.c_str(), userParams.byte_swap);
            }
        }
        read_phase_aux<P>(userParams.file_reader[i], userParams.imult[i], *nt, userParams);
        if (!userParams.do_not_open) {
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
        userParams.do_not_open = true;
    }
    nrn_multithread_job(phase_wrapper_w<P>, userParams);
    if (direct) {
        userParams.do_not_open = false;
    }
}
}  // namespace coreneuron
}  // namespace coreneuron
#endif
