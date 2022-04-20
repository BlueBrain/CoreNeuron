#pragma once
#include "coreneuron/mechanism/mech/mod2c_core_thread.hpp"

namespace coreneuron {

#define s_(arg) _p[s[arg] * _STRIDE]

static int check_state(int n, int* s, _threadargsproto_) {
    bool flag = true;
    for (int i = 0; i < n; i++) {
        if (s_(i) < -1e-6) {
            s_(i) = 0.;
            flag = false;
        }
    }
    return flag ? 1 : 0;
}
#undef s_

nrn_pragma_acc(routine seq)
template <typename SPFUN>
int _ss_sparse_thread(SparseObj* so,
                      int n,
                      int* s,
                      int* d,
                      double* t,
                      double dt,
                      SPFUN fun,
                      int linflag,
                      _threadargsproto_) {
    int err;
    double ss_dt = 1e9;
    _modl_set_dt_thread(ss_dt, _nt);

    if (linflag) { /*iterate linear solution*/
        err = sparse_thread(so, n, s, d, t, ss_dt, fun, 0, _threadargs_);
    } else {
#define NIT 7
        int i = NIT;
        err = 0;
        while (i) {
            err = sparse_thread(so, n, s, d, t, ss_dt, fun, 1, _threadargs_);
            if (!err) {
                if (check_state(n, s, _threadargs_)) {
                    err = sparse_thread(so, n, s, d, t, ss_dt, fun, 0, _threadargs_);
                }
            }
            --i;
            if (!err) {
                i = 0;
            }
        }
    }

    _modl_set_dt_thread(dt, _nt);
    return err;
}

template <typename DIFUN>
int _ss_derivimplicit_thread(int n, int* slist, int* dlist, DIFUN fun, _threadargsproto_) {
    double dtsav = _modl_get_dt_thread(_nt);
    _modl_set_dt_thread(1e-9, _nt);
    int err = fun(_threadargs_);
    _modl_set_dt_thread(dtsav, _nt);
    return err;
}
}
