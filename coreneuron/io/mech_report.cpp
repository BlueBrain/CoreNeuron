#include <iostream>
#include <vector>

#include "coreneuron/coreneuron.hpp"
#include "coreneuron/mpi/nrnmpi.h"
#include "coreneuron/mpi/nrnmpi_impl.h"

namespace coreneuron {

/** display global mechanism count */
void write_mech_report() {
#if NRNMPI
    /// mechanim count across all gids, local to rank
    auto n_memb_func = corenrn.get_memb_funcs().size();
    std::vector<unsigned long long> local_mech_count(n_memb_func, 0);

    /// each gid record goes on separate row, only check non-empty threads
    for (size_t i = 0; i < nrn_nthread; i++) {
        const auto& nt = nrn_threads[i];
        std::vector<int> mech_count(n_memb_func, 0);
        for (auto* tml = nt.tml; tml; tml = tml->next) {
            int type = tml->index;
            const auto& ml = tml->ml;
            local_mech_count[type] += ml->nodecount;
        }
    }

    /// get global sum of all mechanism instances
    std::vector<unsigned long long> total_mech_count(n_memb_func);
    MPI_Allreduce(&local_mech_count[0],
                  &total_mech_count[0],
                  local_mech_count.size(),
                  MPI_UNSIGNED_LONG_LONG,
                  MPI_SUM,
                  MPI_COMM_WORLD);

    /// print global stats to stdout
    if (nrnmpi_myid == 0) {
        printf("\n================ MECHANISMS COUNT BY TYPE ==================\n");
        printf("%4s %20s %10s\n", "Id", "Name", "Count");
        for (int i = 0; i < total_mech_count.size(); i++) {
            printf("%4d %20s %10lld\n", i, nrn_get_mechname(i), total_mech_count[i]);
        }
        printf("=============================================================\n");
    }
#else
    std::cout << "Build and run with MPI to get mechanism stats \n";
#endif
}

}  // namespace coreneuron
