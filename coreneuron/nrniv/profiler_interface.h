#pragma once

#include <initializer_list>
#include <type_traits>

#if defined(CORENEURON_CALIPER)
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

#if defined(LIKWID_PERFMON)
#include <likwid.h>
#endif

namespace coreneuron {

namespace detail {

template <class... TProfilerImpl>
struct Instrumentor {
    inline static void phase_begin(const char* name) {
        std::initializer_list<int>{(TProfilerImpl::phase_begin(name), 0)...};
    }
    inline static void phase_end(const char* name) {
        std::initializer_list<int>{(TProfilerImpl::phase_end(name), 0)...};
    }
    inline static void start_profile() {
        std::initializer_list<int>{(TProfilerImpl::start_profile(), 0)...};
    }
    inline static void stop_profile() {
        std::initializer_list<int>{(TProfilerImpl::stop_profile(), 0)...};
    }
    inline static void init_profile() {
        std::initializer_list<int>{(TProfilerImpl::init_profile(), 0)...};
    }
    inline static void finalize_profile() {
        std::initializer_list<int>{(TProfilerImpl::finalize_profile(), 0)...};
    }
};

#if defined(CORENEURON_CALIPER)

struct Caliper {
    inline static void phase_begin(const char* name) {
        CALI_MARK_BEGIN(name);
    };

    inline static void phase_end(const char* name) {
        CALI_MARK_END(name);
    };

    inline static void start_profile(){};

    inline static void stop_profile(){};

    inline static void init_profile(){};

    inline static void finalize_profile(){};
};

#endif

#if defined(CUDA_PROFILING)

struct CudaProfiling {
    inline static void phase_begin(const char* name){};

    inline static void phase_end(const char* name){};

    inline static void start_profile() {
        cudaProfilerStart();
    };

    inline static void stop_profile() {
        cudaProfilerStop();
    };

    inline static void init_profile(){};

    inline static void finalize_profile(){};
};

#endif

#if defined(CRAYPAT)

struct CrayPat {
    inline static void phase_begin(const char* name){};

    inline static void phase_end(const char* name){};

    inline static void start_profile() {
        PAT_record(PAT_STATE_ON);
    };

    inline static void stop_profile() {
        PAT_record(PAT_STATE_OFF);
    };

    inline static void init_profile(){};

    inline static void finalize_profile(){};
};
#endif

#if defined(TAU)

struct Tau {
    inline static void phase_begin(const char* name){};

    inline static void phase_end(const char* name){};

    inline static void start_profile() {
        TAU_ENABLE_INSTRUMENTATION();
    };

    inline static void stop_profile() {
        TAU_DISABLE_INSTRUMENTATION();
    };

    inline static void init_profile(){};

    inline static void finalize_profile(){};
};

#endif

#if defined(LIKWID_PERFMON)

struct Likwid {
    inline static void phase_begin(const char* name){
        LIKWID_MARKER_START(name);
    };

    inline static void phase_end(const char* name){
        LIKWID_MARKER_STOP(name);
    };

    inline static void start_profile() {};

    inline static void stop_profile() {};

    inline static void init_profile(){
        LIKWID_MARKER_INIT;

#pragma omp parallel
        {
            LIKWID_MARKER_THREADINIT;
        }

    };

    inline static void finalize_profile(){
        LIKWID_MARKER_CLOSE;
    };
};

#endif

struct NullInstrumentor {
    inline static void phase_begin(const char* name){};
    inline static void phase_end(const char* name){};
    inline static void start_profile(){};
    inline static void stop_profile(){};
    inline static void init_profile(){};
    inline static void finalize_profile(){};
};

}  // namespace detail

using Instrumentor = detail::Instrumentor<
#if defined CORENEURON_CALIPER
    detail::Caliper,
#endif
#if defined(CUDA_PROFILING)
    detail::CudaProfiling,
#endif
#if defined(CRAYPAT)
    detail::CrayPat,
#endif
#if defined(TAU)
    detail::Tau,
#endif
#if defined(LIKWID_PERFMON)
    detail::Likwid,
#endif
    detail::NullInstrumentor>;

}  // namespace coreneuron
