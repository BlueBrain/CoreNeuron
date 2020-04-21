#pragma once

#include "coreneuron/io/nrn_filehandler.hpp"
#include "coreneuron/io/user_params.hpp"

#include <memory>

namespace coreneuron {
struct NrnThread;
struct NrnThreadMembList;
struct Memb_func;
struct Memb_list;

class Phase2 {
    public:
    void read_direct(int thread_id, const NrnThread& nt);
    void read_file(FileHandler& F, const NrnThread& nt);
    void populate(NrnThread& nt, int imult, const UserParams& userParams);

    std::vector<int> preSynConditionEventFlags;

    // All of this is public for nrn_checkpoint
    struct EventTypeBase {
        double te;
    };
    struct NetConType_: public EventTypeBase {
        int ncindex;
    };
    struct SelfEventType_: public EventTypeBase {
        int target_type;
        int pinstance;
        int target_instance;
        double flag;
        int movable;
        int weight_index;
    };
    struct PreSynType_: public EventTypeBase {
        int psindex;
    };
    struct NetParEvent_: public EventTypeBase {
    };
    struct PlayRecordEventType_: public EventTypeBase {
        int prtype;
        int vecplay_index;
    };

    struct VecPlayContinuous2 {
        int vtype;
        int mtype;
        int ix;
        std::vector<double> yvec;
        std::vector<double> tvec;

        int last_index;
        int discon_index;
        int ubound_index;
    };
    std::vector<VecPlayContinuous2> vecPlayContinuous;
    int pastim_index;

    std::vector<std::pair<int, std::shared_ptr<EventTypeBase>>> events;

    private:
    // Internal state
    bool setted = false;
    bool direct;

    void check_mechanism();
    NrnThreadMembList* create_tml(int mech_id, Memb_func& memb_func, int& shadow_rhs_cnt);
    void pdata_relocation(int elem0, int nodecount, int* pdata, int i, int dparam_size, int layout, int n_node_);
    void set_net_send_buffer(Memb_list** ml_list, const std::vector<int>& pnt_offset);
    void save_events(FileHandler& F);
    void fill_ba_lists(NrnThread& nt, const std::vector<Memb_func>& memb_func);

    int n_output;
    int n_real_output;
    int n_node;
    int n_diam;  // 0 if not needed, else n_node
    int n_mech;
    std::vector<int> types;
    std::vector<int> nodecounts;
    int n_idata;
    int n_vdata;
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
};
}  // namespace coreneuron
