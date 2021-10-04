/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#ifndef nrnmpispike_h
#define nrnmpispike_h

#if NRNMPI

#ifndef nrn_spikebuf_size
#define nrn_spikebuf_size 0
#endif

namespace coreneuron {

#if nrn_spikebuf_size > 0
struct NRNMPI_Spikebuf {
    int nspike;
    int gid[nrn_spikebuf_size];
    double spiketime[nrn_spikebuf_size];
};
#endif

#define nout_ nrnmpi_nout_
extern int nout_;

#define spfixout_     nrnmpi_spikeout_fixed_
#define spfixin_      nrnmpi_spikein_fixed_
#define ag_send_size_ nrnmpi_ag_send_size_
#define ovfl_         nrnmpi_ovfl_
extern int ag_send_size_; /* bytes */
extern int ovfl_;         /* spikes */
extern unsigned char* spfixout_;
extern unsigned char* spfixin_;

#if nrn_spikebuf_size > 0
#define spbufout_ nrnmpi_spbufout_
#define spbufin_  nrnmpi_spbufin_
extern NRNMPI_Spikebuf* spbufout_;
extern NRNMPI_Spikebuf* spbufin_;
#endif

}  // namespace coreneuron

#endif  // NRNMPI
#endif
