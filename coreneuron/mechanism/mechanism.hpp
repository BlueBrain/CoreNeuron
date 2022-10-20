/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#pragma once

#include <string.h>

#include "coreneuron/nrnconf.h"
#include "coreneuron/utils/memory.h"

namespace coreneuron {
// OpenACC with PGI compiler has issue when union is used and hence use struct
// \todo check if newer PGI versions has resolved this issue
#if defined(_OPENACC)
struct ThreadDatum {
    int i;
    double* pval;
    void* _pvoid;
};
#else
union ThreadDatum {
    double val;
    int i;
    double* pval;
    void* _pvoid;
};
#endif

/* will go away at some point */
struct Point_process {
    int _i_instance;
    short _type;
    short _tid; /* NrnThread id */
};

struct NetReceiveBuffer_t: UnifiedMemManaged<false> {
    unified_uniq_ptr<int[]> _displ;     /* _displ_cnt + 1 of these */
    unified_uniq_ptr<int[]> _nrb_index; /* _cnt of these (order of increasing _pnt_index) */

    unified_uniq_ptr<int[]> _pnt_index;
    unified_uniq_ptr<int[]> _weight_index;
    unified_uniq_ptr<double[]> _nrb_t;
    unified_uniq_ptr<double[]> _nrb_flag;
    int _cnt = 0;
    int _displ_cnt = 0; /* number of unique _pnt_index */

    std::size_t _size = 0;      /* capacity */
    int _pnt_offset = 0;
    std::size_t size_of_object() {
        std::size_t nbytes = 0;
        nbytes += _size * sizeof(int) * 3;
        nbytes += (_size + 1) * sizeof(int);
        nbytes += _size * sizeof(double) * 2;
        return nbytes;
    }

    void initialize(std::size_t size) {
        _size = size;
        _pnt_index = allocate_unique<int[]>(allocator<int>{}, _size);
        auto displ_size = _size + 1;
        _displ = allocate_unique<int[]>(allocator<int>{}, displ_size);
        _nrb_index = allocate_unique<int[]>(allocator<int>{}, _size);
        _weight_index = allocate_unique<int[]>(allocator<int>{}, _size);
        _nrb_t = allocate_unique<double[]>(allocator<double>{}, _size);
        _nrb_flag = allocate_unique<double[]>(allocator<double>{}, _size);
    }

    void grow() {
        std::size_t new_size = _size * 2;
        grow_buf(_pnt_index, _size, new_size);
        grow_buf(_weight_index, _size, new_size);
        grow_buf(_nrb_t, _size, new_size);
        grow_buf(_nrb_flag, _size, new_size);
        grow_buf(_displ,  _size + 1, new_size + 1);
        grow_buf(_nrb_index, _size, new_size);
        _size = new_size;
    }
};

struct NetSendBuffer_t: UnifiedMemManaged<false> {
    unified_uniq_ptr<int[]> _sendtype;  // net_send, net_event, net_move
    unified_uniq_ptr<int[]> _vdata_index;
    unified_uniq_ptr<int[]> _pnt_index;
    unified_uniq_ptr<int[]> _weight_index;
    unified_uniq_ptr<double[]> _nsb_t;
    unified_uniq_ptr<double[]> _nsb_flag;
    int _cnt = 0;
    std::size_t _size = 0;       /* capacity */
    int reallocated = 0; /* if buffer resized/reallocated, needs to be copy to cpu */

    NetSendBuffer_t(int size)
        : _size(size) {
        _cnt = 0;

        _sendtype = allocate_unique<int[]>(allocator<int>{}, _size);
        _vdata_index = allocate_unique<int[]>(allocator<int>{}, _size);
        _pnt_index = allocate_unique<int[]>(allocator<int>{}, _size);
        _weight_index = allocate_unique<int[]>(allocator<int>{}, _size);
        // when == 1, NetReceiveBuffer_t is newly allocated (i.e. we need to free previous copy
        // and recopy new data
        reallocated = 1;
        _nsb_t = allocate_unique<double[]>(allocator<double>{}, _size);
        _nsb_flag = allocate_unique<double[]>(allocator<double>{}, _size);
    }

    size_t size_of_object() {
        size_t nbytes = 0;
        nbytes += _size * sizeof(int) * 4;
        nbytes += _size * sizeof(double) * 2;
        return nbytes;
    }

    void grow() {
        std::size_t new_size = _size * 2;
        grow_buf(_sendtype, _size, new_size);
        grow_buf(_vdata_index, _size, new_size);
        grow_buf(_pnt_index, _size, new_size);
        grow_buf(_weight_index, _size, new_size);
        grow_buf(_nsb_t, _size, new_size);
        grow_buf(_nsb_flag, _size, new_size);
        _size = new_size;
    }
};

struct Memb_list {
    /* nodeindices contains all nodes this extension is responsible for,
     * ordered according to the matrix. This allows to access the matrix
     * directly via the nrn_actual_* arrays instead of accessing it in the
     * order of insertion and via the node-structure, making it more
     * cache-efficient */
    int* nodeindices = nullptr;
    int* _permute = nullptr;
    double* data = nullptr;
    Datum* pdata = nullptr;
    ThreadDatum* _thread = nullptr; /* thread specific data (when static is no good) */
    NetReceiveBuffer_t* _net_receive_buffer = nullptr;
    NetSendBuffer_t* _net_send_buffer = nullptr;
    int nodecount; /* actual node count */
    int _nodecount_padded;
    void* instance{nullptr}; /* mechanism instance struct */
    // nrn_acc_manager.cpp handles data movement to/from the accelerator as the
    // "private constructor" in the translated MOD file code is called before
    // the main nrn_acc_manager methods that copy thread/mechanism data to the
    // device
    void* global_variables{nullptr};
    std::size_t global_variables_size{};
};
}  // namespace coreneuron
