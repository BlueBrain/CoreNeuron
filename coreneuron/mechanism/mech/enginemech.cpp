/// Corenrnmech is a wrapper lib providing a single solve_core function
/// which initializes the solver, loads the external mechanisms and launches the simulation

#include <coreneuron/enginemech.h>
#include <coreneuron/engine.h>

#ifdef ADDITIONAL_MECHS
namespace coreneuron {
extern void modl_reg();
}
#endif

int solve_core(int argc, char** argv) {
    if (mk_mech_init(argc, argv) == 1) { // If this return 1, it means it has only receive "--version", so finish
        return 0;
    }

#ifdef ADDITIONAL_MECHS
    /// Initializing additional Neurodamus mechanisms (in mod_func.c, built by mech/mod_func.c.pl)
    coreneuron::modl_reg();
#endif

    return run_solve_core(argc, argv);
}
