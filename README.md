# CoreNEURON
> Optimised simulator engine for [NEURON](https://www.neuron.yale.edu/neuron/)

CoreNEURON is a compute engine for the [NEURON](https://www.neuron.yale.edu/neuron/) simulator optimised for both memory usage and computational speed. Its goal is to simulate large cell networks with minimal memory footprint and optimal performance.

If you are a new user and would like to use CoreNEURON, [this tutorial](https://github.com/nrnhines/ringtest) will be a good starting point to understand complete workflow of using CoreNEURON with NEURON.


## Features

CoreNEURON can transparently handle all spiking network simulations including gap junction coupling with the fixed time step method. The model descriptions written in NMODL need to be thread safe to exploit vector units of modern CPUs and GPUs. The NEURON models must use Random123 random number generator.


## Dependencies
* [CMake 3.0.12+](https://cmake.org)
* [MOD2C](http://github.com/BlueBrain/mod2c)
* [MPI 2.0+](http://mpich.org) [Optional]
* [PGI OpenACC Compiler >=18.0](https://www.pgroup.com/resources/accel.htm) [Optional, for GPU systems]
* [CUDA Toolkit >=6.0](https://developer.nvidia.com/cuda-toolkit-60) [Optional, for GPU systems]


## Installation

This project uses git submodules which must be cloned along with the repository itself:

```
git clone --recursive https://github.com/BlueBrain/CoreNeuron.git

```

Set the appropriate MPI wrappers for the C and C++ compilers, e.g.:

```bash
export CC=mpicc
export CXX=mpicxx
```

And build using:

```bash
cmake ..
make -j
```

If you don't have MPI, you can disable MPI dependency using the CMake option `-DENABLE_MPI=OFF`:

```bash
export CC=gcc
export CXX=g++
cmake .. -DENABLE_MPI=OFF
make -j
```

And you can run inbuilt tests using:

```
make test
```

### About MOD files

The workflow for building CoreNEURON is different from that of NEURON, especially considering the use of **nrnivmodl**. Currently we do not provide **nrnivmodl-core** for CoreNEURON. If you have MOD files from a NEURON model, you have to explicitly specify those MOD file directory paths during CoreNEURON build using the `-DADDITIONAL_MECHPATH` option:

```bash
cmake .. -DADDITIONAL_MECHPATH="path/of/mod/files/directory/"
```


## Building with GPU support

CoreNEURON has support for GPUs using the OpenACC programming model when enabled with `-DENABLE_OPENACC=ON`. Below are the steps to compile with PGI compiler:

```bash
module purge
module purge all
module load pgi/18.4 cuda/9.0.176 cmake intel-mpi # change pgi, cuda and mpi modules

export CC=mpicc
export CXX=mpicxx

cmake ..  -DCMAKE_C_FLAGS:STRING="-O2" \
          -DCMAKE_CXX_FLAGS:STRING="-O2" \
          -DCOMPILE_LIBRARY_TYPE=STATIC \
          -DCUDA_HOST_COMPILER=`which gcc` \
          -DCUDA_PROPAGATE_HOST_FLAGS=OFF \
          -DENABLE_SELECTIVE_GPU_PROFILING=ON \
          -DENABLE_OPENACC=ON
```

Note that the CUDA Toolkit version should be compatible with PGI compiler installed on your system. Otherwise you have to add extra C/C++ flags. For example, if we are using CUDA Toolkit 9.0 installation but PGI default target is CUDA 8.0 then we have to add :

```bash
-DCMAKE_C_FLAGS:STRING="-O2 -ta=tesla:cuda9.0" -DCMAKE_CXX_FLAGS:STRING="-O2 -ta=tesla:cuda9.0"
```

> If there are large functions / procedures in MOD file that are not inlined by compiler, you need to pass additional c/c++ compiler flags: `-Minline=size:1000,levels:100,totalsize:40000,maxsize:4000`

You have to run GPU executable with the `--gpu` or `-gpu`. Make sure to enable cell re-ordering mechanism to improve GPU performance using `--cell_permute` option (permutation types : 2 or 1):

```bash
mpirun -n 1 ./bin/coreneuron_exec -d ../tests/integration/ring -mpi -e 100 --gpu --cell_permute 2
```

Note that if your model is using Random123 random number generator, you can't use same executable for CPU and GPU runs. We suggest to build separate executable for CPU and GPU simulations. This will be fixed in future releases.


## Building on Cray System

On a Cray system the user has to provide the path to the MPI library as follows:

```bash
export CC=`which cc`
export CXX=`which CC`
cmake -DMPI_C_INCLUDE_PATH=$MPICH_DIR/include -DMPI_C_LIBRARIES=$MPICH_DIR/lib
make -j
```

## Optimization Flags

* One can specify C/C++ optimization flags specific to the compiler and architecture with `-DCMAKE_CXX_FLAGS` and `-DCMAKE_C_FLAGS` options to the CMake command. For example:

```bash
cmake .. -DCMAKE_CXX_FLAGS="-O3 -g" \
         -DCMAKE_C_FLAGS="-O3 -g" \
         -DCMAKE_BUILD_TYPE=CUSTOM
```

* By default OpenMP threading is enabled. You can disable it with `-DCORENEURON_OPENMP=OFF`
* By default CoreNEURON uses the SoA (Structure of Array) memory layout for all data structures. You can switch to AoS using `-DENABLE_SOA=OFF`.


## RUNNING SIMULATION:

Note that the CoreNEURON simulator depends on NEURON to build the network model: see [NEURON](https://www.neuron.yale.edu/neuron/) documentation for more information. Once you build the model using NEURON, you can launch CoreNEURON on the same or different machine by:

```bash
export OMP_NUM_THREADS=2     #set appropriate value
mpiexec -np 2 build/apps/coreneuron_exec -e 10 --mpi input -d /path/to/model/built/by/neuron
```

[This tutorial](https://github.com/nrnhines/ringtest) provide more information for parallel runs and performance comparison.

### Command Line Interface

:warning: :warning: :warning: **In a recent update the command line interface was updated, so please update your scripts accordingly!**

Some details on the new interface:

The new command line interface is based on CLI11. All the previous options are still supported but they are organized differently. You can find more details by running `coreneuron_exec --help-all`.

Multiple character options with single dash (e.g. `-gpu`) are not supported anymore. All multiple characters options now require a double dash (e.g. `--gpu`), but single characters option still support a single dash (e.g. `-g`).

The structure of a command is the following: `./apps/coreneuron_exec [OPTIONS] [SUBCOMMAND]`. The `OPTIONS` should **always** be written before any of the `SUBCOMMAND`. To know which of the parameter is an `OPTION` or a `SUBCOMMAND` run `coreneuron_exec --help-all`.

To access a parameter that falls under any of the subcommands category it is necessary to include in the command line a certain keyworld first. For example to use the `-d` option, one must enter in the `input` subcommands first by using the `input` keyword first:
`coreneuron_exec -d /path/to/model/built/by/neuron` becomes `coreneuron_exec input -d /path/to/model/built/by/neuron`

To define **more than one** option in a certain subcommand it is needed to state the **subcommand keyword** (for example `input`) **directly followed** by the **options** of this subcommand (for example `-d /path/to/model -f /path/to/filesdat/file.dat`). The full command would be:
`coreneuron_exec --mpi input -d /path/to/model -f /path/to/filesdat/file.dat`.

In order to see the command line options, you can use:

```bash
/path/to/install/directory/coreneuron_exec -H                                             
CoreNeuron - Optimised Simulator Engine for NEURON.
Usage: ./apps/coreneuron_exec [OPTIONS] [SUBCOMMAND]

Options:
  -h,--help                                       Print this help message and exit
  -H,--help-all                                   Print this help including subcommands and exit.
  --config TEXT:FILE=config.ini                   Read parameters from ini file
  --mpi                                           Enable MPI. In order to initialize MPI environment this argument must be specified.
  --gpu                                           Activate GPU computation.
  --dt FLOAT:FLOAT in [-1000 - 1e+09]=0.025       Fixed time step. The default value is set by defaults.dat or is 0.025.
  -e,--tstop FLOAT:FLOAT in [0 - 1e+09]           Stop Time in ms.
  --show                                          Print arguments.

Subcommands:
gpu
  Commands relative to GPU.
  Options:
    -W,--nwarp INT:INT in [0 - 1000000]=0           Number of warps to balance.
    -R,--cell-permute INT:INT in [0 - 3]=0          Cell permutation: 0 No permutation; 1 optimise node adjacency; 2 optimize parent adjacency.

input
  Input dataset options. REQUIRED 
  Options:
    -d,--datpath TEXT:PATH(existing) REQUIRED       Path containing CoreNeuron data files.
    -f,--filesdat TEXT:FILE=files.dat               Name for the distribution file.
    -p,--pattern TEXT:FILE                          Apply patternstim using the specified spike file.
    -s,--seed INT:INT in [0 - 100000000]            Initialization seed for random number generator.
    -v,--voltage FLOAT:FLOAT in [-1e+09 - 1e+09]    Initial voltage used for nrn_finitialize(1, v_init). If 1000, then nrn_finitialize(0,...).
    --read-config TEXT:PATH(existing)               Read configuration file filename.
    --report-conf TEXT:PATH(existing)               Reports configuration file.
    --restore TEXT:PATH(existing)                   Restore simulation from provided checkpoint directory.

parallel
  Parallel processing options.
  Options:
    -c,--threading                                  Parallel threads. The default is serial threads.
    --skip-mpi-finalize                             Do not call mpi finalize.

spike
  Spike exchange options.
  Options:
    --ms-phases INT:INT in [1 - 2]=2                Number of multisend phases, 1 or 2.
    --ms-subintervals INT:INT in [1 - 2]=2          Number of multisend subintervals, 1 or 2.
    --multisend                                     Use Multisend spike exchange instead of Allgather.
    --spkcompress INT:INT in [0 - 100000]=0         Spike compression. Up to ARG are exchanged during MPI_Allgather.
    --binqueue                                      Use bin queue.

config
  Config options.
  Options:
    -b,--spikebuf INT:INT in [0 - 2000000000]=100000
                                                    Spike buffer size.
    -g,--prcellgid INT:INT in [-1 - 2000000000]     Output prcellstate information for the gid NUMBER.
    -k,--forwardskip FLOAT:FLOAT in [0 - 1e+09]     Forwardskip to TIME
    -l,--celsius FLOAT:FLOAT in [-1000 - 1000]=34   Temperature in degC. The default value is set in defaults.dat or else is 34.0.
    -x,--extracon INT:INT in [0 - 10000000]         Number of extra random connections in each thread to other duplicate models.
    -z,--multiple INT:INT in [1 - 10000000]         Model duplication factor. Model size is normal size * multiple
    --mindelay FLOAT:FLOAT in [0 - 1e+09]=10        Maximum integration interval (likely reduced by minimum NetCon delay).
    --report-buffer-size INT:INT in [1 - 128]       Size in MB of the report buffer.

output
  Output configuration.
  Options:
    -i,--dt_io FLOAT:FLOAT in [-1000 - 1e+09]=0.1   Dt of I/O.
    -o,--outpath TEXT:PATH(existing)=.              Path to place output data files.
    --checkpoint TEXT:DIR                           Enable checkpoint and specify directory to store related files.
```


## Results

Currently CoreNEURON only outputs spike data as `out.dat` file.

## Running tests

Once you compile CoreNEURON, unit tests and a ring test will be compiled if Boost is available. You can run tests using

```bash
make test
```

If you have different mpi launcher, you can specify it during cmake configuration as:

```bash
cmake .. -DTEST_MPI_EXEC_BIN="mpirun" \
         -DTEST_EXEC_PREFIX="mpirun;-n;2" \
         -DTEST_EXEC_PREFIX="mpirun;-n;2" \
         -DAUTO_TEST_WITH_SLURM=OFF \
         -DAUTO_TEST_WITH_MPIEXEC=OFF \
```
You can disable tests using with options:

```
cmake .. -DUNIT_TESTS=OFF -DFUNCTIONAL_TESTS=OFF
```

## License
* See LICENSE.txt
* See [NEURON](https://www.neuron.yale.edu/neuron/)
* [NMC portal](https://bbp.epfl.ch/nmc-portal/copyright) provides more license information
about ME-type models in testsuite

## Contributors
See [contributors](https://github.com/BlueBrain/CoreNeuron/graphs/contributors).


## Funding

CoreNEURON is developed in a joint collaboration between the Blue Brain Project and Yale University. This work has been funded by the EPFL Blue Brain Project (funded by the Swiss ETH board), NIH grant number R01NS11613 (Yale University), the European Union Seventh Framework Program (FP7/20072013) under grant agreement n◦ 604102 (HBP) and the Eu- ropean Union’s Horizon 2020 Framework Programme for Research and Innovation under Grant Agreement n◦ 720270 (Human Brain Project SGA1) and Grant Agreement n◦ 785907 (Human Brain Project SGA2).