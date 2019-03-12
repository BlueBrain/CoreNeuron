#pragma once

#if defined(CALIPER)
#include <caliper/cali.h>
#define INSTRUMENTOR_T ProfilerType::caliper

#elif defined(CUDA_PROFILING)
#include <cuda_profiler_api.h>
#define INSTRUMENTOR_T ProfilerType::cuda

#elif defined(CRAYPAT)
#include <pat_api.h>
#define INSTRUMENTOR_T ProfilerType::craypat

#elif defined(TAU)
#include <TAU.h>
#define INSTRUMENTOR_T ProfilerType::tau

#else
#define INSTRUMENTOR_T ProfilerType::none

#endif


namespace coreneuron {

enum class ProfilerType {NONE, caliper, cuda, craypat, tau};

template<ProfilerType type>
struct Instrumentor {
    static void phase_begin(const char*);
    static void phase_end(const char*);
    static void start_profile();
    static void stop_profile();
};

template<ProfilerType type>
void Instrumentor<type>::phase_begin(const char*) {};

template<ProfilerType type>
void Instrumentor<type>::phase_end(const char*) {};

template<ProfilerType type>
void Instrumentor<type>::start_profile() {};

template<ProfilerType type>
void Instrumentor<type>::stop_profile() {};


#if defined(CALIPER)
template<>
inline void Instrumentor<ProfilerType::caliper>::phase_begin(const char* name) {
	CALI_MARK_BEGIN(name);
};

template<>
inline void Instrumentor<ProfilerType::caliper>::phase_end(const char* name) {
	CALI_MARK_END(name);
};
#endif


#if defined(CUDA_PROFILING)
template<>
inline void Instrumentor<ProfilerType::cuda>::start_profile() {
	cudaProfilerStart();
};

template<>
inline void Instrumentor<ProfilerType::cuda>::stop_profile() {
	cudaProfilerStop();
};
#endif


#if defined(CRAYPAT)
template<>
inline void Instrumentor<ProfilerType::craypat>::start_profile() {
	PAT_record(PAT_STATE_ON);
};

template<>
inline void Instrumentor<ProfilerType::craypat>::stop_profile() {
	PAT_record(PAT_STATE_OFF);
};
#endif


#if defined(TAU)
template<>
inline void Instrumentor<ProfilerType::tau>::start_profile() {
	TAU_ENABLE_INSTRUMENTATION();
};

template<>
inline void Instrumentor<ProfilerType::tau>::stop_profile() {
	TAU_DISABLE_INSTRUMENTATION();
};
#endif

}  // namespace coreneuron
