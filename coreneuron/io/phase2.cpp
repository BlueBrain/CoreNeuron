#include "coreneuron/io/phase2.hpp"
#include "coreneuron/coreneuron.hpp"
#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/io/nrn_checkpoint.hpp"
#include "coreneuron/utils/nrnoc_aux.hpp"
#include "coreneuron/network/partrans.hpp"
#include "coreneuron/permute/cellorder.hpp"
#include "coreneuron/permute/node_permute.h"
#include "coreneuron/utils/vrecitem.h"
#include "coreneuron/io/nrn_soa.hpp"

int (*nrn2core_get_dat2_1_)(int tid,
                            int& ngid,
                            int& n_real_gid,
                            int& nnode,
                            int& ndiam,
                            int& nmech,
                            int*& tml_index,
                            int*& ml_nodecount,
                            int& nidata,
                            int& nvdata,
                            int& nweight);

int (*nrn2core_get_dat2_2_)(int tid,
                            int*& v_parent_index,
                            double*& a,
                            double*& b,
                            double*& area,
                            double*& v,
                            double*& diamvec);

int (*nrn2core_get_dat2_mech_)(int tid,
                               size_t i,
                               int dsz_inst,
                               int*& nodeindices,
                               double*& data,
                               int*& pdata);

int (*nrn2core_get_dat2_3_)(int tid,
                            int nweight,
                            int*& output_vindex,
                            double*& output_threshold,
                            int*& netcon_pnttype,
                            int*& netcon_pntindex,
                            double*& weights,
                            double*& delays);

int (*nrn2core_get_dat2_corepointer_)(int tid, int& n);

int (*nrn2core_get_dat2_corepointer_mech_)(int tid,
                                           int type,
                                           int& icnt,
                                           int& dcnt,
                                           int*& iarray,
                                           double*& darray);

int (*nrn2core_get_dat2_vecplay_)(int tid, int& n);

int (*nrn2core_get_dat2_vecplay_inst_)(int tid,
                                       int i,
                                       int& vptype,
                                       int& mtype,
                                       int& ix,
                                       int& sz,
                                       double*& yvec,
                                       double*& tvec);

namespace coreneuron {
template <typename T>
inline void mech_layout(T* data, int cnt, int sz, int layout) {
    if (layout == 1) { /* AoS */
        return;
    }
    // layout is equal to 0 because we are SoA, let's transpose
    int align_cnt = nrn_soa_padded_size(cnt, layout);
    T* d = new T[cnt * sz];
    // copy matrix
    for (int i = 0; i < cnt; ++i) {
        for (int j = 0; j < sz; ++j) {
            d[i * sz + j] = data[i * sz + j];
        }
    }
    // transpose matrix
    for (int i = 0; i < cnt; ++i) {
        for (int j = 0; j < sz; ++j) {
            data[i + j * align_cnt] = d[i * sz + j];
        }
    }
    delete[] d;
}

void Phase2::read_file(FileHandler& F, const NrnThread& nt) {
    assert(!setted);
    this->setted = true;
    this->direct = false;
    n_output = F.read_int();
    nrn_assert(n_output > 0);  // avoid n_output unused warning
    n_real_output = F.read_int();
    n_node = F.read_int();
    n_diam = F.read_int();
    n_mech = F.read_int();
    types = std::vector<int>(n_mech, 0);
    nodecounts = std::vector<int>(n_mech, 0);
    for (int i = 0; i < n_mech; ++i) {
        types[i] = F.read_int();
        nodecounts[i] = F.read_int();
    }
    n_idata = F.read_int();
    n_vdata = F.read_int();
    int n_weight = F.read_int();
    v_parent_index = F.read_vector<int>(n_node);
    actual_a = F.read_vector<double>(n_node);
    actual_b = F.read_vector<double>(n_node);
    actual_area = F.read_vector<double>(n_node);
    actual_v = F.read_vector<double>(n_node);
    if (n_diam > 0)
        actual_diam = F.read_vector<double>(n_node);

    auto& param_sizes = corenrn.get_prop_param_size();
    auto& dparam_sizes = corenrn.get_prop_dparam_size();
    for (size_t i = 0; i < n_mech; ++i) {
        int type = types[i];
        std::vector<int> nodeindices;
        if (!corenrn.get_is_artificial()[type]) {
            nodeindices = F.read_vector<int>(nodecounts[i]);
        }
        auto data = F.read_vector<double>(param_sizes[type] * nodecounts[i]);
        std::vector<int> pdata;
        if (dparam_sizes[type] > 0) {
            pdata = F.read_vector<int>(dparam_sizes[type] * nodecounts[i]);
        }
        tmls.emplace_back(TML{nodeindices, data, pdata, 0, {}, {}});
    }
    output_vindex = F.read_vector<int>(nt.n_presyn);
    output_threshold = F.read_vector<double>(n_real_output);
    pnttype = F.read_vector<int>(nt.n_netcon - nrn_setup_extracon);
    pntindex = F.read_vector<int>(nt.n_netcon - nrn_setup_extracon);
    weights = F.read_vector<double>(n_weight);
    delay = F.read_vector<double>(nt.n_netcon);
    npnt = F.read_int();

    for (size_t i = 0; i < n_mech; ++i) {
        if (!corenrn.get_bbcore_read()[types[i]]) {
            continue;
        }
        tmls[i].type = F.read_int();
        int icnt = F.read_int();
        int dcnt = F.read_int();
        if (icnt > 0) {
            tmls[i].iArray = F.read_vector<int>(icnt);
        }
        if (dcnt > 0) {
            tmls[i].dArray = F.read_vector<double>(dcnt);
        }
    }

    int n_vecPlayContinous = F.read_int();
    vecPlayContinuous.reserve(n_vecPlayContinous);
    for (size_t i = 0; i < n_vecPlayContinous; ++i) {
        VecPlayContinuous2 item;
        item.vtype = F.read_int();
        item.mtype = F.read_int();
        item.ix = F.read_int();
        int sz = F.read_int();
        item.yvec = F.read_vector<double>(sz);
        item.tvec = F.read_vector<double>(sz);
        vecPlayContinuous.push_back(item);
    }

    // store current checkpoint state to continue reading mapping
    // The checkpoint numbering in phase 3 is a continuing of phase 2, and so will be restored
    F.record_checkpoint();

    if (F.eof())
        return;

    assert(F.read_int() == n_vecPlayContinous);

    for (int i = 0; i < n_vecPlayContinous; ++i) {
        auto &vecPlay = vecPlayContinuous[i];
        vecPlay.last_index = F.read_int();
        vecPlay.discon_index = F.read_int();
        vecPlay.ubound_index = F.read_int();
    }

    pastim_index = F.read_int();

    assert(F.read_int() == -1);

    for (int i = 0; i < nt.n_presyn; ++i) {
        preSynConditionEventFlags.push_back(F.read_int());
    }

    assert(F.read_int() == -1);
    save_events(F);

    assert(F.read_int() == -1);
    save_events(F);
}

void Phase2::read_direct(int thread_id, const NrnThread& nt) {
    assert(!this->setted);
    this->setted = true;
    this->direct = true;
    int* types_ = nullptr;
    int* nodecounts_ = nullptr;
    int n_weight;
    (*nrn2core_get_dat2_1_)(thread_id, n_output, n_real_output, n_node, n_diam, n_mech, types_,
            nodecounts_, n_idata, n_vdata, n_weight);
    types = std::vector<int>(types_, types_ + n_mech);
    delete[] types_;

    nodecounts = std::vector<int>(nodecounts_, nodecounts_ + n_mech);
    delete[] nodecounts_;

    v_parent_index.resize(n_node);
    actual_a.resize(n_node);
    actual_b.resize(n_node);
    actual_area.resize(n_node);
    actual_v.resize(n_node);
    if (n_diam > 0) {
        actual_diam.resize(n_node);
    }
    int* v_parent_index_ = const_cast<int*>(v_parent_index.data());
    double* actual_a_ = const_cast<double*>(actual_a.data());
    double* actual_b_ = const_cast<double*>(actual_b.data());
    double* actual_area_ = const_cast<double*>(actual_area.data());
    double* actual_v_ = const_cast<double*>(actual_v.data());
    double* actual_diam_ = const_cast<double*>(actual_diam.data());
    (*nrn2core_get_dat2_2_)(thread_id, v_parent_index_, actual_a_, actual_b_,
            actual_area_, actual_v_, actual_diam_);

    tmls.resize(n_mech);

    auto& param_sizes = corenrn.get_prop_param_size();
    auto& dparam_sizes = corenrn.get_prop_dparam_size();
    int dsz_inst = 0;
    for (size_t i = 0; i < n_mech; ++i) {
        auto& tml = tmls[i];
        int type = types[i];

        tml.nodeindices.resize(nodecounts[i]);
        tml.data.resize(nodecounts[i] * param_sizes[type]);
        tml.pdata.resize(nodecounts[i] * dparam_sizes[type]);

        int* nodeindices_ = nullptr;
        double* data_ = const_cast<double*>(tml.data.data());
        int* pdata_ = const_cast<int*>(tml.pdata.data());
        (*nrn2core_get_dat2_mech_)(thread_id, i, dparam_sizes[type] > 0 ? dsz_inst : 0, nodeindices_, data_, pdata_);
        if (dparam_sizes[type] > 0)
            dsz_inst++;
        std::copy(nodeindices_, nodeindices_ + nodecounts[i], tml.nodeindices.data());
        free_memory(nodeindices_);
    }

    int* output_vindex_ = nullptr;
    double* output_threshold_ = nullptr;
    int* pnttype_ = nullptr;
    int* pntindex_ = nullptr;
    double* weight_ = nullptr;
    double* delay_ = nullptr;
    (*nrn2core_get_dat2_3_)(thread_id, n_weight, output_vindex_, output_threshold_, pnttype_,
            pntindex_, weight_, delay_);

    output_vindex = std::vector<int>(output_vindex_, output_vindex_ + nt.n_presyn);
    delete[] output_vindex_;

    output_threshold = std::vector<double>(output_threshold_, output_threshold_ + n_real_output);
    delete[] output_threshold_;

    int n_netcon = nt.n_netcon - nrn_setup_extracon;
    pnttype = std::vector<int>(pnttype_, pnttype_ + n_netcon);
    delete[] pnttype_;

    pntindex = std::vector<int>(pntindex_, pntindex_ + n_netcon);
    delete[] pntindex_;

    weights = std::vector<double>(weight_, weight_ + n_weight);
    delete[] weight_;

    delay = std::vector<double>(delay_, delay_ + n_netcon);
    delete[] delay_;

    (*nrn2core_get_dat2_corepointer_)(nt.id, npnt);

    for (size_t i = 0; i < n_mech; ++i) {
        if (!corenrn.get_bbcore_read()[types[i]]) {
            continue; // I don't get this test
        }
        int icnt;
        int* iArray_ = nullptr;
        int dcnt;
        double* dArray_ = nullptr;
        (*nrn2core_get_dat2_corepointer_mech_)(nt.id, tmls[i].type, icnt, dcnt, iArray_, dArray_);
        tmls[i].iArray.resize(icnt);
        std::copy(iArray_, iArray_ + icnt, tmls[i].iArray.begin());
        delete[] iArray_;

        tmls[i].dArray.resize(dcnt);
        std::copy(dArray_, dArray_ + dcnt, tmls[i].dArray.begin());
        delete[] dArray_;
    }

    int n_vecPlayContinous;
    (*nrn2core_get_dat2_vecplay_)(thread_id, n_vecPlayContinous);

    for (size_t i = 0; i < n_vecPlayContinous; ++i) {
        VecPlayContinuous2 item;
        // yvec_ and tvec_ are not deleted as that space is within
        // NEURON Vector
        double *yvec_, *tvec_;
        int sz;
        (*nrn2core_get_dat2_vecplay_inst_)(thread_id, i, item.vtype, item.mtype, item.ix, sz, yvec_, tvec_);
        item.yvec = std::vector<double>(yvec_, yvec_ + sz);
        item.tvec = std::vector<double>(tvec_, tvec_ + sz);
        vecPlayContinuous.push_back(item);
    }
}

void Phase2::check_mechanism() {
    int diff_mech_count = 0;
    for (int i = 0; i < n_mech; ++i) {
        if (std::any_of(corenrn.get_different_mechanism_type().begin(),
                    corenrn.get_different_mechanism_type().end(),
                    [&](int e) { return e == types[i]; })) {
            if (nrnmpi_myid == 0) {
                printf("Error: %s is a different MOD file than used by NEURON!\n",
                        nrn_get_mechname(types[i]));
            }
            diff_mech_count++;
        }
    }

    if (diff_mech_count > 0) {
        if (nrnmpi_myid == 0) {
            printf(
                    "Error : NEURON and CoreNEURON must use same mod files for compatibility, %d different mod file(s) found. Re-compile special and special-core!\n",
                    diff_mech_count);
        }
        nrn_abort(1);
    }

}

void Phase2::pdata_relocation(int elem0, int nodecount, int* pdata, int i, int dparam_size, int layout, int n_node_) {
    for (int iml = 0; iml < nodecount; ++iml) {
        int* pd = pdata + nrn_i_layout(iml, nodecount, i, dparam_size, layout);
        int ix = *pd;  // relative to beginning of _actual_*
        nrn_assert((ix >= 0) && (ix < n_node_));
        *pd = elem0 + ix;  // relative to nt._data
    }
}

NrnThreadMembList* Phase2::create_tml(int mech_id, Memb_func& memb_func, int& shadow_rhs_cnt) {
    auto tml = (NrnThreadMembList*)emalloc_align(sizeof(NrnThreadMembList));
    tml->next = nullptr;
    tml->index = types[mech_id];

    tml->ml = (Memb_list*)ecalloc_align(1, sizeof(Memb_list));
    tml->ml->_net_receive_buffer = nullptr;
    tml->ml->_net_send_buffer = nullptr;
    tml->ml->_permute = nullptr;
    if (memb_func.alloc == nullptr) {
        hoc_execerror(memb_func.sym, "mechanism does not exist");
    }
    tml->ml->nodecount = nodecounts[mech_id];
    if (!memb_func.sym) {
        printf("%s (type %d) is not available\n", nrn_get_mechname(tml->index), tml->index);
        exit(1);
    }
    tml->ml->_nodecount_padded =
        nrn_soa_padded_size(tml->ml->nodecount, corenrn.get_mech_data_layout()[tml->index]);
    if (memb_func.is_point && corenrn.get_is_artificial()[tml->index] == 0) {
        // Avoid race for multiple PointProcess instances in same compartment.
        if (tml->ml->nodecount > shadow_rhs_cnt) {
            shadow_rhs_cnt = tml->ml->nodecount;
        }
    }

    // printf("index=%d nodecount=%d membfunc=%s\n", tml->index, tml->ml->nodecount,
    // memb_func.sym?memb_func.sym:"None");

    return tml;
}

void Phase2::set_net_send_buffer(Memb_list** ml_list, const std::vector<int>& pnt_offset) {
    // NetReceiveBuffering
    for (auto& net_buf_receive : corenrn.get_net_buf_receive()) {
        int type = net_buf_receive.second;
        // Does this thread have this type.
        Memb_list* ml = ml_list[type];
        if (ml) {  // needs a NetReceiveBuffer
            NetReceiveBuffer_t* nrb =
                (NetReceiveBuffer_t*)ecalloc_align(1, sizeof(NetReceiveBuffer_t));
            ml->_net_receive_buffer = nrb;
            nrb->_pnt_offset = pnt_offset[type];

            // begin with a size of 5% of the number of instances
            nrb->_size = ml->nodecount;
            // or at least 8
            nrb->_size = std::max(8, nrb->_size);
            // but not more than nodecount
            nrb->_size = std::min(ml->nodecount, nrb->_size);

            nrb->_pnt_index = (int*)ecalloc_align(nrb->_size, sizeof(int));
            nrb->_displ = (int*)ecalloc_align(nrb->_size + 1, sizeof(int));
            nrb->_nrb_index = (int*)ecalloc_align(nrb->_size, sizeof(int));
            nrb->_weight_index = (int*)ecalloc_align(nrb->_size, sizeof(int));
            nrb->_nrb_t = (double*)ecalloc_align(nrb->_size, sizeof(double));
            nrb->_nrb_flag = (double*)ecalloc_align(nrb->_size, sizeof(double));
        }
    }

    // NetSendBuffering
    for (int type: corenrn.get_net_buf_send_type()) {
        // Does this thread have this type.
        Memb_list* ml = ml_list[type];
        if (ml) {  // needs a NetSendBuffer
            NetSendBuffer_t* nsb = (NetSendBuffer_t*)ecalloc_align(1, sizeof(NetSendBuffer_t));
            ml->_net_send_buffer = nsb;

            // begin with a size equal to twice number of instances
            // at present there is no provision for dynamically increasing this.
            nsb->_size = ml->nodecount * 2;
            nsb->_cnt = 0;

            nsb->_sendtype = (int*)ecalloc_align(nsb->_size, sizeof(int));
            nsb->_vdata_index = (int*)ecalloc_align(nsb->_size, sizeof(int));
            nsb->_pnt_index = (int*)ecalloc_align(nsb->_size, sizeof(int));
            nsb->_weight_index = (int*)ecalloc_align(nsb->_size, sizeof(int));
            // when == 1, NetReceiveBuffer_t is newly allocated (i.e. we need to free previous copy
            // and recopy new data
            nsb->reallocated = 1;
            nsb->_nsb_t = (double*)ecalloc_align(nsb->_size, sizeof(double));
            nsb->_nsb_flag = (double*)ecalloc_align(nsb->_size, sizeof(double));
        }
    }
}
void Phase2::save_events(FileHandler& F) {
    int type;
    while (type = F.read_int() != 0) {
        double te;
        F.read_array(&te, 1);
        switch (type) {
            case NetConType: {
                auto event = std::make_shared<NetConType_>();
                event->te = te;
                event->ncindex = F.read_int();
                events.emplace_back(type, event);
                break;
            }
            case SelfEventType: {
                auto event = std::make_shared<SelfEventType_>();
                event->te = te;
                event->target_type = F.read_int();
                event->pinstance = F.read_int();
                event->target_instance = F.read_int();
                F.read_array(&event->flag, 1);
                event->movable = F.read_int();
                event->weight_index = F.read_int();
                events.emplace_back(type, event);
                break;
            }
            case PreSynType: {
                auto event = std::make_shared<PreSynType_>();
                event->te = te;
                event->psindex = F.read_int();
                events.emplace_back(type, event);
                break;
            }
            case NetParEventType: {
                auto event = std::make_shared<NetParEvent_>();
                event->te = te;
                events.emplace_back(type, event);
                break;
            }
            case PlayRecordEventType: {
                auto event = std::make_shared<PlayRecordEventType_>();
                event->te = te;
                event->prtype = F.read_int();
                event->vecplay_index = F.read_int();
                events.emplace_back(type, event);
                break;
            }
            default: {
                assert(0);
                break;
            }
        }
    }
}

void Phase2::populate(NrnThread& nt, int imult, const UserParams& userParams) {
    assert(this->setted);

    nrn_assert(imult >= 0);  // avoid imult unused warning
    NrnThreadChkpnt& ntc = nrnthread_chkpnt[nt.id];
    ntc.file_id = userParams.gidgroups[nt.id];

    nt.ncell = n_real_output;
    nt.end = n_node;

    check_mechanism();

#if CHKPNTDEBUG
    ntc.n_outputgids = n_output;
    ntc.nmech = n_mech;
#endif

    /// Checkpoint in coreneuron is defined for both phase 1 and phase 2 since they are written
    /// together
    // printf("ncell=%d end=%d nmech=%d\n", nt.ncell, nt.end, n_mech);
    // printf("nart=%d\n", nart);
    nt._ml_list = (Memb_list**)ecalloc_align(corenrn.get_memb_funcs().size(), sizeof(Memb_list*));

    auto& memb_func = corenrn.get_memb_funcs();
#if CHKPNTDEBUG
    ntc.mlmap = new Memb_list_chkpnt*[memb_func.size()];
    for (int i = 0; i < _memb_func.size(); ++i) {
        ntc.mlmap[i] = nullptr;
    }
#endif

    nt.stream_id = 0;
    nt.compute_gpu = 0;
    auto& nrn_prop_param_size_ = corenrn.get_prop_param_size();
    auto& nrn_prop_dparam_size_ = corenrn.get_prop_dparam_size();

/* read_phase2 is being called from openmp region
 * and hence we can set the stream equal to current thread id.
 * In fact we could set gid as stream_id when we will have nrn threads
 * greater than number of omp threads.
 */
#if defined(_OPENMP)
    nt.stream_id = omp_get_thread_num();
#endif

    int shadow_rhs_cnt = 0;
    nt.shadow_rhs_cnt = 0;

    NrnThreadMembList* tml_last = nullptr;
    for (int i = 0; i < n_mech; ++i) {
        auto tml = create_tml(i, memb_func[types[i]], shadow_rhs_cnt);

        nt._ml_list[tml->index] = tml->ml;

#if CHKPNTDEBUG
        Memb_list_chkpnt* mlc = new Memb_list_chkpnt;
        ntc.mlmap[tml->index] = mlc;
#endif

        if (nt.tml) {
            tml_last->next = tml;
        } else {
            nt.tml = tml;
        }
        tml_last = tml;
    }

    if (shadow_rhs_cnt) {
        nt._shadow_rhs =
            (double*)ecalloc_align(nrn_soa_padded_size(shadow_rhs_cnt, 0), sizeof(double));
        nt._shadow_d =
            (double*)ecalloc_align(nrn_soa_padded_size(shadow_rhs_cnt, 0), sizeof(double));
        nt.shadow_rhs_cnt = shadow_rhs_cnt;
    }

    nt._data = nullptr;    // allocated below after padding
    nt.mapping = nullptr;  // section segment mapping

    nt._nidata = n_idata;
    if (nt._nidata)
        nt._idata = (int*)ecalloc(nt._nidata, sizeof(int));
    else
        nt._idata = nullptr;
    // see patternstim.cpp
    int extra_nv = (&nt == nrn_threads) ? nrn_extra_thread0_vdata : 0;
    nt._nvdata = n_vdata;
    if (nt._nvdata + extra_nv)
        nt._vdata = (void**)ecalloc_align(nt._nvdata + extra_nv, sizeof(void*));
    else
        nt._vdata = nullptr;
    // printf("_nidata=%d _nvdata=%d\n", nt._nidata, nt._nvdata);

    // The data format begins with the matrix data
    int ne = nrn_soa_padded_size(nt.end, MATRIX_LAYOUT);
    size_t offset = 6 * ne;

    if (n_diam) {
        // in the rare case that a mechanism has dparam with diam semantics
        // then actual_diam array added after matrix in nt._data
        // Generally wasteful since only a few diam are pointed to.
        // Probably better to move the diam semantics to the p array of the mechanism
        offset += ne;
    }

    // Memb_list.data points into the nt.data array.
    // Also count the number of Point_process
    int npnt = 0;
    for (auto tml = nt.tml; tml; tml = tml->next) {
        Memb_list* ml = tml->ml;
        int type = tml->index;
        int layout = corenrn.get_mech_data_layout()[type];
        int n = ml->nodecount;
        int sz = nrn_prop_param_size_[type];
        offset = nrn_soa_byte_align(offset);
        ml->data = (double*)0 + offset;  // adjust below since nt._data not allocated
        offset += nrn_soa_padded_size(n, layout) * sz;
        if (corenrn.get_pnt_map()[type] > 0) {
            npnt += n;
        }
    }
    nt.pntprocs = (Point_process*)ecalloc_align(
        npnt, sizeof(Point_process));  // includes acell with and without gid
    nt.n_pntproc = npnt;
    // printf("offset=%ld\n", offset);
    nt._ndata = offset;

    // now that we know the effect of padding, we can allocate data space,
    // fill matrix, and adjust Memb_list data pointers
    nt._data = (double*)ecalloc_align(nt._ndata, sizeof(double));
    nt._actual_rhs = nt._data + 0 * ne;
    nt._actual_d = nt._data + 1 * ne;
    nt._actual_a = nt._data + 2 * ne;
    nt._actual_b = nt._data + 3 * ne;
    nt._actual_v = nt._data + 4 * ne;
    nt._actual_area = nt._data + 5 * ne;
    nt._actual_diam = n_diam ? nt._data + 6 * ne : nullptr;
    for (auto tml = nt.tml; tml; tml = tml->next) {
        Memb_list* ml = tml->ml;
        ml->data = nt._data + (ml->data - (double*)0);
    }

    // matrix info
    nt._v_parent_index = (int*)ecalloc_align(nt.end, sizeof(int));
    std::copy(v_parent_index.begin(), v_parent_index.end(), nt._v_parent_index);
    std::copy(actual_a.begin(), actual_a.end(), nt._actual_a);
    std::copy(actual_b.begin(), actual_b.end(), nt._actual_b);
    std::copy(actual_area.begin(), actual_area.end(), nt._actual_area);
    std::copy(actual_v.begin(), actual_v.end(), nt._actual_v);
    if (n_diam) {
        std::copy(actual_diam.begin(), actual_diam.end(), nt._actual_diam);
    }

#if CHKPNTDEBUG
    ntc.parent = new int[nt.end];
    memcpy(ntc.parent, nt._v_parent_index, nt.end * sizeof(int));
    ntc.area = new double[nt.end];
    memcpy(ntc.area, nt._actual_area, nt.end * sizeof(double));
#endif

    int synoffset = 0;
    std::vector<int> pnt_offset(memb_func.size());

    // All the mechanism data and pdata.
    // Also fill in the pnt_offset
    // Complete spec of Point_process except for the acell presyn_ field.
    int itml = 0;
    for (auto tml = nt.tml; tml; tml = tml->next, ++itml) {
        int type = tml->index;
        Memb_list* ml = tml->ml;
        int n = ml->nodecount;
        int szp = nrn_prop_param_size_[type];
        int szdp = nrn_prop_dparam_size_[type];
        int layout = corenrn.get_mech_data_layout()[type];

        if (!corenrn.get_is_artificial()[type]) {
            ml->nodeindices = (int*)ecalloc_align(ml->nodecount, sizeof(int));
            std::copy(tmls[itml].nodeindices.begin(), tmls[itml].nodeindices.end(), ml->nodeindices);
        }

        std::copy(tmls[itml].data.begin(), tmls[itml].data.end(), ml->data);
        mech_layout<double>(ml->data, n, szp, layout);

        if (szdp) {
            ml->pdata = (int*)ecalloc_align(nrn_soa_padded_size(n, layout) * szdp, sizeof(int));
            std::copy(tmls[itml].pdata.begin(), tmls[itml].pdata.end(), ml->pdata);
            mech_layout<int>(ml->pdata, n, szdp, layout);

#if CHKPNTDEBUG  // Not substantive. Only for debugging.
            Memb_list_ckpnt* mlc = ntc.mlmap[type];
            mlc->pdata_not_permuted = (int*)coreneuron::ecalloc_align(n * szdp, sizeof(int));
            if (layout == 1) {  // AoS just copy
                for (int i = 0; i < n; ++i) {
                    for (int j = 0; j < szdp; ++j) {
                        mlc->pdata_not_permuted[i * szdp + j] = ml->pdata[i * szdp + j];
                    }
                }
            } else if (layout == 0) {  // SoA transpose and unpad
                int align_cnt = nrn_soa_padded_size(n, layout);
                for (int i = 0; i < n; ++i) {
                    for (int j = 0; j < szdp; ++j) {
                        mlc->pdata_not_permuted[i * szdp + j] = ml->pdata[i + j * align_cnt];
                    }
                }
            }
#endif
        } else {
            ml->pdata = nullptr;
        }
        if (corenrn.get_pnt_map()[type] > 0) {  // POINT_PROCESS mechanism including acell
            int cnt = ml->nodecount;
            Point_process* pnt = nullptr;
            pnt = nt.pntprocs + synoffset;
            pnt_offset[type] = synoffset;
            synoffset += cnt;
            for (int i = 0; i < cnt; ++i) {
                Point_process* pp = pnt + i;
                pp->_type = type;
                pp->_i_instance = i;
                nt._vdata[ml->pdata[nrn_i_layout(i, cnt, 1, szdp, layout)]] = pp;
                pp->_tid = nt.id;
            }
        }
    }

    if (nrn_have_gaps) {
        nrn_partrans::gap_thread_setup(nt);
    }

    // Some pdata may index into data which has been reordered from AoS to
    // SoA. The four possibilities are if semantics is -1 (area), -5 (pointer),
    // -9 (diam), // or 0-999 (ion variables).
    // Note that pdata has a layout and the // type block in nt.data into which
    // it indexes, has a layout.
    for (auto tml = nt.tml; tml; tml = tml->next) {
        int type = tml->index;
        int layout = corenrn.get_mech_data_layout()[type];
        int* pdata = tml->ml->pdata;
        int cnt = tml->ml->nodecount;
        int szdp = nrn_prop_dparam_size_[type];
        int* semantics = memb_func[type].dparam_semantics;

        // ignore ARTIFICIAL_CELL (has useless area pointer with semantics=-1)
        if (corenrn.get_is_artificial()[type]) {
            continue;
        }

        if (szdp) {
            if (!semantics)
                continue;  // temporary for HDFReport, Binreport which will be skipped in
                           // bbcore_write of HBPNeuron
            nrn_assert(semantics);
        }

        for (int i = 0; i < szdp; ++i) {
            int s = semantics[i];
            if (s == -1) {  // area
                pdata_relocation(nt._actual_area - nt._data, cnt, pdata, i, szdp, layout, nt.end);
            } else if (s == -9) {  // diam
                pdata_relocation(nt._actual_diam - nt._data, cnt, pdata, i, szdp, layout, nt.end);
            } else if (s == -5) {  // pointer assumes a pointer to membrane voltage
                pdata_relocation(nt._actual_v - nt._data, cnt, pdata, i, szdp, layout, nt.end);
            } else if (s >= 0 && s < 1000) {  // ion
                int etype = s;
                /* if ion is SoA, must recalculate pdata values */
                /* if ion is AoS, have to deal with offset */
                Memb_list* eml = nt._ml_list[etype];
                int edata0 = eml->data - nt._data;
                int ecnt = eml->nodecount;
                int esz = nrn_prop_param_size_[etype];
                for (int iml = 0; iml < cnt; ++iml) {
                    int* pd = pdata + nrn_i_layout(iml, cnt, i, szdp, layout);
                    int ix = *pd;  // relative to the ion data
                    nrn_assert((ix >= 0) && (ix < ecnt * esz));
                    /* Original pd order assumed ecnt groups of esz */
                    *pd = edata0 + nrn_param_layout(ix, etype, eml);
                }
            }
        }
    }

    /* if desired, apply the node permutation. This involves permuting
       at least the node parameter arrays for a, b, and area (and diam) and all
       integer vector values that index into nodes. This could have been done
       when originally filling the arrays with AoS ordered data, but can also
       be done now, after the SoA transformation. The latter has the advantage
       that the present order is consistent with all the layout values. Note
       that after this portion of the permutation, a number of other node index
       vectors will be read and will need to be permuted as well in subsequent
       sections of this function.
    */
    if (interleave_permute_type) {
        nt._permute = interleave_order(nt.id, nt.ncell, nt.end, nt._v_parent_index);
    }
    if (nt._permute) {
        int* p = nt._permute;
        permute_data(nt._actual_a, nt.end, p);
        permute_data(nt._actual_b, nt.end, p);
        permute_data(nt._actual_area, nt.end, p);
        permute_data(nt._actual_v, nt.end,
                     p);  // need if restore or finitialize does not initialize voltage
        if (nt._actual_diam) {
            permute_data(nt._actual_diam, nt.end, p);
        }
        // index values change as well as ordering
        permute_ptr(nt._v_parent_index, nt.end, p);
        node_permute(nt._v_parent_index, nt.end, p);

#if DEBUG
        for (int i=0; i < nt.end; ++i) {
            printf("parent[%d] = %d\n", i, nt._v_parent_index[i]);
        }
#endif

        // specify the ml->_permute and sort the nodeindices
        for (auto tml = nt.tml; tml; tml = tml->next) {
            if (tml->ml->nodeindices) {  // not artificial
                permute_nodeindices(tml->ml, p);
            }
        }

        // permute mechanism data, pdata (and values)
        for (auto tml = nt.tml; tml; tml = tml->next) {
            if (tml->ml->nodeindices) {  // not artificial
                permute_ml(tml->ml, tml->index, nt);
            }
        }

        // permute the Point_process._i_instance
        for (int i = 0; i < nt.n_pntproc; ++i) {
            Point_process& pp = nt.pntprocs[i];
            Memb_list* ml = nt._ml_list[pp._type];
            if (ml->_permute) {
                pp._i_instance = ml->_permute[pp._i_instance];
            }
        }
    }

    if (nrn_have_gaps && interleave_permute_type) {
        nrn_partrans::gap_indices_permute(nt);
    }

    /* here we setup the mechanism dependencies. if there is a mechanism dependency
     * then we allocate an array for tml->dependencies otherwise set it to nullptr.
     * In order to find out the "real" dependencies i.e. dependent mechanism
     * exist at the same compartment, we compare the nodeindices of mechanisms
     * returned by nrn_mech_depend.
     */

    /* temporary array for dependencies */
    int* mech_deps = (int*)ecalloc(memb_func.size(), sizeof(int));

    for (auto tml = nt.tml; tml; tml = tml->next) {
        /* initialize to null */
        tml->dependencies = nullptr;
        tml->ndependencies = 0;

        /* get dependencies from the models */
        int deps_cnt = nrn_mech_depend(tml->index, mech_deps);

        /* if dependencies, setup dependency array */
        if (deps_cnt) {
            /* store "real" dependencies in the vector */
            std::vector<int> actual_mech_deps;

            Memb_list* ml = tml->ml;
            int* nodeindices = ml->nodeindices;

            /* iterate over dependencies */
            for (int j = 0; j < deps_cnt; j++) {
                /* memb_list of dependency mechanism */
                Memb_list* dml = nt._ml_list[mech_deps[j]];

                /* dependency mechanism may not exist in the model */
                if (!dml)
                    continue;

                /* take nodeindices for comparison */
                int* dnodeindices = dml->nodeindices;

                /* set_intersection function needs temp vector to push the common values */
                std::vector<int> node_intersection;

                /* make sure they have non-zero nodes and find their intersection */
                if ((ml->nodecount > 0) && (dml->nodecount > 0)) {
                    std::set_intersection(nodeindices, nodeindices + ml->nodecount, dnodeindices,
                                          dnodeindices + dml->nodecount,
                                          std::back_inserter(node_intersection));
                }

                /* if they intersect in the nodeindices, it's real dependency */
                if (!node_intersection.empty()) {
                    actual_mech_deps.push_back(mech_deps[j]);
                }
            }

            /* copy actual_mech_deps to dependencies */
            if (!actual_mech_deps.empty()) {
                tml->ndependencies = actual_mech_deps.size();
                tml->dependencies = (int*)ecalloc(actual_mech_deps.size(), sizeof(int));
                memcpy(tml->dependencies, &actual_mech_deps[0],
                       sizeof(int) * actual_mech_deps.size());
            }
        }
    }

    /* free temp dependency array */
    free(mech_deps);

    /// Fill the BA lists
    std::vector<BAMech*> bamap(memb_func.size());
    for (int i = 0; i < BEFORE_AFTER_SIZE; ++i) {
        for (size_t ii = 0; ii < memb_func.size(); ++ii) {
            bamap[ii] = nullptr;
        }
        for (auto bam = corenrn.get_bamech()[i]; bam; bam = bam->next) {
            bamap[bam->type] = bam;
        }
        /* unnecessary but keep in order anyway */
        NrnThreadBAList **ptbl = nt.tbl + i;
        for (auto tml = nt.tml; tml; tml = tml->next) {
            if (bamap[tml->index]) {
                auto tbl = (NrnThreadBAList*)emalloc(sizeof(NrnThreadBAList));
                tbl->next = nullptr;
                tbl->bam = bamap[tml->index];
                tbl->ml = tml->ml;
                *ptbl = tbl;
                ptbl = &(tbl->next);
            }
        }
    }

    // for fast watch statement checking
    // setup a list of types that have WATCH statement
    {
        int sz = 0;  // count the types with WATCH
        for (auto tml = nt.tml; tml; tml = tml->next) {
            if (corenrn.get_watch_check()[tml->index]) {
                ++sz;
            }
        }
        if (sz) {
            nt._watch_types = (int*)ecalloc(sz + 1, sizeof(int));  // nullptr terminated
            sz = 0;
            for (auto tml = nt.tml; tml; tml = tml->next) {
                if (corenrn.get_watch_check()[tml->index]) {
                    nt._watch_types[sz++] = tml->index;
                }
            }
        }
    }
    auto& pnttype2presyn = corenrn.get_pnttype2presyn();
    auto& nrn_has_net_event_ = corenrn.get_has_net_event();
    // from nrn_has_net_event create pnttype2presyn.
    if (pnttype2presyn.empty()) {
        pnttype2presyn.resize(memb_func.size(), -1);
    }
    for (size_t i = 0; i < nrn_has_net_event_.size(); ++i) {
        pnttype2presyn[nrn_has_net_event_[i]] = i;
    }
    // create the nt.pnt2presyn_ix array of arrays.
    nt.pnt2presyn_ix = (int**)ecalloc(nrn_has_net_event_.size(), sizeof(int*));
    for (size_t i = 0; i < nrn_has_net_event_.size(); ++i) {
        Memb_list* ml = nt._ml_list[nrn_has_net_event_[i]];
        if (ml && ml->nodecount > 0) {
            nt.pnt2presyn_ix[i] = (int*)ecalloc(ml->nodecount, sizeof(int));
        }
    }

    // Real cells are at the beginning of the nt.presyns followed by
    // acells (with and without gids mixed together)
    // Here we associate the real cells with voltage pointers and
    // acell PreSyn with the Point_process.
    // nt.presyns order same as output_vindex order
#if CHKPNTDEBUG
    ntc.output_vindex = new int[nt.n_presyn];
    memcpy(ntc.output_vindex, output_vindex, nt.n_presyn * sizeof(int));
#endif
    if (nt._permute) {
        // only indices >= 0 (i.e. _actual_v indices) will be changed.
        node_permute(output_vindex.data(), nt.n_presyn, nt._permute);
    }
#if CHKPNTDEBUG
    ntc.output_threshold = new double[nt.ncell];
    memcpy(ntc.output_threshold, output_threshold, nt.ncell * sizeof(double));
#endif
    for (int i = 0; i < nt.n_presyn; ++i) {  // real cells
        PreSyn* ps = nt.presyns + i;

        int ix = output_vindex[i];
        if (ix == -1 && i < nt.ncell) {  // real cell without a presyn
            continue;
        }
        if (ix < 0) {
            ix = -ix;
            int index = ix / 1000;
            int type = ix - index * 1000;
            Point_process* pnt = nt.pntprocs + (pnt_offset[type] + index);
            ps->pntsrc_ = pnt;
            // pnt->_presyn = ps;
            int ip2ps = pnttype2presyn[pnt->_type];
            if (ip2ps >= 0) {
                nt.pnt2presyn_ix[ip2ps][pnt->_i_instance] = i;
            }
            if (ps->gid_ < 0) {
                ps->gid_ = -1;
            }
        } else {
            assert(ps->gid_ > -1);
            ps->thvar_index_ = ix;  // index into _actual_v
            assert(ix < nt.end);
            ps->threshold_ = output_threshold[i];
        }
    }

    // initial net_send_buffer size about 1% of number of presyns
    // nt._net_send_buffer_size = nt.ncell/100 + 1;
    // but, to avoid reallocation complexity on GPU ...
    nt._net_send_buffer_size = nt.ncell;
    nt._net_send_buffer = (int*)ecalloc_align(nt._net_send_buffer_size, sizeof(int));

    // do extracon later as the target and weight info
    // is not directly in the file
    int nnetcon = nt.n_netcon - nrn_setup_extracon;

    // printf("nnetcon=%d nweight=%d\n", nnetcon, nweight);
    // it may happen that Point_process structures will be made unnecessary
    // by factoring into NetCon.

#if CHKPNTDEBUG
    ntc.pnttype = new int[nnetcon];
    ntc.pntindex = new int[nnetcon];
    memcpy(ntc.pnttype, pnttype, nnetcon * sizeof(int));
    memcpy(ntc.pntindex, pntindex, nnetcon * sizeof(int));
#endif
    for (int i = 0; i < nnetcon; ++i) {
        int type = pnttype[i];
        if (type > 0) {
            int index = pnt_offset[type] + pntindex[i];  /// Potentially uninitialized pnt_offset[],
                                                         /// check for previous assignments
            NetCon& nc = nt.netcons[i];
            nc.target_ = nt.pntprocs + index;
            nc.active_ = true;
        }
    }

    int extracon_target_nweight = 0;
    if (nrn_setup_extracon > 0) {
        // Fill in the extracon target_ and active_.
        // Simplistic.
        // Rotate through the pntindex and use only pnttype for ProbAMPANMDA_EMS
        // (which happens to have a weight vector length of 5.)
        // Edge case: if there is no such synapse, let the target_ be nullptr
        //   and the netcon be inactive.
        // Same pattern as algorithm for extracon netcon_srcgid above in phase1.
        int extracon_target_type = nrn_get_mechtype("ProbAMPANMDA_EMS");
        assert(extracon_target_type > 0);
        extracon_target_nweight = corenrn.get_pnt_receive_size()[extracon_target_type];
        int j = 0;
        for (int i = 0; i < nrn_setup_extracon; ++i) {
            bool active = false;
            for (int k = 0; k < nnetcon; ++k) {
                if (pnttype[j] == extracon_target_type) {
                    active = true;
                    break;
                }
                j = (j + 1) % nnetcon;
            }
            NetCon& nc = nt.netcons[i + nnetcon];
            nc.active_ = active ? 1 : 0;
            if (active) {
                nc.target_ = nt.pntprocs + (pnt_offset[extracon_target_type] + pntindex[j]);
            } else {
                nc.target_ = nullptr;
            }
        }
    }

    {
        nt.n_weight = weights.size();
        int nweight = nt.n_weight;
        // weights in netcons order in groups defined by Point_process target type.
        nt.n_weight += nrn_setup_extracon * extracon_target_nweight;
        nt.weights = (double*)ecalloc_align(nt.n_weight, sizeof(double));
        std::copy(weights.begin(), weights.end(), nt.weights);

        int iw = 0;
        for (int i = 0; i < nnetcon; ++i) {
            NetCon& nc = nt.netcons[i];
            nc.u.weight_index_ = iw;
            if (pnttype[i] != 0) {
                iw += corenrn.get_pnt_receive_size()[pnttype[i]];
            } else {
                iw += 1;
            }
        }
        assert(iw == nweight);

#if CHKPNTDEBUG
        ntc.delay = new double[nnetcon];
        memcpy(ntc.delay, delay, nnetcon * sizeof(double));
#endif
        for (int i = 0; i < nnetcon; ++i) {
            NetCon& nc = nt.netcons[i];
            nc.delay_ = delay[i];
        }

        if (nrn_setup_extracon > 0) {
            // simplistic. delay is 1 and weight is 0.001
            for (int i = 0; i < nrn_setup_extracon; ++i) {
                NetCon& nc = nt.netcons[nnetcon + i];
                nc.delay_ = 1.0;
                nc.u.weight_index_ = nweight + i * extracon_target_nweight;
                nt.weights[nc.u.weight_index_] = 2.0;  // this value 2.0 is extracted from .dat files
            }
        }
    }

    // BBCOREPOINTER information
#if CHKPNTDEBUG
    ntc.nbcp = npnt;
    ntc.bcpicnt = new int[npnt];
    ntc.bcpdcnt = new int[npnt];
    ntc.bcptype = new int[npnt];
#endif
    for (size_t i = 0; i < n_mech; ++i) {
        int type = types[i];
        if (!corenrn.get_bbcore_read()[type]) {
            continue;
        }
        type = tmls[i].type; // This is not an error, but it has to be fixed I think
        if (!corenrn.get_bbcore_write()[type] && nrn_checkpoint_arg_exists) {
            fprintf(
                stderr,
                "Checkpoint is requested involving BBCOREPOINTER but there is no bbcore_write function for %s\n",
                memb_func[type].sym);
            assert(corenrn.get_bbcore_write()[type]);
        }
#if CHKPNTDEBUG
        ntc.bcptype[i] = type;
        ntc.bcpicnt[i] = icnt;
        ntc.bcpdcnt[i] = dcnt;
#endif
        int ik = 0;
        int dk = 0;
        Memb_list* ml = nt._ml_list[type];
        int dsz = nrn_prop_param_size_[type];
        int pdsz = nrn_prop_dparam_size_[type];
        int cntml = ml->nodecount;
        int layout = corenrn.get_mech_data_layout()[type];
        for (int j = 0; j < cntml; ++j) {
            int jp = j;
            if (ml->_permute) {
                jp = ml->_permute[j];
            }
            double* d = ml->data;
            Datum* pd = ml->pdata;
            d += nrn_i_layout(jp, cntml, 0, dsz, layout);
            pd += nrn_i_layout(jp, cntml, 0, pdsz, layout);
            int aln_cntml = nrn_soa_padded_size(cntml, layout);
            (*corenrn.get_bbcore_read()[type])(tmls[i].dArray.data(), tmls[i].iArray.data(), &dk, &ik, 0, aln_cntml, d, pd, ml->_thread,
                                      &nt, 0.0);
        }
        assert(dk == tmls[i].dArray.size());
        assert(ik == tmls[i].iArray.size());
    }

    // VecPlayContinuous instances
    // No attempt at memory efficiency
    nt.n_vecplay = vecPlayContinuous.size();
    if (nt.n_vecplay) {
        nt._vecplay = new void*[nt.n_vecplay];
    } else {
        nt._vecplay = nullptr;
    }
#if CHKPNTDEBUG
    ntc.vecplay_ix = new int[nt.n_vecplay];
    ntc.vtype = new int[nt.n_vecplay];
    ntc.mtype = new int[nt.n_vecplay];
#endif
    for (int i = 0; i < nt.n_vecplay; ++i) {
        auto& vecPlay = vecPlayContinuous[i];
        nrn_assert(vecPlay.vtype == VecPlayContinuousType);
#if CHKPNTDEBUG
        ntc.vtype[i] = vecPlay.vtype;
#endif
#if CHKPNTDEBUG
        ntc.mtype[i] = vecPlay.mtype;
#endif
        Memb_list* ml = nt._ml_list[vecPlay.mtype];
#if CHKPNTDEBUG
        ntc.vecplay_ix[i] = vecPlay.ix;
#endif
        IvocVect* yvec = vector_new(vecPlay.yvec.size());
        IvocVect* tvec = vector_new(vecPlay.tvec.size());
        double* py = vector_vec(yvec);
        double* pt = vector_vec(tvec);
        for (int j = 0; j < vecPlay.yvec.size(); ++j) {
            py[j] = vecPlay.yvec[j];
            pt[j] = vecPlay.tvec[j];
        }

        vecPlay.ix = nrn_param_layout(vecPlay.ix, vecPlay.mtype, ml);
        if (ml->_permute) {
            vecPlay.ix = nrn_index_permute(vecPlay.ix, vecPlay.mtype, ml);
        }
        nt._vecplay[i] = new VecPlayContinuous(ml->data + vecPlay.ix, yvec, tvec, nullptr, nt.id);
    }
    if (!events.empty()) {
        checkpoint_restore_tqueue(nt, *this);
    }

    set_net_send_buffer(nt._ml_list, pnt_offset);
}
}  // namespace coreneuron

