#pragma once

namespace coreneuron {
struct UserParams {
    /// direct memory mode with neuron, do not open files
    bool in_memory_transfer;
    /// Number of local cell groups
    int ngroup;
    /// Array of cell group numbers (indices)
    int* gidgroups;
    /// Array of duplicate indices. Normally, with nrn_setup_multiple=1,
    //   they are ngroup values of 0.
    int* imult;
    /// path to dataset file
    const char* path;
    ///dataset  path from where simulation is being restored
    const char* restore_path;
    FileHandler* file_reader;
    /// true if byte swapping should be done (little endian <-> big endian) 
    bool byte_swap;
};
}  // namespace coreneuron
