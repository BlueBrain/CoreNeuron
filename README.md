[![Build Status](https://travis-ci.org/BlueBrain/CoreNeuron.svg?branch=master)](https://travis-ci.org/BlueBrain/CoreNeuron)

# CoreNEURON
> Optimised simulator engine for [NEURON](https://www.neuron.yale.edu/neuron/)

CoreNEURON is a compute engine for the [NEURON](https://www.neuron.yale.edu/neuron/) simulator optimised for both memory usage and computational speed. Its goal is to simulate large cell networks with small memory footprint and optimal performance.

## Features / Compatibility

CoreNEURON is designed as library within NEURON simulator and can transparently handle all spiking network simulations including gap junction coupling with the **fixed time step method**. In order to run NEURON model with CoreNEURON,

* MOD files should be THREADSAFE
* MOD files must use Random123 random number generator (instead of MCellRan4)
* POINTER variables in MOD files need special handling. Please [open an issue](https://github.com/BlueBrain/CoreNeuron/issues) with an example of MOD file. We will add documentation about this in near future.

## Dependencies
* [CMake 3.7+](https://cmake.org)
* [MPI 2.0+](http://mpich.org) [Optional]
* [PGI OpenACC Compiler >=19.0](https://www.pgroup.com/resources/accel.htm) [Optional, for GPU support]
* [CUDA Toolkit >=6.0](https://developer.nvidia.com/cuda-toolkit-60) [Optional, for GPU support]

In addition to this, you will need other [NEURON dependencies](https://github.com/neuronsimulator/nrn) like Python, Flex, Bison etc.

## Installation

CoreNEURON is now integrated into development version of NEURON simulator. If you are a NEURON user, preferred way to install CoreNEURON is to enable extra build options during NEURON installation as follows:

1. Clone latest version of NEURON:

  ```
  git clone https://github.com/neuronsimulator/nrn
  cd nrn
  ```

2. Create a build directory:

  ```
  mkdir build
  cd build
  ```

3. Load modules : Currently CoreNEURON version rely on compiler auto-vectorisation and hence we advise to use Intel/Cray/PGI compiler to get better performance. This constraint will be removed in near future with the integration of [NMODL](https://github.com/BlueBrain/nmodl) project. Load necessary modules on your system, e.g.

	```
	module load intel intel-mpi python cmake
	```
If you are building on Cray system with GNU toolchain, set following environmental variable:

	```bash
	export CRAYPE_LINK_TYPE=dynamic
	```

3. Run CMake with the appropriate [options](https://github.com/neuronsimulator/nrn#build-using-cmake) and additionally enable CoreNEURON with `-DNRN_ENABLE_CORENEURON=ON` option:

  ```bash
  cmake .. \
   -DNRN_ENABLE_CORENEURON=ON \
   -DNRN_ENABLE_INTERVIEWS=OFF \
   -DNRN_ENABLE_RX3D=OFF \
   -DCMAKE_INSTALL_PREFIX=$HOME/install
  ```
If you would like to enable GPU support with OpenACC, make sure to use `-DCORENRN_ENABLE_GPU=ON` option and use PGI compiler with CUDA.
> NOTE : if you see error and re-run CMake command then make sure to remove temporary CMake cache files by deleting `CMakeCache.txt`.

4. Build and Install :  once configure step is done, you can build and install project as:

	```bash
	make -j
	make install
	```

## Building Model

Once NEURON is installed with CoreNEURON support, setup `PATH` and `PYTHONPATH ` variables as:

```
export PYTHONPATH=$HOME/install/lib/python:$PYTHONPATH
export PATH=$HOME/install/bin:$PATH
```

Like typical NEURON workflow, you can use `nrnivmodl` to translate MOD files:

```
nrnivmodl mod_directory
```

In order to enable CoreNEURON support, we have to use `-coreneuron` flag:

```
nrnivmodl -coreneuron mod_directory
```

If you see any compilation error then one of the mod file might be incompatible with CoreNEURON. Please [open an issue](https://github.com/BlueBrain/CoreNeuron/issues) with an example and we can help to fix it.


## Running Simulations

With CoreNEURON, existing NEURON models can be run with minimal. If you have existing NEURON model, we typically need to make following changes:

1. Enable cache effficiency : `h.cvode.cache_efficient(1)`
2. Enable CoreNEURON :

	```
	from neuron import coreneuron
	coreneuron.enable = True
	```
3. Use `psolve` to run simulation after initialization :

	```
	h.stdinit()
	pc.psolve(h.tstop)
	```

Here is a simple example of model that run with NEURON first followed by CoreNEURON and compares results between NEURON and CoreNEURON execution:


```python
import sys
from neuron import h, gui

# setup model
h('''create soma''')
h.soma.L=5.6419
h.soma.diam=5.6419
h.soma.insert("hh")
h.soma.nseg = 3
ic = h.IClamp(h.soma(.25))
ic.delay = .1
ic.dur = 0.1
ic.amp = 0.3

ic2 = h.IClamp(h.soma(.75))
ic2.delay = 5.5
ic2.dur = 1
ic2.amp = 0.3

h.tstop = 10

# make sure to enable cache efficiency
h.cvode.cache_efficient(1)

pc = h.ParallelContext()
pc.set_gid2node(pc.id()+1, pc.id())
myobj = h.NetCon(h.soma(0.5)._ref_v, None, sec=h.soma)
pc.cell(pc.id()+1, myobj)

# First run NEURON and record spikes
nrn_spike_t = h.Vector()
nrn_spike_gids = h.Vector()
pc.spike_record(-1, nrn_spike_t, nrn_spike_gids)
h.run()

# copy vector as numpy array
nrn_spike_t = nrn_spike_t.to_python()
nrn_spike_gids = nrn_spike_gids.to_python()

# now run CoreNEURON
from neuron import coreneuron
coreneuron.enable = True
coreneuron.verbose = 0
h.stdinit()
corenrn_all_spike_t = h.Vector()
corenrn_all_spike_gids = h.Vector()
pc.spike_record(-1, corenrn_all_spike_t, corenrn_all_spike_gids )
pc.psolve(h.tstop)

# copy vector as numpy array
corenrn_all_spike_t = corenrn_all_spike_t.to_python()
corenrn_all_spike_gids = corenrn_all_spike_gids.to_python()

# check spikes match between NEURON and CoreNEURON
assert(nrn_spike_t == corenrn_all_spike_t)
assert(nrn_spike_gids == corenrn_all_spike_gids)

h.quit()
```

We can run this model as:

```
python test.py
```

You can find [HOC example](https://github.com/neuronsimulator/nrn/blob/master/test/coreneuron/test_direct.hoc) here.

## FAQs

#### What results are returned by CoreNEURON?

At the end of simulation, CoreNEURON can transfers spikes, voltages, state variables and NetCon weights to NEURON. These variables can be recorded using regular NEURON API (e.g. [Vector.record](https://www.neuron.yale.edu/neuron/static/py_doc/programming/math/vector.html#Vector.record) or [spike_record](https://www.neuron.yale.edu/neuron/static/new_doc/modelspec/programmatic/network/parcon.html#ParallelContext.spike_record)).

#### How can I poass additional flags to build?

One can specify C/C++ optimization flags specific to the compiler with `-DCMAKE_CXX_FLAGS` and `-DCMAKE_C_FLAGS` options to the CMake command. For example:

```bash
cmake .. -DCMAKE_CXX_FLAGS="-O3 -g" \
         -DCMAKE_C_FLAGS="-O3 -g" \
         -DCMAKE_BUILD_TYPE=CUSTOM
```

By default, OpenMP threading is enabled. You can disable it with `-DCORENRN_ENABLE_OPENMP=OFF`

#### GPU enabled build is failing with inlining related errors, what to do?

If there are large functions / procedures in MOD file that are not inlined by compiler, you may need to pass additional C++ flags to PGI compiler. You can try:

```
cmake .. -DCMAKE_CXX_FLAGS="-O2 -Minline=size:1000,levels:100,totalsize:40000,maxsize:4000" \
         -DCORENRN_ENABLE_GPU=ON -DCMAKE_INSTALL_PREFIX=$HOME/install
```


## Developer Build

##### Building standalone CoreNEURON

If you want to build standalone CoreNEURON version, first download repository as:

```
git clone https://github.com/BlueBrain/CoreNeuron.git

```

Once appropriate modules for compiler, MPI, CMake are loaded, you can build CoreNEURON with:

```bash
mkdir CoreNeuron/build && cd CoreNeuron/build
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/install
make -j && make install
```

If you don't have MPI, you can disable MPI dependency using the CMake option `-DCORENRN_ENABLE_MPI=OFF`. Once build is successful, you can run tests using:

```
make test
```

##### Compiling MOD files

In order to compiler mod files, one can use **nrnivmodl-core** as:

```bash
/install-path/bin/nrnivmodl-core mod-dir
```

This will create `special-core` executable under `<arch>` directory.

##### Building with GPU support

CoreNEURON has support for GPUs using the OpenACC programming model when enabled with `-DCORENRN_ENABLE_GPU=ON`. Below are the steps to compile with PGI compiler:

```bash
module purge all
module load pgi/19.4 cuda/10 cmake intel-mpi # change pgi, cuda and mpi modules
cmake .. -DCORENRN_ENABLE_GPU=ON -DCMAKE_INSTALL_PREFIX=$HOME/install
make -j && make install
```

Note that the CUDA Toolkit version should be compatible with PGI compiler installed on your system. Otherwise, you have to add extra C/C++ flags. For example, if we are using CUDA Toolkit 9.0 installation but PGI default target is CUDA 8.0 then we have to add :

```bash
-DCMAKE_C_FLAGS:STRING="-O2 -ta=tesla:cuda9.0" -DCMAKE_CXX_FLAGS:STRING="-O2 -ta=tesla:cuda9.0"
```

You have to run GPU executable with the `--gpu` flag. Make sure to enable cell re-ordering mechanism to improve GPU performance using `--cell_permute` option (permutation types : 2 or 1):

```bash
mpirun -n 1 ./bin/nrniv-core --mpi --gpu --tstop 100 --datpath ../tests/integration/ring --cell-permute 2
```

> Note: that if your model is using Random123 random number generator, you can't use same executable for CPU and GPU runs. We suggest to build separate executable for CPU and GPU simulations. This will be fixed in future releases.


##### Running tests with SLURM

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
cmake .. -CORENRN_ENABLE_UNIT_TESTS=OFF
```

##### CLI Options

To see all CLI options for CoreNEURON, see `./bin/nrniv-core -h`.

##### Formatting CMake and C++ Code

In order to format code with `cmake-format` and `clang-format` tools, before creating PR, enable below CMake options:

```
cmake .. -DCORENRN_CLANG_FORMAT=ON -DCORENRN_CMAKE_FORMAT=ON
make -j
```

and now you can use `cmake-format` or `clang-format` targets:

```
make cmake-format
make clang-format
```

### Citation

If you would like to know more about the the CoreNEURON or would like to cite it then use following paper:

* Pramod Kumbhar, Michael Hines, Jeremy Fouriaux, Aleksandr Ovcharenko, James King, Fabien Delalondre and Felix Schürmann. CoreNEURON : An Optimized Compute Engine for the NEURON Simulator ([doi.org/10.3389/fninf.2019.00063](https://doi.org/10.3389/fninf.2019.00063))


### Support / Contribuition

If you see any issue, feel free to [raise a ticket](https://github.com/BlueBrain/CoreNeuron/issues/new). If you would like to improve this library, see [open issues](https://github.com/BlueBrain/CoreNeuron/issues).

You can see current [contributors here](https://github.com/BlueBrain/CoreNeuron/graphs/contributors).


## License
* See LICENSE.txt
* See [NEURON](https://www.neuron.yale.edu/neuron/)


## Funding

CoreNEURON is developed in a joint collaboration between the Blue Brain Project and Yale University. This work has been funded by the EPFL Blue Brain Project (funded by the Swiss ETH board), NIH grant number R01NS11613 (Yale University), the European Union Seventh Framework Program (FP7/20072013) under grant agreement n◦ 604102 (HBP) and the Eu- ropean Union’s Horizon 2020 Framework Programme for Research and Innovation under Grant Agreement n◦ 720270 (Human Brain Project SGA1) and Grant Agreement n◦ 785907 (Human Brain Project SGA2).
