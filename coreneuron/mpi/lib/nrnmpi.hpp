// This file contains functions that does not go outside of the mpi library
namespace coreneuron {
extern int nrnmpi_numprocs;
extern int nrnmpi_myid;
void nrnmpi_spike_initialize();
}
