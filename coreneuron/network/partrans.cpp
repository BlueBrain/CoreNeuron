#include "coreneuron/nrnconf.h"
#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/mpi/nrnmpi.h"
#include "coreneuron/network/partrans.hpp"

// This is the computational code for src->target transfer (e.g. gap junction)
// simulation.
// The setup code is in partrans_setup.cpp

namespace coreneuron {
bool nrn_have_gaps;

using namespace nrn_partrans;

TransferThreadData* nrn_partrans::transfer_thread_data_;

// TODO: Where should those go?
// MPI_Alltoallv buffer info
double* nrn_partrans::insrc_buf_;   // Receive buffer for gap voltages
double* nrn_partrans::outsrc_buf_;  // Send buffer for gap voltages
int* nrn_partrans::insrccnt_;
int* nrn_partrans::insrcdspl_;
int* nrn_partrans::outsrccnt_;
int* nrn_partrans::outsrcdspl_;

void nrnmpi_v_transfer() {
    // copy source voltages to outsrc_buf_
    // note that same voltage may get copied to several locations in outsrc_buf

    // gather the source values. can be done in parallel
    for (int tid = 0; tid < nrn_nthread; ++tid) {
        auto& ttd = transfer_thread_data_[tid];
        auto& nt = nrn_threads[tid];
        int n = int(ttd.outsrc_indices.size());
        if (n == 0) {
            continue;
        }
        double* src_data = nt._data;
        int* src_indices = ttd.src_indices.data();

        // gather sources on gpu and copy to cpu, cpu scatters to outsrc_buf
        double* src_val = ttd.src_gather.data();
        int n_src_val = int(ttd.src_gather.size());
// clang-format off
        #pragma acc parallel loop present(                        \
            src_indices[0:n_src_val], src_data[0:nt._ndata],      \
            src_val[0 : n_src_val]) /*copyout(vg[0:n_src_val])*/  \
            if (nt.compute_gpu) async(nt.stream_id)
        for (int i = 0; i < n_src_val; ++i) {
            src_val[i] = src_data[src_indices[i]];
        }
        // do not know why the copyout above did not work
        // and the following update is needed
        #pragma acc update host(src_val[0 : n])     \
            if (nrn_threads[0].compute_gpu)         \
            async(nt.stream_id)
        // clang-format on
    }

    // copy source values to outsrc_buf_
    for (int tid = 0; tid < nrn_nthread; ++tid) {
// clang-format off
        #pragma acc wait(nrn_threads[tid].stream_id)
        // clang-format on
        TransferThreadData& ttd = transfer_thread_data_[tid];
        int n = int(ttd.outsrc_indices.size());
        int* outsrc_indices = ttd.outsrc_indices.data();
        double* gather_val = ttd.src_gather.data();
        int* gather_indices = ttd.gather2outsrc_indices.data();
        for (int i = 0; i < n; ++i) {
            outsrc_buf_[outsrc_indices[i]] = gather_val[gather_indices[i]];
//printf("t=%g rank=%d tid=%d i=%d outsrc_buf %d %g  gather_indices %d\n",
//nrn_threads[tid]._t, nrnmpi_myid, tid, i, outsrc_indices[i],
//outsrc_buf_[outsrc_indices[i]], gather_indices[i]);
        }
    }

// transfer
#if NRNMPI
    if (nrnmpi_numprocs > 1) {  // otherwise insrc_buf_ == outsrc_buf_
        nrnmpi_barrier();
        nrnmpi_dbl_alltoallv(outsrc_buf_, outsrccnt_, outsrcdspl_, insrc_buf_, insrccnt_,
                             insrcdspl_);
    } else
#endif
    {  // actually use the multiprocess code even for one process to aid debugging
        for (int i = 0; i < outsrcdspl_[1]; ++i) {
            insrc_buf_[i] = outsrc_buf_[i];
        }
    }

// insrc_buf_ will get copied to targets via nrnthread_v_transfer
// clang-format off
    #pragma acc update device(                      \
        insrc_buf_[0:insrcdspl_[nrnmpi_numprocs]])  \
        if (nrn_threads[0].compute_gpu)
    // clang-format on
}

void nrnthread_v_transfer(NrnThread* _nt) {
    TransferThreadData& ttd = transfer_thread_data_[_nt->id];
    int ntar = int(ttd.tar_indices.size());
    int* tar_indices = ttd.tar_indices.data();
    int* insrc_indices = ttd.insrc_indices.data();
    double* tar_data = _nt->_data;

// clang-format off
    #pragma acc parallel loop present(              \
        insrc_indices[0:ntar],             \
        tar_data[0:nt._ndata],                      \
        insrc_buf_[0:insrcdspl_[nrnmpi_numprocs]])  \
    if (_nt->compute_gpu)                       \
        async(_nt->stream_id)
    // clang-format on
    for (int i = 0; i < ntar; ++i) {
        tar_data[tar_indices[i]] = insrc_buf_[insrc_indices[i]];
    }
}

void nrn_partrans::gap_update_indices() {
    if (insrcdspl_) {
// clang-format off
        #pragma acc enter data create(                  \
            insrc_buf_[0:insrcdspl_[nrnmpi_numprocs]])  \
            if (nrn_threads[0].compute_gpu)
        // clang-format on
    }
    for (int tid = 0; tid < nrn_nthread; ++tid) {
        TransferThreadData& ttd = transfer_thread_data_[tid];

        int n = int(ttd.src_indices.size());
        int ng = int(ttd.src_gather.size());
        if (n) {
// clang-format off
            #pragma acc enter data copyin(ttd.src_indices.data()[0 : n]) if (nrn_threads[0].compute_gpu)
            #pragma acc enter data create(ttd.src_gather.data[0 : ng]) if (nrn_threads[0].compute_gpu)
            // clang-format on
        }

        if (ttd.insrc_indices.size()) {
// clang-format off
            #pragma acc enter data copyin(ttd.insrc_indices.data()[0 : ttd.insrc_indices.size()]) if (nrn_threads[0].compute_gpu)
            // clang-format on
        }
    }
}
}  // namespace coreneuron
