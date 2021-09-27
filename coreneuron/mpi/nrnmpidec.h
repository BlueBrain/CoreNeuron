/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

/*
This file is processed by mkdynam.sh and so it is important that
the prototypes be of the form "type foo(type arg, ...)"
*/

#ifndef nrnmpidec_h
#define nrnmpidec_h


#if NRNMPI
#include <stdlib.h>

namespace coreneuron {
/* from bbsmpipack.c */
struct bbsmpibuf {
    char* buf;
    int size;
    int pkposition;
    int upkpos;
    int keypos;
    int refcount;
};

extern bbsmpibuf* nrnmpi_newbuf_impl(int size);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_newbuf_impl)> nrnmpi_newbuf;
extern void nrnmpi_copy_impl(bbsmpibuf* dest, bbsmpibuf* src);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_copy_impl)> nrnmpi_copy;
extern void nrnmpi_ref_impl(bbsmpibuf* buf);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_ref_impl)> nrnmpi_ref;
extern void nrnmpi_unref_impl(bbsmpibuf* buf);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_unref_impl)> nrnmpi_unref;

extern void nrnmpi_upkbegin_impl(bbsmpibuf* buf);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_upkbegin_impl)> nrnmpi_upkbegin;
extern char* nrnmpi_getkey_impl(bbsmpibuf* buf);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_getkey_impl)> nrnmpi_getkey;
extern int nrnmpi_getid_impl(bbsmpibuf* buf);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_getid_impl)> nrnmpi_getid;
extern int nrnmpi_upkint_impl(bbsmpibuf* buf);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_upkint_impl)> nrnmpi_upkint;
extern double nrnmpi_upkdouble_impl(bbsmpibuf* buf);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_upkdouble_impl)> nrnmpi_upkdouble;
extern void nrnmpi_upkvec_impl(int n, double* x, bbsmpibuf* buf);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_upkvec_impl)> nrnmpi_upkvec;
extern char* nrnmpi_upkstr_impl(bbsmpibuf* buf);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_upkstr_impl)> nrnmpi_upkstr;
extern char* nrnmpi_upkpickle_impl(size_t* size, bbsmpibuf* buf);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_upkpickle_impl)> nrnmpi_upkpickle;

extern void nrnmpi_pkbegin_impl(bbsmpibuf* buf);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_pkbegin_impl)> nrnmpi_pkbegin;
extern void nrnmpi_enddata_impl(bbsmpibuf* buf);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_enddata_impl)> nrnmpi_enddata;
extern void nrnmpi_pkint_impl(int i, bbsmpibuf* buf);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_pkint_impl)> nrnmpi_pkint;
extern void nrnmpi_pkdouble_impl(double x, bbsmpibuf* buf);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_pkdouble_impl)> nrnmpi_pkdouble;
extern void nrnmpi_pkvec_impl(int n, double* x, bbsmpibuf* buf);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_pkvec_impl)> nrnmpi_pkvec;
extern void nrnmpi_pkstr_impl(const char* s, bbsmpibuf* buf);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_pkstr_impl)> nrnmpi_pkstr;
extern void nrnmpi_pkpickle_impl(const char* s, size_t size, bbsmpibuf* buf);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_pkpickle_impl)> nrnmpi_pkpickle;

extern int nrnmpi_iprobe_impl(int* size, int* tag, int* source);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_iprobe_impl)> nrnmpi_iprobe;
extern void nrnmpi_bbssend_impl(int dest, int tag, bbsmpibuf* r);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_bbssend_impl)> nrnmpi_bbssend;
extern int nrnmpi_bbsrecv_impl(int source, bbsmpibuf* r);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_bbsrecv_impl)> nrnmpi_bbsrecv;
extern int nrnmpi_bbssendrecv_impl(int dest, int tag, bbsmpibuf* s, bbsmpibuf* r);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_bbssendrecv_impl)> nrnmpi_bbssendrecv;

/* from nrnmpi.cpp */
extern void nrnmpi_init_impl(int* pargc, char*** pargv);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_init_impl)> nrnmpi_init;
extern int nrnmpi_wrap_mpi_init_impl(int* flag);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_wrap_mpi_init_impl)> nrnmpi_wrap_mpi_init;
extern void nrnmpi_finalize_impl(void);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_finalize_impl)> nrnmpi_finalize;
extern void nrnmpi_terminate_impl();
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_terminate_impl)> nrnmpi_terminate;
extern void nrnmpi_subworld_size_impl(int n);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_subworld_size_impl)> nrnmpi_subworld_size;
extern int nrn_wrap_mpi_init_impl(int* flag);
extern mpi_function<cnrn_make_integral_constant_t(nrn_wrap_mpi_init_impl)> nrn_wrap_mpi_init;
extern void nrnmpi_check_threading_support_impl();
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_check_threading_support_impl)>
    nrnmpi_check_threading_support;
// Write given buffer to a new file using MPI collective I/O
extern void nrnmpi_write_file_impl(const std::string& filename, const char* buffer, size_t length);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_write_file_impl)> nrnmpi_write_file;


/* from mpispike.c */
extern void nrnmpi_spike_initialize_impl(void);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_spike_initialize_impl)>
    nrnmpi_spike_initialize;
extern int nrnmpi_spike_exchange_impl(void);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_spike_exchange_impl)>
    nrnmpi_spike_exchange;
extern int nrnmpi_spike_exchange_compressed_impl(void);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_spike_exchange_compressed_impl)>
    nrnmpi_spike_exchange_compressed;
extern int nrnmpi_int_allmax_impl(int i);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_int_allmax_impl)> nrnmpi_int_allmax;
extern void nrnmpi_int_gather_impl(int* s, int* r, int cnt, int root);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_int_gather_impl)> nrnmpi_int_gather;
extern void nrnmpi_int_gatherv_impl(int* s, int scnt, int* r, int* rcnt, int* rdispl, int root);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_int_gatherv_impl)> nrnmpi_int_gatherv;
extern void nrnmpi_int_allgather_impl(int* s, int* r, int n);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_int_allgather_impl)> nrnmpi_int_allgather;
extern void nrnmpi_int_allgatherv_impl(int* s, int* r, int* n, int* dspl);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_int_allgatherv_impl)>
    nrnmpi_int_allgatherv;
extern void nrnmpi_int_alltoall_impl(int* s, int* r, int n);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_int_alltoall_impl)> nrnmpi_int_alltoall;
extern void nrnmpi_int_alltoallv_impl(const int* s,
                                      const int* scnt,
                                      const int* sdispl,
                                      int* r,
                                      int* rcnt,
                                      int* rdispl);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_int_alltoallv_impl)> nrnmpi_int_alltoallv;
extern void nrnmpi_dbl_allgatherv_impl(double* s, double* r, int* n, int* dspl);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_dbl_allgatherv_impl)>
    nrnmpi_dbl_allgatherv;
extern void nrnmpi_dbl_alltoallv_impl(double* s,
                                      int* scnt,
                                      int* sdispl,
                                      double* r,
                                      int* rcnt,
                                      int* rdispl);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_dbl_alltoallv_impl)> nrnmpi_dbl_alltoallv;
extern void nrnmpi_char_alltoallv_impl(char* s,
                                       int* scnt,
                                       int* sdispl,
                                       char* r,
                                       int* rcnt,
                                       int* rdispl);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_char_alltoallv_impl)>
    nrnmpi_char_alltoallv;
extern void nrnmpi_dbl_broadcast_impl(double* buf, int cnt, int root);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_dbl_broadcast_impl)> nrnmpi_dbl_broadcast;
extern void nrnmpi_int_broadcast_impl(int* buf, int cnt, int root);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_int_broadcast_impl)> nrnmpi_int_broadcast;
extern void nrnmpi_char_broadcast_impl(char* buf, int cnt, int root);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_char_broadcast_impl)>
    nrnmpi_char_broadcast;
extern int nrnmpi_int_sum_reduce_impl(int in);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_int_sum_reduce_impl)>
    nrnmpi_int_sum_reduce;
extern void nrnmpi_assert_opstep_impl(int opstep, double t);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_assert_opstep_impl)> nrnmpi_assert_opstep;
extern double nrnmpi_dbl_allmin_impl(double x);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_dbl_allmin_impl)> nrnmpi_dbl_allmin;
extern double nrnmpi_dbl_allmax_impl(double x);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_dbl_allmax_impl)> nrnmpi_dbl_allmax;
extern int nrnmpi_pgvts_least_impl(double* t, int* op, int* init);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_pgvts_least_impl)> nrnmpi_pgvts_least;
extern void nrnmpi_send_doubles_impl(double* pd, int cnt, int dest, int tag);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_send_doubles_impl)> nrnmpi_send_doubles;
extern void nrnmpi_recv_doubles_impl(double* pd, int cnt, int src, int tag);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_recv_doubles_impl)> nrnmpi_recv_doubles;
extern void nrnmpi_postrecv_doubles_impl(double* pd, int cnt, int src, int tag, void** request);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_postrecv_doubles_impl)>
    nrnmpi_postrecv_doubles;
extern void nrnmpi_wait_impl(void** request);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_wait_impl)> nrnmpi_wait;
extern void nrnmpi_barrier_impl(void);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_barrier_impl)> nrnmpi_barrier;
extern double nrnmpi_dbl_allreduce_impl(double x, int type);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_dbl_allreduce_impl)> nrnmpi_dbl_allreduce;
extern long nrnmpi_long_allreduce_impl(long x, int type);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_long_allreduce_impl)>
    nrnmpi_long_allreduce;
extern void nrnmpi_dbl_allreduce_vec_impl(double* src, double* dest, int cnt, int type);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_dbl_allreduce_vec_impl)>
    nrnmpi_dbl_allreduce_vec;
extern void nrnmpi_long_allreduce_vec_impl(long* src, long* dest, int cnt, int type);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_long_allreduce_vec_impl)>
    nrnmpi_long_allreduce_vec;
extern void nrnmpi_dbl_allgather_impl(double* s, double* r, int n);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_dbl_allgather_impl)> nrnmpi_dbl_allgather;
extern int nrnmpi_initialized_impl();
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_initialized_impl)> nrnmpi_initialized;
extern void nrnmpi_abort_impl(int);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_abort_impl)> nrnmpi_abort;
extern double nrnmpi_wtime_impl();
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_wtime_impl)> nrnmpi_wtime;
extern int nrnmpi_local_rank_impl();
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_local_rank_impl)> nrnmpi_local_rank;
extern int nrnmpi_local_size_impl();
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_local_size_impl)> nrnmpi_local_size;
#if NRN_MULTISEND
extern void nrnmpi_multisend_comm_impl();
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_multisend_comm_impl)>
    nrnmpi_multisend_comm;
extern void nrnmpi_multisend_impl(NRNMPI_Spike* spk, int n, int* hosts);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_multisend_impl)> nrnmpi_multisend;
extern int nrnmpi_multisend_single_advance_impl(NRNMPI_Spike* spk);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_multisend_single_advance_impl)>
    nrnmpi_multisend_single_advance;
extern int nrnmpi_multisend_conserve_impl(int nsend, int nrecv);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_multisend_conserve_impl)>
    nrnmpi_multisend_conserve;
#endif

}  // namespace coreneuron
#endif
#endif
