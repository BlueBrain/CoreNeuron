/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#include "coreneuron/mechanism/mech/cfile/scoplib.h"
#include "coreneuron/mechanism/mech/mod2c_core_thread.hpp"
#include "coreneuron/sim/scopmath/errcodes.h"
#include "coreneuron/utils/offload.hpp"

namespace coreneuron {
void _modl_set_dt_thread(double dt, NrnThread* nt) {
    nt->_dt = dt;
}
double _modl_get_dt_thread(NrnThread* nt) {
    return nt->_dt;
}
}  // namespace coreneuron
