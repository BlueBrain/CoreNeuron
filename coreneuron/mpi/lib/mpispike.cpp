/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#include "coreneuron/nrnconf.h"
/* do not want the redef in the dynamic load case */
#include "coreneuron/mpi/nrnmpiuse.h"
#include "coreneuron/mpi/nrnmpi.h"
#include "coreneuron/mpi/nrnmpidec.h"
#include "nrnmpi_impl.h"
#include "nrnmpi.hpp"
#include "../mpispike.hpp"
#include "coreneuron/utils/profile/profiler_interface.h"
#include "coreneuron/utils/nrnoc_aux.hpp"

#if NRNMPI
#include <mpi.h>

#include <cstring>

namespace coreneuron {
static int np;
static int* displs;
static int* byteovfl; /* for the compressed transfer method */
static MPI_Datatype spike_type;

static void pgvts_op(double* in, double* inout, int* len, MPI_Datatype* dptr) {
    bool copy = false;
    if (*dptr != MPI_DOUBLE)
        printf("ERROR in mpispike.cpp! *dptr should be MPI_DOUBLE.");
    if (*len != 4)
        printf("ERROR in mpispike.cpp! *len should be 4.");
    if (in[0] < inout[0]) {
        /* least time has highest priority */
        copy = true;
    } else if (in[0] == inout[0]) {
        /* when times are equal then */
        if (in[1] < inout[1]) {
            /* NetParEvent done last */
            copy = true;
        } else if (in[1] == inout[1]) {
            /* when times and ops are equal then */
            if (in[2] < inout[2]) {
                /* init done next to last.*/
                copy = true;
            } else if (in[2] == inout[2]) {
                /* when times, ops, and inits are equal then */
                if (in[3] < inout[3]) {
                    /* choose lowest rank */
                    copy = true;
                }
            }
        }
    }
    if (copy) {
        for (int i = 0; i < 4; ++i) {
            inout[i] = in[i];
        }
    }
}

static MPI_Op mpi_pgvts_op;

void nrnmpi_spike_initialize() {
    NRNMPI_Spike s;
    int block_lengths[2] = {1, 1};
    MPI_Aint addresses[3];

    MPI_Get_address(&s, &addresses[0]);
    MPI_Get_address(&(s.gid), &addresses[1]);
    MPI_Get_address(&(s.spiketime), &addresses[2]);

    MPI_Aint displacements[2] = {addresses[1] - addresses[0], addresses[2] - addresses[1]};

    MPI_Datatype typelist[2] = {MPI_INT, MPI_DOUBLE};
    MPI_Type_create_struct(2, block_lengths, displacements, typelist, &spike_type);
    MPI_Type_commit(&spike_type);

    MPI_Op_create((MPI_User_function*) pgvts_op, 1, &mpi_pgvts_op);
}

#if nrn_spikebuf_size > 0

static MPI_Datatype spikebuf_type;

static void make_spikebuf_type() {
    NRNMPI_Spikebuf s;
    int block_lengths[3] = {1, nrn_spikebuf_size, nrn_spikebuf_size};
    MPI_Datatype typelist[3] = {MPI_INT_ MPI_INT, MPI_DOUBLE};

    MPI_Aint addresses[4];
    MPI_Get_address(&s, &addresses[0]);
    MPI_Get_address(&(s.nspike), &addresses[1]);
    MPI_Get_address(&(s.gid[0]), &addresses[2]);
    MPI_Get_address(&(s.spiketime[0]), &addresses[3]);

    MPI_Aint displacements[3] = {addresses[1] - addresses[0],
                                 addresses[2] - addresses[0],
                                 addresses[3] - addresses[0]};

    MPI_Type_create_struct(3, block_lengths, displacements, typelist, &spikebuf_type);
    MPI_Type_commit(&spikebuf_type);
}
#endif

void wait_before_spike_exchange() {
    MPI_Barrier(nrnmpi_comm);
}

int nrnmpi_spike_exchange_impl(int* nin,
                               NRNMPI_Spike* spikeout,
                               int icapacity,
                               NRNMPI_Spike* spikein) {
    Instrumentor::phase_begin("spike-exchange");

    {
        Instrumentor::phase p("imbalance");
        wait_before_spike_exchange();
    }

    Instrumentor::phase_begin("communication");
    if (!displs) {
        np = nrnmpi_numprocs_;
        displs = (int*) emalloc(np * sizeof(int));
        displs[0] = 0;
#if nrn_spikebuf_size > 0
        make_spikebuf_type();
#endif
    }
#if nrn_spikebuf_size == 0
    MPI_Allgather(&nout_, 1, MPI_INT, nin, 1, MPI_INT, nrnmpi_comm);
    int n = nin[0];
    for (int i = 1; i < np; ++i) {
        displs[i] = n;
        n += nin[i];
    }
    if (n) {
        if (icapacity < n) {
            icapacity = n + 10;
            free(spikein);
            spikein = (NRNMPI_Spike*) emalloc(icapacity * sizeof(NRNMPI_Spike));
        }
        MPI_Allgatherv(spikeout, nout_, spike_type, spikein, nin, displs, spike_type, nrnmpi_comm);
    }
#else
    MPI_Allgather(spbufout_, 1, spikebuf_type, spbufin_, 1, spikebuf_type, nrnmpi_comm);
    int novfl = 0;
    int n = spbufin_[0].nspike;
    if (n > nrn_spikebuf_size) {
        nin[0] = n - nrn_spikebuf_size;
        novfl += nin[0];
    } else {
        nin[0] = 0;
    }
    for (int i = 1; i < np; ++i) {
        displs[i] = novfl;
        int n1 = spbufin_[i].nspike;
        n += n1;
        if (n1 > nrn_spikebuf_size) {
            nin[i] = n1 - nrn_spikebuf_size;
            novfl += nin[i];
        } else {
            nin[i] = 0;
        }
    }
    if (novfl) {
        if (icapacity < novfl) {
            icapacity = novfl + 10;
            free(spikein);
            spikein = (NRNMPI_Spike*) hoc_Emalloc(icapacity * sizeof(NRNMPI_Spike));
            hoc_malchk();
        }
        int n1 = (nout_ > nrn_spikebuf_size) ? nout_ - nrn_spikebuf_size : 0;
        MPI_Allgatherv(spikeout, n1, spike_type, spikein, nin, displs, spike_type, nrnmpi_comm);
    }
    ovfl_ = novfl;
#endif
    Instrumentor::phase_end("communication");
    Instrumentor::phase_end("spike-exchange");
    return n;
}

/*
The compressed spike format is restricted to the fixed step method and is
a sequence of unsigned char.
nspike = buf[0]*256 + buf[1]
a sequence of spiketime, localgid pairs. There are nspike of them.
        spiketime is relative to the last transfer time in units of dt.
        note that this requires a mindelay < 256*dt.
        localgid is an unsigned int, unsigned short,
        or unsigned char in size depending on the range and thus takes
        4, 2, or 1 byte respectively. To be machine independent we do our
        own byte coding. When the localgid range is smaller than the true
        gid range, the gid->PreSyn are remapped into
        hostid specific	maps. If there are not many holes, i.e just about every
        spike from a source machine is delivered to some cell on a
        target machine, then instead of	a hash map, a vector is used.
The allgather sends the first part of the buf and the allgatherv buffer
sends any overflow.
*/
int nrnmpi_spike_exchange_compressed_impl(int localgid_size,
                                          unsigned char* spfixin_ovfl,
                                          int send_nspike,
                                          int* nin,
                                          int ovfl_capacity) {
    if (!displs) {
        np = nrnmpi_numprocs_;
        displs = (int*) emalloc(np * sizeof(int));
        displs[0] = 0;
        byteovfl = (int*) emalloc(np * sizeof(int));
    }

    MPI_Allgather(
        spfixout_, ag_send_size_, MPI_BYTE, spfixin_, ag_send_size_, MPI_BYTE, nrnmpi_comm);
    int novfl = 0;
    int ntot = 0;
    int bstot = 0;
    for (int i = 0; i < np; ++i) {
        displs[i] = bstot;
        int idx = i * ag_send_size_;
        int n = spfixin_[idx++] * 256;
        n += spfixin_[idx++];
        ntot += n;
        nin[i] = n;
        if (n > send_nspike) {
            int bs = 2 + n * (1 + localgid_size) - ag_send_size_;
            byteovfl[i] = bs;
            bstot += bs;
            novfl += n - send_nspike;
        } else {
            byteovfl[i] = 0;
        }
    }
    if (novfl) {
        if (ovfl_capacity < novfl) {
            ovfl_capacity = novfl + 10;
            free(spfixin_ovfl);
            spfixin_ovfl = (unsigned char*) emalloc(ovfl_capacity * (1 + localgid_size) *
                                                    sizeof(unsigned char));
        }
        int bs = byteovfl[nrnmpi_myid_];
        /*
        note that the spfixout_ buffer is one since the overflow
        is contiguous to the first part. But the spfixin_ovfl is
        completely separate from the spfixin_ since the latter
        dynamically changes its size during a run.
        */
        MPI_Allgatherv(spfixout_ + ag_send_size_,
                       bs,
                       MPI_BYTE,
                       spfixin_ovfl,
                       byteovfl,
                       displs,
                       MPI_BYTE,
                       nrnmpi_comm);
    }
    ovfl_ = novfl;
    return ntot;
}

int nrnmpi_int_allmax_impl(int x) {
    int result;
    MPI_Allreduce(&x, &result, 1, MPI_INT, MPI_MAX, nrnmpi_comm);
    return result;
}

extern void nrnmpi_int_gather_impl(int* s, int* r, int cnt, int root) {
    MPI_Gather(s, cnt, MPI_INT, r, cnt, MPI_INT, root, nrnmpi_comm);
}

extern void nrnmpi_int_gatherv_impl(int* s, int scnt, int* r, int* rcnt, int* rdispl, int root) {
    MPI_Gatherv(s, scnt, MPI_INT, r, rcnt, rdispl, MPI_INT, root, nrnmpi_comm);
}

extern void nrnmpi_int_alltoall_impl(int* s, int* r, int n) {
    MPI_Alltoall(s, n, MPI_INT, r, n, MPI_INT, nrnmpi_comm);
}

extern void nrnmpi_int_alltoallv_impl(const int* s,
                                      const int* scnt,
                                      const int* sdispl,
                                      int* r,
                                      int* rcnt,
                                      int* rdispl) {
    MPI_Alltoallv(s, scnt, sdispl, MPI_INT, r, rcnt, rdispl, MPI_INT, nrnmpi_comm);
}

extern void nrnmpi_dbl_alltoallv_impl(double* s,
                                      int* scnt,
                                      int* sdispl,
                                      double* r,
                                      int* rcnt,
                                      int* rdispl) {
    MPI_Alltoallv(s, scnt, sdispl, MPI_DOUBLE, r, rcnt, rdispl, MPI_DOUBLE, nrnmpi_comm);
}

extern void nrnmpi_char_alltoallv_impl(char* s,
                                       int* scnt,
                                       int* sdispl,
                                       char* r,
                                       int* rcnt,
                                       int* rdispl) {
    MPI_Alltoallv(s, scnt, sdispl, MPI_CHAR, r, rcnt, rdispl, MPI_CHAR, nrnmpi_comm);
}

/* following are for the partrans */

void nrnmpi_int_allgather_impl(int* s, int* r, int n) {
    MPI_Allgather(s, n, MPI_INT, r, n, MPI_INT, nrnmpi_comm);
}

void nrnmpi_int_allgatherv_impl(int* s, int* r, int* n, int* dspl) {
    MPI_Allgatherv(s, n[nrnmpi_myid_], MPI_INT, r, n, dspl, MPI_INT, nrnmpi_comm);
}

void nrnmpi_dbl_allgatherv_impl(double* s, double* r, int* n, int* dspl) {
    MPI_Allgatherv(s, n[nrnmpi_myid_], MPI_DOUBLE, r, n, dspl, MPI_DOUBLE, nrnmpi_comm);
}

void nrnmpi_dbl_broadcast_impl(double* buf, int cnt, int root) {
    MPI_Bcast(buf, cnt, MPI_DOUBLE, root, nrnmpi_comm);
}

void nrnmpi_int_broadcast_impl(int* buf, int cnt, int root) {
    MPI_Bcast(buf, cnt, MPI_INT, root, nrnmpi_comm);
}

void nrnmpi_char_broadcast_impl(char* buf, int cnt, int root) {
    MPI_Bcast(buf, cnt, MPI_CHAR, root, nrnmpi_comm);
}

int nrnmpi_int_sum_reduce_impl(int in) {
    int result;
    MPI_Allreduce(&in, &result, 1, MPI_INT, MPI_SUM, nrnmpi_comm);
    return result;
}

void nrnmpi_assert_opstep_impl(int opstep, double tt) {
    /* all machines in comm should have same opstep and same tt. */
    double buf[2] = {(double) opstep, tt};
    MPI_Bcast(buf, 2, MPI_DOUBLE, 0, nrnmpi_comm);
    if (opstep != (int) buf[0] || tt != buf[1]) {
        printf("%d opstep=%d %d  t=%g t-troot=%g\n",
               nrnmpi_myid_,
               opstep,
               (int) buf[0],
               tt,
               tt - buf[1]);
        hoc_execerror("nrnmpi_assert_opstep failed", (char*) 0);
    }
}

double nrnmpi_dbl_allmin_impl(double x) {
    double result;
    MPI_Allreduce(&x, &result, 1, MPI_DOUBLE, MPI_MIN, nrnmpi_comm);
    return result;
}

double nrnmpi_dbl_allmax_impl(double x) {
    double result;
    MPI_Allreduce(&x, &result, 1, MPI_DOUBLE, MPI_MAX, nrnmpi_comm);
    return result;
}

int nrnmpi_pgvts_least_impl(double* tt, int* op, int* init) {
    double ibuf[4], obuf[4];
    ibuf[0] = *tt;
    ibuf[1] = (double) (*op);
    ibuf[2] = (double) (*init);
    ibuf[3] = (double) nrnmpi_myid_;
    std::memcpy(obuf, ibuf, 4 * sizeof(double));

    MPI_Allreduce(ibuf, obuf, 4, MPI_DOUBLE, mpi_pgvts_op, nrnmpi_comm);
    assert(obuf[0] <= *tt);
    if (obuf[0] == *tt) {
        assert((int) obuf[1] <= *op);
        if ((int) obuf[1] == *op) {
            assert((int) obuf[2] <= *init);
            if ((int) obuf[2] == *init) {
                assert((int) obuf[3] <= nrnmpi_myid_);
            }
        }
    }
    *tt = obuf[0];
    *op = (int) obuf[1];
    *init = (int) obuf[2];
    if (nrnmpi_myid_ == (int) obuf[3]) {
        return 1;
    }
    return 0;
}

/* following for splitcell.cpp transfer */
void nrnmpi_send_doubles_impl(double* pd, int cnt, int dest, int tag) {
    MPI_Send(pd, cnt, MPI_DOUBLE, dest, tag, nrnmpi_comm);
}

void nrnmpi_recv_doubles_impl(double* pd, int cnt, int src, int tag) {
    MPI_Status status;
    MPI_Recv(pd, cnt, MPI_DOUBLE, src, tag, nrnmpi_comm, &status);
}

void nrnmpi_postrecv_doubles_impl(double* pd, int cnt, int src, int tag, void** request) {
    MPI_Irecv(pd, cnt, MPI_DOUBLE, src, tag, nrnmpi_comm, (MPI_Request*) request);
}

void nrnmpi_wait_impl(void** request) {
    MPI_Status status;
    MPI_Wait((MPI_Request*) request, &status);
}

void nrnmpi_barrier_impl() {
    MPI_Barrier(nrnmpi_comm);
}

double nrnmpi_dbl_allreduce_impl(double x, int type) {
    double result;
    MPI_Op tt;
    if (type == 1) {
        tt = MPI_SUM;
    } else if (type == 2) {
        tt = MPI_MAX;
    } else {
        tt = MPI_MIN;
    }
    MPI_Allreduce(&x, &result, 1, MPI_DOUBLE, tt, nrnmpi_comm);
    return result;
}

long nrnmpi_long_allreduce_impl(long x, int type) {
    long result;
    MPI_Op tt;
    if (type == 1) {
        tt = MPI_SUM;
    } else if (type == 2) {
        tt = MPI_MAX;
    } else {
        tt = MPI_MIN;
    }
    MPI_Allreduce(&x, &result, 1, MPI_LONG, tt, nrnmpi_comm);
    return result;
}

void nrnmpi_dbl_allreduce_vec_impl(double* src, double* dest, int cnt, int type) {
    MPI_Op tt;
    assert(src != dest);
    if (type == 1) {
        tt = MPI_SUM;
    } else if (type == 2) {
        tt = MPI_MAX;
    } else {
        tt = MPI_MIN;
    }
    MPI_Allreduce(src, dest, cnt, MPI_DOUBLE, tt, nrnmpi_comm);
    return;
}

void nrnmpi_long_allreduce_vec_impl(long* src, long* dest, int cnt, int type) {
    MPI_Op tt;
    assert(src != dest);
    if (type == 1) {
        tt = MPI_SUM;
    } else if (type == 2) {
        tt = MPI_MAX;
    } else {
        tt = MPI_MIN;
    }
    MPI_Allreduce(src, dest, cnt, MPI_LONG, tt, nrnmpi_comm);
    return;
}

void nrnmpi_dbl_allgather_impl(double* s, double* r, int n) {
    MPI_Allgather(s, n, MPI_DOUBLE, r, n, MPI_DOUBLE, nrnmpi_comm);
}

#if NRN_MULTISEND

static MPI_Comm multisend_comm;

void nrnmpi_multisend_comm_impl() {
    if (!multisend_comm) {
        MPI_Comm_dup(MPI_COMM_WORLD, &multisend_comm);
    }
}

void nrnmpi_multisend_impl(NRNMPI_Spike* spk, int n, int* hosts) {
    MPI_Request r;
    for (int i = 0; i < n; ++i) {
        MPI_Isend(spk, 1, spike_type, hosts[i], 1, multisend_comm, &r);
        MPI_Request_free(&r);
    }
}

int nrnmpi_multisend_single_advance_impl(NRNMPI_Spike* spk) {
    int flag = 0;
    MPI_Status status;
    MPI_Iprobe(MPI_ANY_SOURCE, 1, multisend_comm, &flag, &status);
    if (flag) {
        MPI_Recv(spk, 1, spike_type, MPI_ANY_SOURCE, 1, multisend_comm, &status);
    }
    return flag;
}

int nrnmpi_multisend_conserve_impl(int nsend, int nrecv) {
    int tcnts[2];
    tcnts[0] = nsend - nrecv;
    MPI_Allreduce(tcnts, tcnts + 1, 1, MPI_INT, MPI_SUM, multisend_comm);
    return tcnts[1];
}

#endif /*NRN_MULTISEND*/
}  // namespace coreneuron
#endif /*NRNMPI*/