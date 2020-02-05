/*
Copyright (c) 2016, Blue Brain Project
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "coreneuron/apps/corenrn_parameters.hpp"

namespace coreneuron {
corenrn_parameters::corenrn_parameters(){

    app.get_formatter()->column_width(50);
    app.set_help_all_flag("-H, --help-all", "Print this help including subcommands and exit.");

    app.set_config("--config", "", "Read parameters from ini file", false)
        ->check(CLI::ExistingFile);
    app.add_flag("--mpi", this->mpi_enable, "Enable MPI. In order to initialize MPI environment this argument must be specified." );
    app.add_flag("--gpu", this->gpu, "Activate GPU computation.");
    app.add_option("--dt", this->dt, "Fixed time step. The default value is set by defaults.dat or is 0.025.", true)
        ->check(CLI::Range(-1'000.,1e9));
    app.add_option("-e, --tstop", this->tstop, "Stop Time in ms.")
        ->check(CLI::Range(0., 1e9));
    app.add_flag("--show", this->print_arg, "Print arguments.");

    auto sub_gpu = app.add_subcommand("gpu", "Commands relative to GPU.");
    sub_gpu -> add_option("-W, --nwarp", this->nwarp, "Number of warps to balance.", true)
        ->check(CLI::Range(0, 1'000'000));
    sub_gpu -> add_option("-R, --cell-permute", this->cell_interleave_permute, "Cell permutation: 0 No permutation; 1 optimise node adjacency; 2 optimize parent adjacency.", true)
        ->check(CLI::Range(0, 3));

    auto sub_input = app.add_subcommand("input", "Input dataset options.")->required(true);
    sub_input -> add_option("-d, --datpath", this->datpath, "Path containing CoreNeuron data files.")
        ->required(true)->check(CLI::ExistingPath);
    sub_input -> add_option("-f, --filesdat", this->filesdat, "Name for the distribution file.", true)
        ->check(CLI::ExistingFile);
    sub_input -> add_option("-p, --pattern", this->patternstim, "Apply patternstim using the specified spike file.")
        ->check(CLI::ExistingFile);
    sub_input -> add_option("-s, --seed", this->seed, "Initialization seed for random number generator.")
        ->check(CLI::Range(0, 100'000'000));
    sub_input -> add_option("-v, --voltage", this->voltage, "Initial voltage used for nrn_finitialize(1, v_init). If 1000, then nrn_finitialize(0,...).")
        ->check(CLI::Range(-1e9, 1e9));
    sub_input -> add_option("--read-config", this->rconfigfilepath, "Read configuration file filename.")
        ->check(CLI::ExistingPath);
    sub_input -> add_option("--report-conf", this->reportfilepath, "Reports configuration file.")
        ->check(CLI::ExistingPath);
    sub_input -> add_option("--restore", this->restorepath, "Restore simulation from provided checkpoint directory.")
        ->check(CLI::ExistingPath);

    auto sub_parallel = app.add_subcommand("parallel", "Parallel processing options.");
    sub_parallel -> add_flag("-c, --threading", this->threading, "Parallel threads. The default is serial threads.");
    sub_parallel -> add_flag("--skip-mpi-finalize", this->skip_mpi_finalize, "Do not call mpi finalize.");

    auto sub_spike = app.add_subcommand("spike", "Spike exchange options.");
    sub_spike -> add_option("--ms-phases", this->ms_phases, "Number of multisend phases, 1 or 2.", true)
        ->check(CLI::Range(1, 2));
    sub_spike -> add_option("--ms-subintervals", this->ms_subint, "Number of multisend subintervals, 1 or 2.", true)
        ->check(CLI::Range(1, 2));
    sub_spike -> add_flag("--multisend", this->multisend, "Use Multisend spike exchange instead of Allgather.");
    sub_spike -> add_option("--spkcompress", this->spkcompress, "Spike compression. Up to ARG are exchanged during MPI_Allgather.", true)
        ->check(CLI::Range(0, 100'000));
    sub_spike->add_flag("--binqueue", this->binqueue, "Use bin queue." );

    auto sub_config = app.add_subcommand("config", "Config options.");
    sub_config -> add_option("-b, --spikebuf", this->spikebuf, "Spike buffer size.", true)
        ->check(CLI::Range(0, 2'000'000'000));
    sub_config -> add_option("-g, --prcellgid", this->prcellgid, "Output prcellstate information for the gid NUMBER.")
        ->check(CLI::Range(-1, 2'000'000'000));
    sub_config -> add_option("-k, --forwardskip", this->forwardskip, "Forwardskip to TIME")
        ->check(CLI::Range(0., 1e9));
    sub_config -> add_option("-l, --celsius", this->celsius, "Temperature in degC. The default value is set in defaults.dat or else is 34.0.", true)
        ->check(CLI::Range(-1000., 1000.));
    sub_config -> add_option("-x, --extracon", this->extracon, "Number of extra random connections in each thread to other duplicate models.")
        ->check(CLI::Range(0, 10'000'000));
    sub_config -> add_option("-z, --multiple", this->multiple, "Model duplication factor. Model size is normal size * multiple")
        ->check(CLI::Range(1, 10'000'000));
    sub_config -> add_option("--mindelay", this->mindelay, "Maximum integration interval (likely reduced by minimum NetCon delay).", true)
        ->check(CLI::Range(0., 1e9));
    sub_config -> add_option("--report-buffer-size", this->report_buff_size, "Size in MB of the report buffer.")
        ->check(CLI::Range(1, 128));

    auto sub_output = app.add_subcommand("output", "Output configuration.");
    sub_output -> add_option("-i, --dt_io", this->dt_io, "Dt of I/O.", true)
        ->check(CLI::Range(-1000., 1e9));
    sub_output -> add_option("-o, --outpath", this->outpath, "Path to place output data files.", true)
        ->check(CLI::ExistingPath);
    sub_output -> add_option("--checkpoint", this->checkpointpath, "Enable checkpoint and specify directory to store related files.")
        ->check(CLI::ExistingDirectory);
};

void corenrn_parameters::parse (int argc, char** argv) {

    try {
        app.parse(argc, argv);
    } catch (const CLI::ExtrasError &e) {

        std::cerr << "Single-dash arguments such as -mpi are deprecated, please check ./coreneuron_exec --help-all for more information. \n" << std::endl;
        app.exit(e);
        throw e;

    } catch (const CLI::ParseError &e) {
        app.exit(e);
        throw e;
    }
};

std::ostream& operator<<(std::ostream& os, const corenrn_parameters& corenrn_param){

    os  << "GENERAL PARAMETERS" << std::endl
        << std::left << std::setw(15) << "MPI" << std::right << std::setw(7) << corenrn_param.mpi_enable << "      "
        << std::left << std::setw(15) << "dt" << std::right << std::setw(7) << corenrn_param.dt << "      "
        << std::left << std::setw(15) << "Tstop" << std::right << std::setw(7) << corenrn_param.tstop << std::endl
        << std::left << std::setw(15) << "Print_arg" << std::right << std::setw(7) << corenrn_param.print_arg << std::endl
        << std::endl
        << "GPU PARAMETERS" << std::endl
        << std::left << std::setw(15) << "Nwarp" << std::right << std::setw(7) << corenrn_param.nwarp << "      "
        << std::left << std::setw(15) << "Cell_perm" << std::right << std::setw(7) << corenrn_param.cell_interleave_permute << std::endl
        << std::endl
        << "INPUT PARAMETERS" << std::endl
        << std::left << std::setw(15) << "Voltage" << std::right << std::setw(7) << corenrn_param.voltage << "      "
        << std::left << std::setw(15) << "Seed" << std::right << std::setw(7) << corenrn_param.seed << std::endl
        << std::left << std::setw(15) << "Datpath" << std::right << std::setw(7) << corenrn_param.datpath << std::endl
        << std::left << std::setw(15) << "Filesdat" << std::right << std::setw(7) << corenrn_param.filesdat << std::endl;
        if (!corenrn_param.patternstim.empty()) os << std::left << std::setw(15) << "Patternstim" << std::right << std::setw(7) << corenrn_param.patternstim << std::endl;
        if (!corenrn_param.reportfilepath.empty()) os << std::left << std::setw(15) << "Reportpath" << std::right << std::setw(7) << corenrn_param.reportfilepath << std::endl;
        if (!corenrn_param.rconfigfilepath.empty()) os << std::left << std::setw(15) << "Rconfigpath" << std::right << std::setw(7) << corenrn_param.rconfigfilepath << std::endl;
        if (!corenrn_param.restorepath.empty()) os << std::left << std::setw(15) << "Restorepath" << std::right << std::setw(7) << corenrn_param.restorepath << std::endl;
        os << std::endl
        << "PARALLEL COMPUTATION PARAMETERS" << std::endl
        << std::left << std::setw(15) << "Threading" << std::right << std::setw(7) << corenrn_param.threading << "      "
        << std::left << std::setw(15) << "Skip_mpi_fin" << std::right << std::setw(7) << corenrn_param.skip_mpi_finalize << std::endl
        << std::endl
        << "SPIKE EXCHANGE" << std::endl
        << std::left << std::setw(15) << "Ms_phases" << std::right << std::setw(7) << corenrn_param.ms_phases << "      "
        << std::left << std::setw(15) << "Ms_Subint" << std::right << std::setw(7) << corenrn_param.ms_subint << "      "
        << std::left << std::setw(15) << "Multisend" << std::right << std::setw(7) << corenrn_param.multisend << std::endl
        << std::left << std::setw(15) << "Spk_compress" << std::right << std::setw(7) << corenrn_param.spkcompress << "      "
        << std::left << std::setw(15) << "Binqueue" << std::right << std::setw(7) << corenrn_param.binqueue << std::endl
        << std::endl
        << "CONFIGURATION" << std::endl
        << std::left << std::setw(15) << "Spike Buffer" << std::right << std::setw(7) << corenrn_param.spikebuf << "      "
        << std::left << std::setw(15) << "Pr Cell Gid" << std::right << std::setw(7) << corenrn_param.prcellgid << "      "
        << std::left << std::setw(15) << "Forwardskip" << std::right << std::setw(7) << corenrn_param.forwardskip << std::endl
        << std::left << std::setw(15) << "Celsius" << std::right << std::setw(7) << corenrn_param.celsius << "      "
        << std::left << std::setw(15) << "Extracon" << std::right << std::setw(7) << corenrn_param.extracon << "      "
        << std::left << std::setw(15) << "Multiple" << std::right << std::setw(7) << corenrn_param.multiple << std::endl
        << std::left << std::setw(15) << "Mindelay" << std::right << std::setw(7) << corenrn_param.mindelay << "      "
        << std::left << std::setw(15) << "Rep_buff" << std::right << std::setw(7) << corenrn_param.report_buff_size << std::endl
        << std::endl
        << "OUTPUT PARAMETERS" << std::endl
        << std::left << std::setw(15) << "dt_io" << std::right << std::setw(7) << corenrn_param.dt_io << std::endl
        << std::left << std::setw(15) << "Outpath" << std::right << std::setw(7) << corenrn_param.outpath << std::endl;
        if (!corenrn_param.checkpointpath.empty()) os << std::left << std::setw(15) << "Checkpointpath" << std::right << std::setw(7) << corenrn_param.checkpointpath<< std::endl;

    return os;
}


corenrn_parameters corenrn_param;

} // namespace coreneuron
