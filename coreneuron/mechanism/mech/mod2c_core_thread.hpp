/*
# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#pragma once

#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/mechanism/mechanism.hpp"
#include "coreneuron/utils/offload.hpp"

namespace coreneuron {

#define _STRIDE _cntml_padded + _iml

#define _threadargscomma_ _iml, _cntml_padded, _p, _ppvar, _thread, _nt, _v,
#define _threadargsprotocomma_                                                                    \
    int _iml, int _cntml_padded, double *_p, Datum *_ppvar, ThreadDatum *_thread, NrnThread *_nt, \
        double _v,
#define _threadargs_ _iml, _cntml_padded, _p, _ppvar, _thread, _nt, _v
#define _threadargsproto_                                                                         \
    int _iml, int _cntml_padded, double *_p, Datum *_ppvar, ThreadDatum *_thread, NrnThread *_nt, \
        double _v

struct Elm {
    unsigned row;        /* Row location */
    unsigned col;        /* Column location */
    double* value;       /* The value SOA  _cntml_padded of them*/
    struct Elm* r_up;    /* Link to element in same column */
    struct Elm* r_down;  /*       in solution order */
    struct Elm* c_left;  /* Link to left element in same row */
    struct Elm* c_right; /*       in solution order (see getelm) */
};
#define ELM0 (Elm*) 0

struct Item {
    Elm* elm;
    unsigned norder; /* order of a row */
    struct Item* next;
    struct Item* prev;
};
#define ITEM0 (Item*) 0

using List = Item; /* list of mixed items */

struct SparseObj {            /* all the state information */
    Elm** rowst{};            /* link to first element in row (solution order)*/
    Elm** diag{};             /* link to pivot element in row (solution order)*/
    void* elmpool{};          /* no interthread cache line sharing for elements */
    unsigned neqn{};          /* number of equations */
    unsigned _cntml_padded{}; /* number of instances */
    unsigned* varord{};       /* row and column order for pivots */
    double* rhs{};            /* initially- right hand side        finally - answer */
    unsigned* ngetcall{};     /* per instance counter for number of calls to _getelm */
    int phase{};              /* 0-solution phase; 1-count phase; 2-build list phase */
    int numop{};
    unsigned coef_list_size{};
    double** coef_list{}; /* pointer to (first instance) value in _getelm order */
    /* don't really need the rest */
    int nroworder{};   /* just for freeing */
    Item** roworder{}; /* roworder[i] is pointer to order item for row i.
                             Does not have to be in orderlist */
    List* orderlist{}; /* list of rows sorted by norder
                             that haven't been used */
    int do_flag{};
};

nrn_pragma_acc(routine seq)
nrn_pragma_omp(declare target)
extern double* _nrn_thread_getelm(SparseObj* so, int row, int col, int _iml);
nrn_pragma_omp(end declare target)

extern void _nrn_destroy_sparseobj_thread(SparseObj* so);

// derived from nrn/src/scopmath/euler.c
// updated for aos/soa layout index
template <typename F>
int euler_thread(int neqn, int* var, int* der, F fun, _threadargsproto_) {
    /* calculate the derivatives */
    fun(_threadargs_);
    /* update dependent variables */
    double const dt{_nt->_dt};
    for (int i = 0; i < neqn; i++) {
        _p[var[i] * _STRIDE] += dt * (_p[der[i] * _STRIDE]);
    }
    return 0;
}

void nrn_sparseobj_copyto_device(SparseObj* so);
void nrn_sparseobj_delete_from_device(SparseObj* so);

}  // namespace coreneuron
