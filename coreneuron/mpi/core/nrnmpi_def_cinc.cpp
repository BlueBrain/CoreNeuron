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

int nrnmpi_send_nspike_;
unsigned char* nrnmpi_spikein_fixed_ovfl_;
}  // namespace coreneuron
