//
// Created by magkanar on 9/12/19.
//

#ifndef fast_imem_h
#define fast_imem_h

#include "coreneuron/nrnconf.h"
#include "coreneuron/nrniv/memory.h"
#include "coreneuron/nrnmpi/nrnmpi.h"
#include "coreneuron/nrnoc/multicore.h"

namespace coreneuron {

/* Enables fast membrane curent culculation and allocates required
 * memory.
 * Found in src/nrncvode/cvodeobj.cpp in NEURON.
 */
void use_fast_imem();

/* Free memory allocated for the fast current membrane calculation.
 * Found in src/nrnoc/multicore.c in NEURON.
 */
static void fast_imem_free();

/* Allocate memory for the rhs and d arrays needed for the fast
 * current membrane calculation.
 * Found in src/nrnoc/multicore.c in NEURON.
 */
static void fast_imem_alloc();

/* fast_imem_alloc() wrapper.
 * Found in src/nrnoc/multicore.c in NEURON.
 */
void nrn_fast_imem_alloc();

/* Calculate the new values of rhs array at every timestep.
 * Found in src/nrnoc/fadvance.c in NEURON.
 */
void nrn_calc_fast_imem(NrnThread* _nt);

}  // namespace coreneuron
#endif //fast_imem_h
