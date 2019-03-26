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

        "--voltage",
        "-32",

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

        "--dt_io",
        "0.2"
        };

    int argc = 0;

    for (; strcmp(argv[argc], "0.2"); argc++);

    argc++;
    
    cn_parameters cn_par_test;

    cn_par_test.parse(argc, const_cast<char**>(argv)); //discarding const as CLI11 interface is not const
    
    BOOST_CHECK(cn_par_test.seed == 0);            // testing default value

    BOOST_CHECK(cn_par_test.spikebuf == 100);

    BOOST_CHECK(cn_par_test.threading == true);

    BOOST_CHECK(cn_par_test.dt == 0.02);

    BOOST_CHECK(cn_par_test.tstop == 0.1);

    BOOST_CHECK(cn_par_test.prcellgid == 12);

    BOOST_CHECK(cn_par_test.gpu == true);

    BOOST_CHECK(cn_par_test.dt_io == 0.2);

    BOOST_CHECK(cn_par_test.forwardskip == 0.02);

    BOOST_CHECK(cn_par_test.celsius == 25.12);

    BOOST_CHECK(cn_par_test.mpi_en == true);

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

}
