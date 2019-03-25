//
// Created by bellotta on 3/22/19.
//

#ifndef CN_PARAMETERS_H
#define CN_PARAMETERS_H

#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
#include "coreneuron/utils/CLI11/CLI.hpp"

typedef std::string string_t;

struct cn_parameters {

    int spikebuf=100000;   /// internal buffer used on every rank for spikes
    int prcellgid=-1;         /// gid of cell for prcellstate
    int ms_phases=2;
    int ms_subint=2;
    int spkcompress=0;
    int cell_interleave_permute=0; /// cell interleaving permutation
    int nwarp=0;                   /// number of warps to balance for cell_interleave_permute == 2
    int multiple=1;
    int extracon=0;
    int report_buff_size=4;
    int seed=0;                       /// Initialization seed for random number generator (int)

    bool mpi_en=0;                    /// Enable MPI flag.
    bool print_arg=0;                 /// Print arguments flag.
    bool skip_mpi_finalize=0;         /// Skip MPI finalization
    bool multisend=0;
    bool threading=0;         /// enable pthread/openmp
    bool gpu=0;
    bool binqueue=0;

    double tstop=100;     /// stop time of simulation in msec
    double dt=0.025;      /// timestep to use in msec
    double dt_io=0.1;     /// i/o timestep to use in msec
    double dt_report;     /// i/o timestep to use in msec for reports
    double celsius=34.0;
    double voltage=-65.0;
    double forwardskip=0.;
    double mindelay=10.;

    string_t patternstim;          /// Apply patternstim using the specified spike file.
    string_t datpath=".";          /// directory path where .dat files
    string_t outpath=".";          /// directory where spikes will be written
    string_t filesdat="files.dat"; /// name of file containing list of gids dat files read in
    string_t rconfigpath;
    string_t restorepath;
    string_t reportpath;
    string_t checkpointpath;

};

std::ostream& operator<<(std::ostream& os, const cn_parameters& cn_par);

extern cn_parameters cn_par;

#endif //CN_PARAMETERS_H
