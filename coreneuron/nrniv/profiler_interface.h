#pragma once

#include <type_traits>

#if defined(CALIPER)
#include <caliper/cali.h>
#endif

#if defined(CUDA_PROFILING)
#include <cuda_profiler_api.h>
#endif

#if defined(CRAYPAT)
#include <pat_api.h>
#endif

#if defined(TAU)
#include <TAU.h>
#endif


namespace coreneuron {


struct NullInstrumentor_Impl {
    inline static void phase_begin(const char* name) {};
    inline static void phase_end(const char* name) {};
    inline static void start_profile() {};
    inline static void stop_profile() {};
};

#if defined(CALIPER)
struct Caliper_Impl : NullInstrumentor_Impl {
    inline static void phase_begin(const char* name) {
        CALI_MARK_BEGIN(name);
    };

    inline static void phase_end(const char* name) {
        CALI_MARK_END(name);
    };

    inline static void start_profile() {};

    inline static void stop_profile() {};
};

#define INSTRUMENTOR_T Caliper_Impl

#endif // CALIPER

#if defined(CUDA_PROFILING)

struct CudaProfiling_Impl : NullInstrumentor_Impl {
    inline static void phase_begin(const char* name) {
    };

    inline static void phase_end(const char* name) {
    };

    inline static void start_profile() {
	cudaProfilerStart();
    };

    inline static void stop_profile() {
	cudaProfilerStop();
    };
};

#define INSTRUMENTOR_T CudaProfiling_Impl

#endif // CUDA_PROFILING

#if defined(CRAYPAT)
struct CrayPat_Impl : NullInstrumentor_Impl {
    inline static void phase_begin(const char* name) {
    };

    inline static void phase_end(const char* name) {
    };

    inline static void start_profile() {
	PAT_record(PAT_STATE_ON);
    };

    inline static void stop_profile() {
	PAT_record(PAT_STATE_OFF);
    };
};

#define INSTRUMENTOR_T CrayPat_Impl

#endif // CRAYPAT

#if defined(TAU)
struct Tau_Impl : NullInstrumentor_Impl {
    inline static void phase_begin(const char* name) {
    };

    inline static void phase_end(const char* name) {
    };

    inline static void start_profile() {
	TAU_ENABLE_INSTRUMENTATION();
    };

    inline static void stop_profile() {
	TAU_DISABLE_INSTRUMENTATION();
    };
};

#define INSTRUMENTOR_T Tau_Impl

#endif // TAU


template<typename TProfilerImpl>
struct Instrumentor {
    static_assert(std::is_base_of<NullInstrumentor_Impl, TProfilerImpl>::value,
        "Not a valid instrumentor implementation");

    static void phase_begin(const char* name) {
        TProfilerImpl::phase_begin(name);
    }
    static void phase_end(const char* name) {
        TProfilerImpl::phase_end(name);
    }
    static void start_profile() {
        TProfilerImpl::start_profile();

    }
    static void stop_profile() {
        TProfilerImpl::stop_profile();

    }

};


}  // namespace coreneuron
