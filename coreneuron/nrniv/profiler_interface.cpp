#include <stdio.h>
#include "coreneuron/nrnmpi/nrnmpi.h"
#include "coreneuron/nrniv/profiler_interface.h"

#ifdef CUDA_PROFILING
#include "coreneuron/nrniv/cuda_profile.h"
#endif

#ifdef CRAYPAT
#include <pat_api.h>
#endif

#ifdef TAU
#include <TAU.h>
#endif

#ifdef _OPENACC
#include <openacc.h>
#endif

#ifdef CALIPER
#include <caliper/cali.h>
#endif


namespace coreneuron {
static int cray_acc_debug_orig = 0;
static int cray_acc_debug_zero = 0;

void start_profile() {
#ifdef CRAYPAT
    PAT_record(PAT_STATE_ON);
#endif

#if defined(_OPENACC) && defined(_CRAYC)
    cray_acc_set_debug_global_level(cray_acc_debug_orig);
#endif

#ifdef CUDA_PROFILING
    start_cuda_profile();
#endif

#ifdef TAU
    TAU_ENABLE_INSTRUMENTATION();
#endif
}

void stop_profile() {
#ifdef CRAYPAT
    PAT_record(PAT_STATE_OFF);
#endif

#if defined(_OPENACC) && defined(_CRAYC)
    cray_acc_debug_orig = cray_acc_get_debug_global_level();
    cray_acc_set_debug_global_level(cray_acc_debug_zero);
#endif

#ifdef CUDA_PROFILING
    stop_cuda_profile();
#endif

#ifdef TAU
    TAU_DISABLE_INSTRUMENTATION();
#endif
}


#ifdef CALIPER

template<>
void Instrumentor::phase_begin<InstrumentorType::Caliper>(const char* name) {
    CALI_MARK_BEGIN(name);
}

template<>
void Instrumentor::phase_end<InstrumentorType::Caliper>(const char* name) {
    CALI_MARK_END(name);
}

#else

template<>
void Instrumentor::phase_begin<InstrumentorType::Empty>(const char*) {
}

template<>
void Instrumentor::phase_end<InstrumentorType::Empty>(const char*) {
}

#endif

}  // namespace coreneuron
