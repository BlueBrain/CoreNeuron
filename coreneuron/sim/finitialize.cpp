/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#include "coreneuron/nrnconf.h"
#include "coreneuron/network/netpar.hpp"
#include "coreneuron/network/netcvode.hpp"
#include "coreneuron/sim/fast_imem.hpp"
#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/utils/profile/profiler_interface.h"
#include "coreneuron/coreneuron.hpp"
#include "coreneuron/utils/nrnoc_aux.hpp"
#include "coreneuron/io/mem_layout_util.hpp"  // for WATCH use of nrn_i_layout
#include "coreneuron/utils/vrecitem.h"
#include "coreneuron/io/core2nrn_data_return.hpp"

namespace coreneuron {

void nrn_finitialize(int setv, double v) {
    Instrumentor::phase_begin("finitialize");
    t = 0.;
    dt2thread(-1.);
    nrn_thread_table_check();
    clear_event_queue();
    nrn_spike_exchange_init();
#if VECTORIZE
    nrn_play_init(); /* Vector.play */
                     /// Play events should be executed before initializing events
    for (int i = 0; i < nrn_nthread; ++i) {
        nrn_deliver_events(nrn_threads + i); /* The play events at t=0 */
    }
    if (setv) {
        for (auto _nt = nrn_threads; _nt < nrn_threads + nrn_nthread; ++_nt) {
            double* vec_v = &(VEC_V(0));
            // clang-format off

            #pragma acc parallel loop present(      \
                _nt[0:1], vec_v[0:_nt->end])        \
                if (_nt->compute_gpu)
            // clang-format on
            for (int i = 0; i < _nt->end; ++i) {
                vec_v[i] = v;
            }
        }
    }

    if (nrn_have_gaps) {
        nrnmpi_v_transfer();
        for (int i = 0; i < nrn_nthread; ++i) {
            nrnthread_v_transfer(nrn_threads + i);
        }
    }

    for (int i = 0; i < nrn_nthread; ++i) {
        nrn_ba(nrn_threads + i, BEFORE_INITIAL);
    }
    /* the INITIAL blocks are ordered so that mechanisms that write
       concentrations are after ions and before mechanisms that read
       concentrations.
    */
    /* the memblist list in NrnThread is already so ordered */
    for (int i = 0; i < nrn_nthread; ++i) {
        NrnThread* nt = nrn_threads + i;
        for (auto tml = nt->tml; tml; tml = tml->next) {
            mod_f_t s = corenrn.get_memb_func(tml->index).initialize;
            if (s) {
                (*s)(nt, tml->ml, tml->index);
            }
        }
    }
#endif

    init_net_events();
    for (int i = 0; i < nrn_nthread; ++i) {
        nrn_ba(nrn_threads + i, AFTER_INITIAL);
    }
    for (int i = 0; i < nrn_nthread; ++i) {
        nrn_deliver_events(nrn_threads + i); /* The INITIAL sent events at t=0 */
    }
    for (int i = 0; i < nrn_nthread; ++i) {
        setup_tree_matrix_minimal(nrn_threads + i);
        if (nrn_use_fast_imem) {
            nrn_calc_fast_imem(nrn_threads + i);
        }
    }
    for (int i = 0; i < nrn_nthread; ++i) {
        nrn_ba(nrn_threads + i, BEFORE_STEP);
    }
    for (int i = 0; i < nrn_nthread; ++i) {
        nrn_deliver_events(nrn_threads + i); /* The record events at t=0 */
    }
#if NRNMPI
    nrn_spike_exchange(nrn_threads);
#endif
    nrncore2nrn_send_init();
    for (int i = 0; i < nrn_nthread; ++i) {
        nrncore2nrn_send_values(nrn_threads + i);
    }
    Instrumentor::phase_end("finitialize");
}

// helper functions defined below.
static void nrn2core_tqueue();
static void watch_activate_clear();
static void nrn2core_transfer_watch_condition(int, int, int, int, int);
static void vec_play_activate();

extern "C" {
/** Pointer to function in NEURON that iterates over activated
    WATCH statements, sending each item to ...
**/
void (*nrn2core_transfer_watch_)(void (*cb)(int, int, int, int, int));
}

/**
  All state from NEURON necessary to continue a run.

  In NEURON direct mode, we desire the exact behavior of
  ParallelContext.psolve(tstop). I.e. a sequence of such calls with and
  without intervening calls to h.finitialize(). Most state (structure
  and data of the substantive model) has been copied
  from NEURON during nrn_setup. Now we need to copy the event queue
  and set up any other invalid internal structures. I.e basically the
  nrn_finitialize above but without changing any simulation data. We follow
  some of the strategy of checkpoint_initialize.
**/
void direct_mode_initialize() {
    dt2thread(-1.);
    nrn_thread_table_check();

    // Reproduce present NEURON WATCH activation
    // Start from nothing active.
    watch_activate_clear();
    // nrn2core_transfer_watch_condition(...) receives the WATCH activation info
    // on a per active WatchCondition basis from NEURON.
    (*nrn2core_transfer_watch_)(nrn2core_transfer_watch_condition);

    nrn_spike_exchange_init();

    // in case some nrn_init allocate data we need to do that but do not
    // want to call initmodel.
    _nrn_skip_initmodel = true;
    for (int i = 0; i < nrn_nthread; ++i) {  // should be parallel
        NrnThread& nt = nrn_threads[i];
        for (NrnThreadMembList* tml = nt.tml; tml; tml = tml->next) {
            Memb_list* ml = tml->ml;
            mod_f_t s = corenrn.get_memb_func(tml->index).initialize;
            if (s) {
                (*s)(&nt, ml, tml->index);
            }
        }
    }
    _nrn_skip_initmodel = false;

    // the things done by checkpoint restore at the end of Phase2::read_file
    // vec_play_continuous n_vec_play_continuous of them
    // patstim_index
    // preSynConditionEventFlags nt.n_presyn of them
    // restore_events
    // restore_events
    // the things done for checkpoint at the end of Phase2::populate
    // checkpoint_restore_tqueue
    // Lastly, if PatternStim exists, needs initialization
    // checkpoint_restore_patternstim
    // io/nrn_checkpoint.cpp: write_tqueue contains examples for each
    // DiscreteEvent type with regard to the information needed for each
    // subclass from the point of view of CoreNEURON.
    // E.g. for NetConType_, just netcon_index
    // The trick, then, is to figure out the CoreNEURON info from the
    // NEURON queue items and that should be available in passing from
    // the existing processing of nrncore_write.

    // activate the vec_play_continuous events defined in phase2 setup.
    vec_play_activate();

    // Any PreSyn.flag_ == 1 on the NEURON side needs to be transferred
    // or the PreSyn will spuriously fire when psolve starts.
    extern void nrn2core_PreSyn_flag_receive(int tid);
    for (int tid = 0; tid < nrn_nthread; ++tid) {
        nrn2core_PreSyn_flag_receive(tid);
    }

    nrn2core_tqueue();
}

void vec_play_activate() {
    for (int tid = 0; tid < nrn_nthread; ++tid) {
        NrnThread* nt = nrn_threads + tid;
        for (int i = 0; i < nt->n_vecplay; ++i) {
            PlayRecord* pr = (PlayRecord*) nt->_vecplay[i];
            assert(pr->type() == VecPlayContinuousType);
            VecPlayContinuous* vpc = (VecPlayContinuous*) pr;
            assert(vpc->e_);
            assert(vpc->discon_indices_ == NULL);  // not implemented
            vpc->e_->send(vpc->t_[vpc->ubound_index_], net_cvode_instance, nt);
        }
    }
}

// For direct transfer of event queue information
// Must be the same as corresponding struct NrnCoreTransferEvents in NEURON
struct NrnCoreTransferEvents {
    std::vector<int> type;        // DiscreteEvent type
    std::vector<double> td;       // delivery time
    std::vector<int> intdata;     // ints specific to the DiscreteEvent type
    std::vector<double> dbldata;  // doubles specific to the type.
};


extern "C" {
/** Pointer to function in NEURON that iterates over its tqeueue **/
NrnCoreTransferEvents* (*nrn2core_transfer_tqueue_)(int tid);
}

// for faster determination of the movable index given the type
static std::map<int, int> type2movable;
static void setup_type2semantics() {
    if (type2movable.empty()) {
        for (auto& mf: corenrn.get_memb_funcs()) {
            size_t n_memb_func = (int) (corenrn.get_memb_funcs().size());
            for (int type = 0; type < n_memb_func; ++type) {
                int* ds = corenrn.get_memb_func((size_t) type).dparam_semantics;
                if (ds) {
                    int dparam_size = corenrn.get_prop_dparam_size()[type];
                    for (int psz = 0; psz < dparam_size; ++psz) {
                        if (ds[psz] == -4) {  // netsend semantics
                            type2movable[type] = psz;
                        }
                    }
                }
            }
        }
    }
}

/** Copy each thread's queue from NEURON **/
static void nrn2core_tqueue() {
    if (type2movable.empty()) {
        setup_type2semantics();  // need type2movable for SelfEvent.
    }
    for (int tid = 0; tid < nrn_nthread; ++tid) {  // should be parallel
        NrnCoreTransferEvents* ncte = (*nrn2core_transfer_tqueue_)(tid);
        if (ncte) {
            size_t idat = 0;
            size_t idbldat = 0;
            NrnThread& nt = nrn_threads[tid];
            for (size_t i = 0; i < ncte->type.size(); ++i) {
                switch (ncte->type[i]) {
                    case 0: {  // DiscreteEvent
                               // Ignore
                    } break;

                    case 2: {  // NetCon
                        int ncindex = ncte->intdata[idat++];
                        NetCon* nc = nt.netcons + ncindex;
#define DEBUGQUEUE 0
#if DEBUGQUEUE
                        printf("nrn2core_tqueue tid=%d i=%zd type=%d tdeliver=%g NetCon %d\n",
                               tid,
                               i,
                               ncte->type[i],
                               ncte->td[i],
                               ncindex);
#endif
                        nc->send(ncte->td[i], net_cvode_instance, &nt);
                    } break;

                    case 3: {  // SelfEvent
                        // target_type, target_instance, weight_index, flag movable

                        // This is a nightmare and needs to be profoundly re-imagined.

                        // Determine Point_process*
                        int target_type = ncte->intdata[idat++];
                        int target_instance = ncte->intdata[idat++];
                        // From target_type and target_instance (mechanism data index)
                        // compute the nt.pntprocs index.
                        int offset = nt._pnt_offset[target_type];
                        Point_process* pnt = nt.pntprocs + offset + target_instance;
                        assert(pnt->_type == target_type);
                        assert(pnt->_i_instance == target_instance);
                        assert(pnt->_tid == tid);

                        // Determine weight_index
                        int netcon_index = ncte->intdata[idat++];  // via the NetCon
                        int weight_index = -1;                     // no associated netcon
                        if (netcon_index >= 0) {
                            weight_index = nt.netcons[netcon_index].u.weight_index_;
                        }

                        double flag = ncte->dbldata[idbldat++];
                        int is_movable = ncte->intdata[idat++];
                        // If the queue item is movable, then the pointer needs to be
                        // stored in the mechanism instance movable slot by net_send.
                        // And don't overwrite if not movable. Only one SelfEvent
                        // for a given target instance is movable.
                        Memb_list* ml = nt._ml_list[target_type];
                        int movable_index =
                            nrn_i_layout(target_instance,
                                         ml->nodecount,
                                         type2movable[target_type],
                                         corenrn.get_prop_dparam_size()[target_type],
                                         corenrn.get_mech_data_layout()[target_type]);
                        void** movable_arg = nt._vdata + ml->pdata[movable_index];
                        TQItem* old_movable_arg = (TQItem*) (*movable_arg);
#if DEBUGQUEUE
                        printf("nrn2core_tqueue tid=%d i=%zd type=%d tdeliver=%g SelfEvent\n",
                               tid,
                               i,
                               ncte->type[i],
                               ncte->td[i]);
                        printf(
                            "  target_type=%d pnt data index=%d flag=%g is_movable=%d netcon index "
                            "for weight=%d\n",
                            target_type,
                            target_instance,
                            flag,
                            is_movable,
                            netcon_index);
#endif
                        net_send(movable_arg, weight_index, pnt, ncte->td[i], flag);
                        if (!is_movable) {
                            *movable_arg = (void*) old_movable_arg;
                        }
                    } break;

                    case 4: {  // PreSyn
                        int ps_index = ncte->intdata[idat++];
#if DEBUGQUEUE
                        printf("nrn2core_tqueue tid=%d i=%zd type=%d tdeliver=%g PreSyn %d\n",
                               tid,
                               i,
                               ncte->type[i],
                               ncte->td[i],
                               ps_index);
#endif
                        PreSyn* ps = nt.presyns + ps_index;
                        int gid = ps->output_index_;
                        // Following assumes already sent to other machines.
                        ps->output_index_ = -1;
                        ps->send(ncte->td[i], net_cvode_instance, &nt);
                        ps->output_index_ = gid;
                    } break;

                    case 6: {  // PlayRecordEvent
                               // Ignore as phase2 handles analogous to checkpoint restore.
                    } break;

                    case 7: {  // NetParEvent
#if DEBUGQUEUE
                        printf("nrn2core_tqueue tid=%d i=%zd type=%d tdeliver=%g NetParEvent\n",
                               tid,
                               i,
                               ncte->type[i],
                               ncte->td[i]);
#endif
                    } break;

                    default: {
                        static char s[20];
                        sprintf(s, "%d", ncte->type[i]);
                        hoc_execerror("Unimplemented transfer queue event type:", s);
                    } break;
                }
            }
            delete ncte;
        }
    }
}

/** @brief return first and last datum indices of WATCH statements
 */
void watch_datum_indices(int type, int& first, int& last) {
    int* semantics = corenrn.get_memb_func(type).dparam_semantics;
    int dparam_size = corenrn.get_prop_dparam_size()[type];
    // which slots are WATCH
    // Note that first is the WatchList item, not the WatchCondition
    first = -1;
    last = 0;
    for (int i = 0; i < dparam_size; ++i) {
        if (semantics[i] == -8) {  // WATCH
            if (first == -1) {
                first = i;
            }
            last = i;
        }
    }
}

void watch_activate_clear() {
    // Can identify mechanisms with WATCH statements from non-NULL
    // corenrn.get_watch_check()[type] and figure out pdata that are
    // _watch_array items from corenrn.get_memb_func(type).dparam_semantics
    // Ironically, all WATCH statements may already be inactivated in
    // consequence of phase2 transfer. But, for direct mode psolve, we would
    // eventually like to minimise that transfer (at least with respect to
    // structure).

    // Loop over threads, mechanisms and pick out the ones with WATCH statements.
    for (int tid = 0; tid < nrn_nthread; ++tid) {
        NrnThread& nt = nrn_threads[tid];
        for (NrnThreadMembList* tml = nt.tml; tml; tml = tml->next) {
            if (corenrn.get_watch_check()[tml->index]) {
                // zero all the WATCH slots.
                Memb_list* ml = tml->ml;
                int type = tml->index;
                int* semantics = corenrn.get_memb_func(type).dparam_semantics;
                int dparam_size = corenrn.get_prop_dparam_size()[type];
                // which slots are WATCH
                int first, last;
                watch_datum_indices(type, first, last);
                // Zero the _watch_array from first to last inclusive.
                // Note: the first is actually unused but is there because NEURON
                // uses it. There is probably a better way to do this.
                int* pdata = ml->pdata;
                int nodecount = ml->nodecount;
                int layout = corenrn.get_mech_data_layout()[type];
                for (int iml = 0; iml < nodecount; ++iml) {
                    for (int i = first; i <= last; ++i) {
                        int* pd = pdata + nrn_i_layout(iml, nodecount, i, dparam_size, layout);
                        *pd = 0;
                    }
                }
            }
        }
    }
}

void nrn2core_transfer_watch_condition(int tid,
                                       int pnttype,
                                       int pntindex,
                                       int watch_index,
                                       int triggered) {
    // Note: watch_index relative to AoS _ppvar for instance.
    NrnThread& nt = nrn_threads[tid];
    int pntoffset = nt._pnt_offset[pnttype];
    Point_process* pnt = nt.pntprocs + (pntoffset + pntindex);
    assert(pnt->_type == pnttype);
    assert(pnt->_i_instance == pntindex);  // is this true for permutation?
    assert(pnt->_tid == tid);

    // perhaps all this should be more closely associated with phase2 since
    // we are really talking about (direct) transfer from NEURON and not able
    // to rely on finitialize() on the CoreNEURON side which would otherwise
    // set up all this stuff as a consequence of SelfEvents initiated
    // and delivered at time 0.
    // I've become shakey in regard to how this is done since the reorganization
    // from where everything was done in nrn_setup.cpp. Here, I'm guessing
    // nrn_i_layout is the relevant index transformation after finding the
    // beginning of the mechanism pdata.
    Memb_list* ml = nt._ml_list[pnttype];
    int* pdata = ml->pdata;
    int iml = pntindex;
    int nodecount = ml->nodecount;
    int i = watch_index;
    int dparam_size = corenrn.get_prop_dparam_size()[pnttype];
    int layout = corenrn.get_mech_data_layout()[pnttype];
    int* pd = pdata + nrn_i_layout(iml, nodecount, i, dparam_size, layout);

    // activate the WatchCondition
    *pd = 2 + triggered;
}

}  // namespace coreneuron
