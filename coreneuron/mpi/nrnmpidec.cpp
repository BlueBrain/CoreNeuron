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

#if NRNMPI
#include "nrnmpi.h"

namespace coreneuron {
mpi_function<cnrn_make_integral_constant_t(nrnmpi_newbuf_impl)> nrnmpi_newbuf{"nrnmpi_newbuf_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_copy_impl)> nrnmpi_copy{"nrnmpi_copy_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_ref_impl)> nrnmpi_ref{"nrnmpi_ref_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_unref_impl)> nrnmpi_unref{"nrnmpi_unref_impl"};

mpi_function<cnrn_make_integral_constant_t(nrnmpi_upkbegin_impl)> nrnmpi_upkbegin{
    "nrnmpi_upkbegin_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_getkey_impl)> nrnmpi_getkey{"nrnmpi_getkey_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_getid_impl)> nrnmpi_getid{"nrnmpi_getid_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_upkint_impl)> nrnmpi_upkint{"nrnmpi_upkint_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_upkdouble_impl)> nrnmpi_upkdouble{
    "nrnmpi_upkdouble_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_upkvec_impl)> nrnmpi_upkvec{"nrnmpi_upkvec_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_upkstr_impl)> nrnmpi_upkstr{"nrnmpi_upkstr_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_upkpickle_impl)> nrnmpi_upkpickle{
    "nrnmpi_upkpickle_impl"};

mpi_function<cnrn_make_integral_constant_t(nrnmpi_pkbegin_impl)> nrnmpi_pkbegin{
    "nrnmpi_pkbegin_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_enddata_impl)> nrnmpi_enddata{
    "nrnmpi_enddata_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_pkint_impl)> nrnmpi_pkint{"nrnmpi_pkint_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_pkdouble_impl)> nrnmpi_pkdouble{
    "nrnmpi_pkdouble_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_pkvec_impl)> nrnmpi_pkvec{"nrnmpi_pkvec_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_pkstr_impl)> nrnmpi_pkstr{"nrnmpi_pkstr_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_pkpickle_impl)> nrnmpi_pkpickle{
    "nrnmpi_pkpickle_impl"};

mpi_function<cnrn_make_integral_constant_t(nrnmpi_iprobe_impl)> nrnmpi_iprobe{"nrnmpi_iprobe_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_bbssend_impl)> nrnmpi_bbssend{
    "nrnmpi_bbssend_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_bbsrecv_impl)> nrnmpi_bbsrecv{
    "nrnmpi_bbsrecv_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_bbssendrecv_impl)> nrnmpi_bbssendrecv{
    "nrnmpi_bbssendrecv_impl"};

/* from nrnmpi.cpp */
mpi_function<cnrn_make_integral_constant_t(nrnmpi_init_impl)> nrnmpi_init{"nrnmpi_init_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_wrap_mpi_init_impl)> nrnmpi_wrap_mpi_init{
    "nrnmpi_wrap_mpi_init_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_finalize_impl)> nrnmpi_finalize{
    "nrnmpi_finalize_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_terminate_impl)> nrnmpi_terminate{
    "nrnmpi_terminate_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_subworld_size_impl)> nrnmpi_subworld_size{
    "nrnmpi_subworld_size_impl"};
mpi_function<cnrn_make_integral_constant_t(nrn_wrap_mpi_init_impl)> nrn_wrap_mpi_init{
    "nrn_wrap_mpi_init_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_check_threading_support_impl)>
    nrnmpi_check_threading_support{"nrnmpi_check_threading_support_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_write_file_impl)> nrnmpi_write_file{
    "nrnmpi_write_file_impl"};

/* from mpispike.c */
mpi_function<cnrn_make_integral_constant_t(nrnmpi_spike_initialize_impl)> nrnmpi_spike_initialize{
    "nrnmpi_spike_initialize_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_spike_exchange_impl)> nrnmpi_spike_exchange{
    "nrnmpi_spike_exchange_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_spike_exchange_compressed_impl)>
    nrnmpi_spike_exchange_compressed{"nrnmpi_spike_exchange_compressed_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_int_allmax_impl)> nrnmpi_int_allmax{
    "nrnmpi_int_allmax_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_int_gather_impl)> nrnmpi_int_gather{
    "nrnmpi_int_gather_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_int_gatherv_impl)> nrnmpi_int_gatherv{
    "nrnmpi_int_gatherv_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_int_allgather_impl)> nrnmpi_int_allgather{
    "nrnmpi_int_allgather_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_int_allgatherv_impl)> nrnmpi_int_allgatherv{
    "nrnmpi_int_allgatherv_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_int_alltoall_impl)> nrnmpi_int_alltoall{
    "nrnmpi_int_alltoall_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_int_alltoallv_impl)> nrnmpi_int_alltoallv{
    "nrnmpi_int_alltoallv_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_dbl_allgatherv_impl)> nrnmpi_dbl_allgatherv{
    "nrnmpi_dbl_allgatherv_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_dbl_alltoallv_impl)> nrnmpi_dbl_alltoallv{
    "nrnmpi_dbl_alltoallv_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_char_alltoallv_impl)> nrnmpi_char_alltoallv{
    "nrnmpi_char_alltoallv_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_dbl_broadcast_impl)> nrnmpi_dbl_broadcast{
    "nrnmpi_dbl_broadcast_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_int_broadcast_impl)> nrnmpi_int_broadcast{
    "nrnmpi_int_broadcast_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_char_broadcast_impl)> nrnmpi_char_broadcast{
    "nrnmpi_char_broadcast_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_int_sum_reduce_impl)> nrnmpi_int_sum_reduce{
    "nrnmpi_int_sum_reduce_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_assert_opstep_impl)> nrnmpi_assert_opstep{
    "nrnmpi_assert_opstep_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_dbl_allmin_impl)> nrnmpi_dbl_allmin{
    "nrnmpi_dbl_allmin_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_dbl_allmax_impl)> nrnmpi_dbl_allmax{
    "nrnmpi_dbl_allmax_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_pgvts_least_impl)> nrnmpi_pgvts_least{
    "nrnmpi_pgvts_least_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_send_doubles_impl)> nrnmpi_send_doubles{
    "nrnmpi_send_doubles_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_recv_doubles_impl)> nrnmpi_recv_doubles{
    "nrnmpi_recv_doubles_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_postrecv_doubles_impl)> nrnmpi_postrecv_doubles{
    "nrnmpi_postrecv_doubles_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_wait_impl)> nrnmpi_wait{"nrnmpi_wait_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_barrier_impl)> nrnmpi_barrier{
    "nrnmpi_barrier_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_dbl_allreduce_impl)> nrnmpi_dbl_allreduce{
    "nrnmpi_dbl_allreduce_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_long_allreduce_impl)> nrnmpi_long_allreduce{
    "nrnmpi_long_allreduce_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_dbl_allreduce_vec_impl)> nrnmpi_dbl_allreduce_vec{
    "nrnmpi_dbl_allreduce_vec_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_long_allreduce_vec_impl)>
    nrnmpi_long_allreduce_vec{"nrnmpi_long_allreduce_vec_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_dbl_allgather_impl)> nrnmpi_dbl_allgather{
    "nrnmpi_dbl_allgather_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_initialized_impl)> nrnmpi_initialized{
    "nrnmpi_initialized_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_abort_impl)> nrnmpi_abort{"nrnmpi_abort_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_wtime_impl)> nrnmpi_wtime{"nrnmpi_wtime_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_local_rank_impl)> nrnmpi_local_rank{
    "nrnmpi_local_rank_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_local_size_impl)> nrnmpi_local_size{
    "nrnmpi_local_size_impl"};
#if NRN_MULTISEND
mpi_function<cnrn_make_integral_constant_t(nrnmpi_multisend_comm_impl)> nrnmpi_multisend_comm{
    "nrnmpi_multisend_comm_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_multisend_impl)> nrnmpi_multisend{
    "nrnmpi_multisend_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_multisend_single_advance_impl)>
    nrnmpi_multisend_single_advance{"nrnmpi_multisend_single_advance_impl"};
mpi_function<cnrn_make_integral_constant_t(nrnmpi_multisend_conserve_impl)>
    nrnmpi_multisend_conserve{"nrnmpi_multisend_conserve_impl"};
#endif  // NRN_MULTISEND

}  // namespace coreneuron
#endif  // NRNMPI
