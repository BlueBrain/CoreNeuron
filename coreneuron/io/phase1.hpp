#pragma once

#include <vector>

#include "coreneuron/utils/nrnmutdec.h"
#include "coreneuron/io/nrn_filehandler.hpp"

namespace coreneuron {

struct NrnThread;

struct Phase1 {
    public:
    void read_direct(int thread_id);
    void read_file(FileHandler& F);
    void populate(NrnThread& nt, int imult MUTCOMMA MUTTYPE MUTVAR);

    private:
    void shift_gids(int imult);
    void add_extracon(NrnThread& nt, int imult);

    std::vector<int> output_gids;
    std::vector<int> netcon_srcgids;
};

}  // namespace coreneuron
