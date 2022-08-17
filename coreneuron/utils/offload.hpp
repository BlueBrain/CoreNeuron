/*
# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/
#pragma once
#define nrn_pragma_stringify(x) #x
#if defined(CORENEURON_ENABLE_GPU) && defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && defined(_OPENMP)
#define nrn_pragma_acc(x)
#define nrn_pragma_omp(x) _Pragma(nrn_pragma_stringify(omp x))
#include <omp.h>
#elif defined(CORENEURON_ENABLE_GPU) && !defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && \
    defined(_OPENACC)
#define nrn_pragma_acc(x) _Pragma(nrn_pragma_stringify(acc x))
#define nrn_pragma_omp(x)
#include <openacc.h>
#else
#define nrn_pragma_acc(x)
#define nrn_pragma_omp(x)
#include <stdexcept>
#endif

#include <cstddef>
#include <string_view>

namespace coreneuron {
void cnrn_target_copyin_debug(std::string_view file,
                              int line,
                              std::size_t sizeof_T,
                              std::type_info const& typeid_T,
                              void const* h_ptr,
                              std::size_t len,
                              void* d_ptr);
void cnrn_target_delete_debug(std::string_view file,
                              int line,
                              std::size_t sizeof_T,
                              std::type_info const& typeid_T,
                              void const* h_ptr,
                              std::size_t len);
void cnrn_target_deviceptr_debug(std::string_view file,
                                 int line,
                                 std::size_t sizeof_T,
                                 std::type_info const& typeid_T,
                                 void const* h_ptr,
                                 void* d_ptr);
void cnrn_target_memcpy_to_device_debug(std::string_view file,
                                        int line,
                                        std::size_t sizeof_T,
                                        std::type_info const& typeid_T,
                                        void const* h_ptr,
                                        std::size_t len,
                                        void* d_ptr);
#if defined(CORENEURON_ENABLE_GPU) && !defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && \
    defined(_OPENACC) && !defined(CORENEURON_UNIFIED_MEMORY)
// Homegrown implementation for buggy NVHPC versions (<=22.3?)
#define CORENEURON_ENABLE_PRESENT_TABLE
void* cnrn_target_deviceptr_impl(void const* h_ptr);
void cnrn_target_copyin_update_present_table(void const* h_ptr, void* d_ptr, std::size_t len);
void cnrn_target_delete_update_present_table(void const* h_ptr, std::size_t len);
#endif

template <typename T>
T* cnrn_target_deviceptr(std::string_view file, int line, const T* h_ptr) {
    T* d_ptr{};
#ifdef CORENEURON_ENABLE_PRESENT_TABLE
    d_ptr = static_cast<T*>(cnrn_target_deviceptr_impl(h_ptr));
#elif defined(CORENEURON_ENABLE_GPU) && !defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && \
    defined(_OPENACC)
    d_ptr = static_cast<T*>(acc_deviceptr(const_cast<T*>(h_ptr)));
#elif defined(CORENEURON_ENABLE_GPU) && defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && \
    defined(_OPENMP)
    nrn_pragma_omp(target data use_device_ptr(h_ptr))
    { d_ptr = const_cast<T*>(h_ptr); }
#else
    throw std::runtime_error(
        "cnrn_target_deviceptr() not implemented without OpenACC/OpenMP and gpu build");
#endif
    cnrn_target_deviceptr_debug(file, line, sizeof(T), typeid(T), h_ptr, d_ptr);
    return d_ptr;
}

template <typename T>
T* cnrn_target_copyin(std::string_view file, int line, const T* h_ptr, std::size_t len = 1) {
    T* d_ptr{};
#if defined(CORENEURON_ENABLE_GPU) && !defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && \
    defined(_OPENACC)
    d_ptr = static_cast<T*>(acc_copyin(const_cast<T*>(h_ptr), len * sizeof(T)));
#elif defined(CORENEURON_ENABLE_GPU) && defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && \
    defined(_OPENMP)
    nrn_pragma_omp(target enter data map(to : h_ptr[:len]))
    nrn_pragma_omp(target data use_device_ptr(h_ptr))
    { d_ptr = const_cast<T*>(h_ptr); }
#else
    throw std::runtime_error(
        "cnrn_target_copyin() not implemented without OpenACC/OpenMP and gpu build");
#endif
#ifdef CORENEURON_ENABLE_PRESENT_TABLE
    cnrn_target_copyin_update_present_table(h_ptr, d_ptr, len * sizeof(T));
#endif
    cnrn_target_copyin_debug(file, line, sizeof(T), typeid(T), h_ptr, len, d_ptr);
    return d_ptr;
}

template <typename T>
void cnrn_target_delete(std::string_view file, int line, T* h_ptr, std::size_t len = 1) {
    cnrn_target_delete_debug(file, line, sizeof(T), typeid(T), h_ptr, len);
#ifdef CORENEURON_ENABLE_PRESENT_TABLE
    cnrn_target_delete_update_present_table(h_ptr, len * sizeof(T));
#endif
#if defined(CORENEURON_ENABLE_GPU) && !defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && \
    defined(_OPENACC)
    acc_delete(h_ptr, len * sizeof(T));
#elif defined(CORENEURON_ENABLE_GPU) && defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && \
    defined(_OPENMP)
    nrn_pragma_omp(target exit data map(delete : h_ptr[:len]))
#else
    throw std::runtime_error(
        "cnrn_target_delete() not implemented without OpenACC/OpenMP and gpu build");
#endif
}

template <typename T>
void cnrn_target_memcpy_to_device(std::string_view file,
                                  int line,
                                  T* d_ptr,
                                  const T* h_ptr,
                                  std::size_t len = 1) {
    cnrn_target_memcpy_to_device_debug(file, line, sizeof(T), typeid(T), h_ptr, len, d_ptr);
#if defined(CORENEURON_ENABLE_GPU) && !defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && \
    defined(_OPENACC)
    acc_memcpy_to_device(d_ptr, const_cast<T*>(h_ptr), len * sizeof(T));
#elif defined(CORENEURON_ENABLE_GPU) && defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && \
    defined(_OPENMP)
    omp_target_memcpy(d_ptr,
                      const_cast<T*>(h_ptr),
                      len * sizeof(T),
                      0,
                      0,
                      omp_get_default_device(),
                      omp_get_initial_device());
#else
    throw std::runtime_error(
        "cnrn_target_memcpy_to_device() not implemented without OpenACC/OpenMP and gpu build");
#endif
}

// Replace with std::source_location once we have C++20
#define cnrn_target_copyin(...)    cnrn_target_copyin(__FILE__, __LINE__, __VA_ARGS__)
#define cnrn_target_delete(...)    cnrn_target_delete(__FILE__, __LINE__, __VA_ARGS__)
#define cnrn_target_deviceptr(...) cnrn_target_deviceptr(__FILE__, __LINE__, __VA_ARGS__)
#define cnrn_target_memcpy_to_device(...) \
    cnrn_target_memcpy_to_device(__FILE__, __LINE__, __VA_ARGS__)

}  // namespace coreneuron
