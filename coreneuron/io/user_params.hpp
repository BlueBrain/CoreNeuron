#pragma once

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
}  // namespace coreneuron

