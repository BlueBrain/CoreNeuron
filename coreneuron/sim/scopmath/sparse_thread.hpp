/*
# =============================================================================
# Originally sparse.c from SCoP library, Copyright (c) 1989-90 Duke University
# =============================================================================
*/
#pragma once
#include "coreneuron/mechanism/mech/mod2c_core_thread.hpp"
#include "coreneuron/sim/scopmath/errcodes.h"

namespace coreneuron {
namespace scopmath {
namespace sparse {
// Methods that are not templates and are not called from offloaded regions.
void initeqn(SparseObj* so, unsigned maxeqn);
void spar_minorder(SparseObj* so);

// Methods that may be called from offloaded regions are declared inline.
inline void init_coef_list(SparseObj* so, int _iml) {
    so->ngetcall[_iml] = 0;
    for (unsigned i = 1; i <= so->neqn; i++) {
        for (Elm* el = so->rowst[i]; el; el = el->c_right) {
            el->value[_iml] = 0.;
        }
    }
}

#define coreneuron_scopmath_sparse_ix(arg) ((arg) *_STRIDE)
inline void subrow(SparseObj* so, Elm* pivot, Elm* rowsub, int _iml) {
    int _cntml_padded = so->_cntml_padded;
    double r = rowsub->value[_iml] / pivot->value[_iml];
    so->rhs[coreneuron_scopmath_sparse_ix(rowsub->row)] -=
        so->rhs[coreneuron_scopmath_sparse_ix(pivot->row)] * r;
    so->numop++;
    for (auto el = pivot->c_right; el; el = el->c_right) {
        for (rowsub = rowsub->c_right; rowsub->col != el->col; rowsub = rowsub->c_right) {
            ;
        }
        rowsub->value[_iml] -= el->value[_iml] * r;
        so->numop++;
    }
}

inline void bksub(SparseObj* so, int _iml) {
    int _cntml_padded = so->_cntml_padded;
    for (unsigned i = so->neqn; i >= 1; i--) {
        for (Elm* el = so->diag[i]->c_right; el; el = el->c_right) {
            so->rhs[coreneuron_scopmath_sparse_ix(el->row)] -=
                el->value[_iml] * so->rhs[coreneuron_scopmath_sparse_ix(el->col)];
            so->numop++;
        }
        so->rhs[coreneuron_scopmath_sparse_ix(so->diag[i]->row)] /= so->diag[i]->value[_iml];
        so->numop++;
    }
}

inline int matsol(SparseObj* so, int _iml) {
    /* Upper triangularization */
    so->numop = 0;
    for (unsigned i = 1; i <= so->neqn; i++) {
        Elm* pivot;
        if (fabs((pivot = so->diag[i])->value[_iml]) <= ROUNDOFF) {
            return SINGULAR;
        }
        /* Eliminate all elements in pivot column */
        for (auto el = pivot->r_down; el; el = el->r_down) {
            subrow(so, pivot, el, _iml);
        }
    }
    bksub(so, _iml);
    return (SUCCESS);
}

template <typename SPFUN>
void create_coef_list(SparseObj* so, int n, SPFUN fun, _threadargsproto_) {
    initeqn(so, (unsigned) n);
    so->phase = 1;
    so->ngetcall[0] = 0;
    fun(so, so->rhs, _threadargs_);  // std::invoke in C++17
    if (so->coef_list) {
        free(so->coef_list);
    }
    so->coef_list_size = so->ngetcall[0];
    so->coef_list = new double*[so->coef_list_size];
    spar_minorder(so);
    so->phase = 2;
    so->ngetcall[0] = 0;
    fun(so, so->rhs, _threadargs_);  // std::invoke in C++17
    so->phase = 0;
}
}  // namespace sparse
}  // namespace scopmath

#define coreneuron_scopmath_sparse_s(arg) _p[coreneuron_scopmath_sparse_ix(s[arg])]
#define coreneuron_scopmath_sparse_d(arg) _p[coreneuron_scopmath_sparse_ix(d[arg])]

/**
 * sparse matrix dynamic allocation: create_coef_list makes a list for fast
 * setup, does minimum ordering and ensures all elements needed are present.
 * This could easily be made recursive but it isn't right now.
 */
template <typename SPFUN>
void* nrn_cons_sparseobj(SPFUN fun, int n, Memb_list* ml, _threadargsproto_) {
    // fill in the unset _threadargsproto_ assuming _iml = 0;
    _iml = 0; /* from _threadargsproto_ */
    _p = ml->data;
    _ppvar = ml->pdata;
    _v = _nt->_actual_v[ml->nodeindices[_iml]];
    SparseObj* so{new SparseObj};
    so->_cntml_padded = _cntml_padded;
    scopmath::sparse::create_coef_list(so, n, fun, _threadargs_);
    nrn_sparseobj_copyto_device(so);
    return so;
}

template <typename F>
int sparse_thread(SparseObj* so,
                  int n,
                  int* s,
                  int* d,
                  double* t,
                  double dt,
                  F fun,
                  int linflag,
                  _threadargsproto_) {
    int i, j, ierr;
    double err;

    for (i = 0; i < n; i++) { /*save old state*/
        coreneuron_scopmath_sparse_d(i) = coreneuron_scopmath_sparse_s(i);
    }
    for (err = 1, j = 0; err > CONVERGE; j++) {
        scopmath::sparse::init_coef_list(so, _iml);
        fun(so, so->rhs, _threadargs_);  // std::invoke in C++17
        if ((ierr = scopmath::sparse::matsol(so, _iml))) {
            return ierr;
        }
        for (err = 0., i = 1; i <= n; i++) { /* why oh why did I write it from 1 */
            coreneuron_scopmath_sparse_s(i - 1) += so->rhs[coreneuron_scopmath_sparse_ix(i)];
            if (!linflag && coreneuron_scopmath_sparse_s(i - 1) < 0.) {
                coreneuron_scopmath_sparse_s(i - 1) = 0.;
            }
            err += fabs(so->rhs[coreneuron_scopmath_sparse_ix(i)]);
        }
        if (j > MAXSTEPS) {
            return EXCEED_ITERS;
        }
        if (linflag)
            break;
    }
    scopmath::sparse::init_coef_list(so, _iml);
    fun(so, so->rhs, _threadargs_);  // std::invoke in C++17
    for (i = 0; i < n; i++) {        /*restore Dstate at t+dt*/
        coreneuron_scopmath_sparse_d(
            i) = (coreneuron_scopmath_sparse_s(i) - coreneuron_scopmath_sparse_d(i)) / dt;
    }
    return SUCCESS;
}
#undef coreneuron_scopmath_sparse_d
#undef coreneuron_scopmath_sparse_ix
#undef coreneuron_scopmath_sparse_s
}  // namespace coreneuron
