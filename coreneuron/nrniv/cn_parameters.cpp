//
// Created by bellotta on 3/22/19.
//

#include "coreneuron/nrniv/cn_parameters.h"

std::ostream& operator<<(std::ostream& os, const cn_parameters& cn_par){

    os  << "GENERAL PARAMETERS" << std::endl
        << std::left << std::setw(15) << "MPI" << std::right << std::setw(7) << cn_par.mpi_en << "      "
        << std::left << std::setw(15) << "dt" << std::right << std::setw(7) << cn_par.dt << "      "
        << std::left << std::setw(15) << "Tstop" << std::right << std::setw(7) << cn_par.tstop << std::endl
        << std::left << std::setw(15) << "Print_arg" << std::right << std::setw(7) << cn_par.print_arg << std::endl
        << std::endl
        << "GPU PARAMETERS" << std::endl
        << std::left << std::setw(15) << "Nwarp" << std::right << std::setw(7) << cn_par.nwarp << "      "
        << std::left << std::setw(15) << "Cell_perm" << std::right << std::setw(7) << cn_par.cell_interleave_permute << std::endl
        << std::endl
        << "INPUT PARAMETERS" << std::endl
        << std::left << std::setw(15) << "Voltage" << std::right << std::setw(7) << cn_par.voltage << "      "
        << std::left << std::setw(15) << "Seed" << std::right << std::setw(7) << cn_par.seed << std::endl
        << std::left << std::setw(15) << "Datpath" << std::right << std::setw(7) << cn_par.datpath << std::endl
        << std::left << std::setw(15) << "Filesdat" << std::right << std::setw(7) << cn_par.filesdat << std::endl
        << std::left << std::setw(15) << "Patternstim" << std::right << std::setw(7) << cn_par.patternstim << std::endl
        << std::left << std::setw(15) << "Reportpath" << std::right << std::setw(7) << cn_par.reportpath << std::endl
        << std::left << std::setw(15) << "Rconfigpath" << std::right << std::setw(7) << cn_par.rconfigpath << std::endl
        << std::left << std::setw(15) << "Restorepath" << std::right << std::setw(7) << cn_par.restorepath << std::endl
        << std::endl
        << "PARALLEL COMPUTATION PARAMETERS" << std::endl
        << std::left << std::setw(15) << "Threading" << std::right << std::setw(7) << cn_par.threading << "      "
        << std::left << std::setw(15) << "Skip_mpi_fin" << std::right << std::setw(7) << cn_par.skip_mpi_finalize << std::endl
        << std::endl
        << "SPIKE EXCHANGE" << std::endl
        << std::left << std::setw(15) << "Ms_phases" << std::right << std::setw(7) << cn_par.ms_phases << "      "
        << std::left << std::setw(15) << "Ms_Subint" << std::right << std::setw(7) << cn_par.ms_subint << "      "
        << std::left << std::setw(15) << "Multisend" << std::right << std::setw(7) << cn_par.multisend << std::endl
        << std::left << std::setw(15) << "Spk_compress" << std::right << std::setw(7) << cn_par.spkcompress << "      "
        << std::left << std::setw(15) << "Binqueue" << std::right << std::setw(7) << cn_par.binqueue << std::endl
        << std::endl
        << "CONFIGURATION" << std::endl
        << std::left << std::setw(15) << "Spike Buffer" << std::right << std::setw(7) << cn_par.spikebuf << "      "
        << std::left << std::setw(15) << "Pr Cell Grid" << std::right << std::setw(7) << cn_par.prcellgid << "      "
        << std::left << std::setw(15) << "Forwardskip" << std::right << std::setw(7) << cn_par.forwardskip << std::endl
        << std::left << std::setw(15) << "Celsius" << std::right << std::setw(7) << cn_par.celsius << "      "
        << std::left << std::setw(15) << "Extracon" << std::right << std::setw(7) << cn_par.extracon << "      "
        << std::left << std::setw(15) << "Multiple" << std::right << std::setw(7) << cn_par.multiple << std::endl
        << std::left << std::setw(15) << "Mindelay" << std::right << std::setw(7) << cn_par.mindelay << "      "
        << std::left << std::setw(15) << "Rep_buff" << std::right << std::setw(7) << cn_par.report_buff_size << std::endl
        << std::endl
        << "OUTPUT PARAMETERS" << std::endl
        << std::left << std::setw(15) << "dt_io" << std::right << std::setw(7) << cn_par.dt_io << std::endl
        << std::left << std::setw(15) << "Outpath" << std::right << std::setw(7) << cn_par.outpath << std::endl
        << std::left << std::setw(15) << "Checkpointpath" << std::right << std::setw(7) << cn_par.checkpointpath<< std::endl;

    return os;
}


cn_parameters cn_par;