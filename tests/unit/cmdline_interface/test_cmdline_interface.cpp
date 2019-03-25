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

#define BOOST_TEST_MODULE cmdline_interface
#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>
#include "nrniv/nrnoptarg.h"
#include <float.h>
#include <utils/CLI11/CLI.hpp>
#include "coreneuron/nrniv/cn_parameters.h"

using namespace coreneuron;

BOOST_AUTO_TEST_CASE(cmdline_interface) {

    const char* argv[] = {

        "coreneuron_exec",

        "--mpi", //generic commands

        "--dt",
        "0.02",

        "--tstop",
        "0.1",

        "--gpu",

        "gpu", //gpu commands

        "--cell-permute",
        "2",

        "--nwarp",
        "8",

        "input", //input subcommands

        "--datpath",
        "/this/is/the/data/path",

        "--filesdat",
        "/this/is/the/file/path",

        "--pattern",
        "filespike.dat",

        "--voltage",
        "-32",

        "--report-conf",
        "report.conf",

        "parallel", //parallel subcommands

        "--threading",

        "spike", //spike exchange subcommands

        "--ms-phases",
        "1",

        "--ms-subintervals",
        "2",

        "--multisend",

        "--spkcompress",
        "32",

        "--binqueue",

        "config", //config subcommands

        "--spikebuf",
        "100",

        "--prcellgid",
        "12",

        "--forwardskip",
        "0.02",

        "--celsius",
        "25.12",

        "--extracon",
        "1000",

        "--multiple",
        "3",

        "--mindelay",
        "0.1",

        "output", //output subcommands

        "--checkpoint",
        "/this/is/the/chkp/path",

        "--dt_io",
        "0.2",

        "--outpath",
        "/this/is/the/output/path"};

    int argc = 0;

    for (; strcmp(argv[argc], "/this/is/the/output/path"); argc++);

    argc++;
    
    cn_parameters cn_par_test;

    CLI::App app_test{"CoreNeuron - Command Line Test!"};

    (app_test).parse((argc), (argv));
    
    BOOST_CHECK(cn_par_test.seed == 0);            // testing default value

    BOOST_CHECK(cn_par_test.spikebuf == 100);

    BOOST_CHECK(cn_par_test.threading == true);

    BOOST_CHECK(!strcmp(cn_par_test.datpath.c_str(), "/this/is/the/data/path"));

    BOOST_CHECK(!strcmp(cn_par_test.checkpointpath.c_str(), "/this/is/the/chkp/path"));

    BOOST_CHECK(cn_par_test.dt == 0.02);

    BOOST_CHECK(cn_par_test.tstop == 0.1);

    BOOST_CHECK(!strcmp(cn_par_test.filesdat.c_str(), "/this/is/the/file/path"));

    BOOST_CHECK(cn_par_test.prcellgid == 12);

    BOOST_CHECK(cn_par_test.gpu == true);

    BOOST_CHECK(cn_par_test.dt_io == 0.2);

    BOOST_CHECK(cn_par_test.forwardskip == 0.02);

    BOOST_CHECK(cn_par_test.celsius == 25.12);

    BOOST_CHECK(cn_par_test.mpi_en == true);

    BOOST_CHECK(!strcmp(cn_par_test.outpath.c_str(), "/this/is/the/output/path"));

    BOOST_CHECK(!strcmp(cn_par_test.patternstim.c_str(), "filespike.dat"));

    BOOST_CHECK(!strcmp(cn_par_test.reportpath.c_str(), "report.conf"));

    BOOST_CHECK(cn_par_test.cell_interleave_permute == 2);

    BOOST_CHECK(cn_par_test.voltage == -32);

    BOOST_CHECK(cn_par_test.nwarp == 8);

    BOOST_CHECK(cn_par_test.extracon == 1000);

    BOOST_CHECK(cn_par_test.multiple == 3);

    BOOST_CHECK(cn_par_test.multisend == true);

    BOOST_CHECK(cn_par_test.mindelay == 0.1);

    BOOST_CHECK(cn_par_test.ms_phases == 1);

    BOOST_CHECK(cn_par_test.ms_subint == 2);

    BOOST_CHECK(cn_par_test.spkcompress == 32);

    BOOST_CHECK(cn_par_test.multisend == true);



    cn_par_test.dt = 18.1;
    BOOST_CHECK(cn_par_test.dt == 18.1);

    // check if default flags are false
    const char* argv_empty[] = {"coreneuron_exec"};
    argc = 1;

    nrnopt_parse(argc, argv_empty);

    BOOST_CHECK(cn_par_test.threading == false);
    BOOST_CHECK(cn_par_test.gpu == false);
    BOOST_CHECK(cn_par_test.mpi_en == false);
    BOOST_CHECK(cn_par_test.binqueue == false);
    BOOST_CHECK(cn_par_test.multisend == false);
}
