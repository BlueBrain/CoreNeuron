# Generating Tests Input Dataset

There two integration tests under `tests/integration/` directory. The input dataset is generated using NEURON. You can follow below steps for test data generation.

Once you have latest NEURON installed, you have to clone [ringtest](https://github.com/nrnhines/ringtest) model from github:

```bash
git clone https://github.com/nrnhines/ringtest.git
```

You have to create `special` as usual with NEURON:

```bash
nrnivmodl mod
```

Now we can generate data for `ring` test as:


```bash
mpirun -n 2 ./x86_64/special ringtest.py -nring 1 -ncell 20 -tstop 100 -mpi

# sort spikes and remove old spike output
sortspike coredat/spk2.std coredat/out.dat.ref
rm coredat/spk2.std
```

The generated dataset can be copied to `tests/integration/ring/`:

```bash
mv coredat/* coreneuron/tests/integration/ring/
```


Similarly, dataset for `ring_gap` test can be generated as:

```bash
mpirun -n 2 ./x86_64/special ringtest.py -nring 1 -ncell 20 -tstop 100 -gap -mpi

# sort spikes and remove old spike output
sortspike coredat/spk2.std coredat/out.dat.ref
rm coredat/spk2.std
mv coredat/* coreneuron/tests/integration/ring_gap/
```
