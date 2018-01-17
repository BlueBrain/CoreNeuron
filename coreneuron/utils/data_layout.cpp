#include "coreneuron/utils/data_layout.hpp"
#include "coreneuron/nrnoc/nrnoc_ml.h"                    // for Memb_list definition
#include "coreneuron/nrniv/node_permute.h"                // for nrn_index_permute
#include "coreneuron/nrnoc/nrnoc_decl.h"                  // for nrn_i_layout
#include "coreneuron/nrnoc/membfunc.h"                    // for nrn_mech_data_layout_

/*
 * Original input files are organized in AoS
 * TODO: check permute
 */
int getDataIndex(int node_index, int variable_index, int mtype, Memb_list* ml) {
  int layout = nrn_mech_data_layout_[mtype];
  if (layout == AOS_LAYOUT) {
    return variable_index + node_index* ml->_nodecount_padded;
  }
  assert(layout == SOA_LAYOUT);
  return variable_index* ml->_nodecount_padded + node_index;
}
