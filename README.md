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


## Installation

CoreNEURON is now integrated into latest development version of NEURON simulator. If you are a NEURON user, preferred way to install CoreNEURON is to enable extra build options duriing NEURON installation as follows:

1. Clone latest version of NEUERON:

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
> NOTE : if you see error and re-run CMake command then make sure to remove temorary CMake cache files by deleting build directory (to avoid cached build results).

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

Like typical NEURON workflow, we have to use `nrnivmodl` to translate MOD files. In addition, we have to use `nrnvmodl-core` to translate mod files for CoreNEURON:

```
nrnivmodl-core mod_directory
nrnivmodl mod_directory
```

If you see any compilation error then one of the mod file is incompatible with CoreNEURON. Please [open an issue](https://github.com/BlueBrain/CoreNeuron/issues) with an example and we can help to fix it.


**NOTE** : If you are building with GPU support, then `nrnivmodl` needs additional flags:

```
nrnivmodl -incflags "-acc" -loadflags "-acc -rdynamic -lrt -Wl,--whole-archive -Lx86_64/ -lcorenrnmech -L$HOME/install/lib -lcoreneuron -lcudacoreneuron  -Wl,--no-whole-archive $CUDA_HOME/lib64/libcudart_static.a" .
```
We are working on fixes to avoid this additional step.

## Running Simulations

With CoreNEURON, existing NEURON models can be run with minimal to no changes. If you have existing NEURON model, we typically need to make following changes:

* Enable cache effficiency : `h.cvode.cache_efficient(1)`
* Enable CoreNEURON :

	```
	from neuron import coreneuron
	coreneuron.enable = True
	```
* Use `psolve` to run simulation after initialization :

	```
	h.stdinit()
	pc.psolve(h.tstop)
	```

Here is simple example of model that run NEURON first followed by CoreNEURON and compares results between NEURON and CoreNEURON:


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

and run this model as:

```
python test.py
```

You can find [HOC example](https://github.com/neuronsimulator/nrn/blob/master/test/coreneuron/test_direct.hoc) here.

## Additional Notes

#### Results

Currently CoreNEURON transfers spikes, voltages and all state variables to NEURON. These variables can be recorded using regular NEURON API (e.g. vector record).

#### Optimization Flags

One can specify C/C++ optimization flags specific to the compiler and architecture with `-DCMAKE_CXX_FLAGS` and `-DCMAKE_C_FLAGS` options to the CMake command. For example:

```bash
cmake .. -DCMAKE_CXX_FLAGS="-O3 -g" \
         -DCMAKE_C_FLAGS="-O3 -g" \
         -DCMAKE_BUILD_TYPE=CUSTOM
```

By default OpenMP threading is enabled. You can disable it with `-DCORENRN_ENABLE_OPENMP=OFF`


## License
* See LICENSE.txt
* See [NEURON](https://www.neuron.yale.edu/neuron/)

## Contributors
See [contributors](https://github.com/BlueBrain/CoreNeuron/graphs/contributors).

## Funding

CoreNEURON is developed in a joint collaboration between the Blue Brain Project and Yale University. This work has been funded by the EPFL Blue Brain Project (funded by the Swiss ETH board), NIH grant number R01NS11613 (Yale University), the European Union Seventh Framework Program (FP7/20072013) under grant agreement n◦ 604102 (HBP) and the Eu- ropean Union’s Horizon 2020 Framework Programme for Research and Innovation under Grant Agreement n◦ 720270 (Human Brain Project SGA1) and Grant Agreement n◦ 785907 (Human Brain Project SGA2).
