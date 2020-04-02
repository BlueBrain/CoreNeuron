#pragma once

#include "coreneuron/io/nrn_filehandler.hpp"
#include "coreneuron/io/user_params.hpp"

namespace coreneuron {
struct NrnThread;
struct NrnThreadMembList;
struct Memb_func;
struct Memb_list;

struct Phase2 {
    public:
    void read_direct(int thread_id, const NrnThread& nt);
    void read_file(FileHandler& F, const NrnThread& nt);
    void populate(NrnThread& nt, int imult, const UserParams& userParams);

    private:
    // Internal state
    bool setted = false;
    bool direct;

    void check_mechanism();
    NrnThreadMembList* create_tml(int mech_id, Memb_func& memb_func, int& shadow_rhs_cnt);
    void pdata_relocation(int elem0, int nodecount, int* pdata, int i, int dparam_size, int layout, int n_node_);
    void set_net_send_buffer(Memb_list** ml_list, const std::vector<int>& pnt_offset);

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
    struct VecPlayContinuous2 {
        int vtype;
        int mtype;
        int ix;
        std::vector<double> yvec;
        std::vector<double> tvec;
    };
    std::vector<VecPlayContinuous2> vecPlayContinuous;
};
}  // namespace coreneuron
