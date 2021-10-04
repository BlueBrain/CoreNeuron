/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

namespace coreneuron {
int nrnmpi_numprocs = 1; /* size */
int nrnmpi_myid = 0;     /* rank */

int nrnmpi_nout_;
int nrnmpi_i_capacity_;

#if NRNMPI
NRNMPI_Spike* nrnmpi_spikeout_;
NRNMPI_Spike* nrnmpi_spikein_;
#endif

int nrnmpi_ag_send_size_;
int nrnmpi_send_nspike_;
int nrnmpi_ovfl_capacity_;
int nrnmpi_ovfl_;
unsigned char* nrnmpi_spikeout_fixed_;
unsigned char* nrnmpi_spikein_fixed_;
unsigned char* nrnmpi_spikein_fixed_ovfl_;
}  // namespace coreneuron
