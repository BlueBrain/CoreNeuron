/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
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
#include <iostream>

namespace coreneuron {
template <typename T>
#define cnrn_target_deviceptr(h_ptr) [&]() {std::cout << #h_ptr << std::endl; return cnrn_target_deviceptr2(h_ptr);}()
T* cnrn_target_deviceptr2(const T* h_ptr) {
    std::cout << "cnrn_target_device_ptr" << std::endl;
#if defined(CORENEURON_ENABLE_GPU) && !defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && defined(_OPENACC)
    return static_cast<T*>(acc_deviceptr(const_cast<T*>(h_ptr)));
#elif defined(CORENEURON_ENABLE_GPU) && defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && defined(_OPENMP)
    T *d_ptr = nullptr;
    T *_h_ptr = const_cast<T*>(h_ptr);

    nrn_pragma_omp(target data use_device_ptr(_h_ptr))
    {
        d_ptr = _h_ptr;
    }

    return d_ptr;
#else
    throw std::runtime_error("cnrn_target_deviceptr() not implemented without OpenACC/OpenMP and gpu build");
#endif
}

template <typename T>
T* cnrn_target_copyin(const T* h_ptr, std::size_t len = 1) {
    std::cout << "cnrn_target_copyin" << std::endl;
#if defined(CORENEURON_ENABLE_GPU) && !defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && defined(_OPENACC)
    return static_cast<T*>(acc_copyin(const_cast<T*>(h_ptr), len * sizeof(T)));
#elif defined(CORENEURON_ENABLE_GPU) && defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && defined(_OPENMP)
    #pragma omp target enter data map(to:h_ptr[:len])
    return cnrn_target_deviceptr(h_ptr);
#else
    throw std::runtime_error("cnrn_target_copyin() not implemented without OpenACC/OpenMP and gpu build");
#endif
}

template <typename T>
void cnrn_target_delete(T* h_ptr, std::size_t len = 1) {
    std::cout << "cnrn_target_delete" << std::endl;
#if defined(CORENEURON_ENABLE_GPU) && !defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && defined(_OPENACC)
    acc_delete(h_ptr, len * sizeof(T));
#elif defined(CORENEURON_ENABLE_GPU) && defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && defined(_OPENMP)
    #pragma omp target exit data map(delete: h_ptr[:len])
#else
    throw std::runtime_error("cnrn_target_delete() not implemented without OpenACC/OpenMP and gpu build");
#endif
}

template <typename T>
void cnrn_target_memcpy_to_device(T* d_ptr, const T* h_ptr, std::size_t len = 1) {
    std::cout << "cnrn_target_memcpy_to_device" << std::endl;
#if defined(CORENEURON_ENABLE_GPU) && !defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && defined(_OPENACC)
    acc_memcpy_to_device(d_ptr, const_cast<T*>(h_ptr), len * sizeof(T));
#elif defined(CORENEURON_ENABLE_GPU) && defined(CORENEURON_PREFER_OPENMP_OFFLOAD) && defined(_OPENMP)
    omp_target_memcpy(d_ptr, const_cast<T*>(h_ptr), len* sizeof(T), 0, 0, omp_get_default_device(), omp_get_initial_device());
#else
    throw std::runtime_error("cnrn_target_memcpy_to_device() not implemented without OpenACC/OpenMP and gpu build");
#endif
}

}
