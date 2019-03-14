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

namespace detail {

template<typename TProfilerImpl>
struct Instrumentor {
    Instrumentor();

    inline static void phase_begin(const char* name) {
        TProfilerImpl::phase_begin(name);
    }
    inline static void phase_end(const char* name) {
        TProfilerImpl::phase_end(name);
    }
    inline static void start_profile() {
        TProfilerImpl::start_profile();
    }
    inline static void stop_profile() {
        TProfilerImpl::stop_profile();
    }

};



#if defined(CALIPER)
struct Caliper : Instrumentor<Caliper> {
    inline static void phase_begin(const char* name) {
        CALI_MARK_BEGIN(name);
    };

    inline static void phase_end(const char* name) {
        CALI_MARK_END(name);
    };

    inline static void start_profile() {};

    inline static void stop_profile() {};
};

using InstrumentorImpl = Caliper;


#elif defined(CUDA_PROFILING)

struct CudaProfiling : Instrumentor<CudaProfiling> {
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

using InstrumentorImpl = CudaProfiling;


#elif defined(CRAYPAT)
struct CrayPat : Instrumentor<CrayPat> {
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

using InstrumentorImpl = CrayPat;


#elif defined(TAU)
struct Tau : Instrumentor<Tau> {
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

using InstrumentorImpl = Tau;

#else

struct NullInstrumentor : Instrumentor<NullInstrumentor> {
    inline static void phase_begin(const char* name) {};
    inline static void phase_end(const char* name) {};
    inline static void start_profile() {};
    inline static void stop_profile() {};
};
using InstrumentorImpl = NullInstrumentor;
#endif

template<typename TProfilerImpl>
inline Instrumentor<TProfilerImpl>::Instrumentor() {
    static_assert(std::is_base_of<Instrumentor<TProfilerImpl>, TProfilerImpl>::value,
                  "Not a valid instrumentor implementation");
}

} // namespace detail

using Instrumentor = detail::Instrumentor<detail::InstrumentorImpl>;



}  // namespace coreneuron
