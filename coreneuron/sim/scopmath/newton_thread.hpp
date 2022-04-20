#pragma once
#include "coreneuron/sim/scopmath/errcodes.h"
#include "coreneuron/sim/scopmath/newton_struct.h"
#include "coreneuron/sim/scopmath/crout_thread.hpp"

namespace coreneuron {
#define ix(arg) ((arg) *_STRIDE)
#define s_(arg) _p[s[arg] * _STRIDE]
#define x_(arg) _p[(arg) *_STRIDE]
namespace detail {
/*------------------------------------------------------------*/
/*                                                            */
/*  BUILDJACOBIAN                                 	      */
/*                                                            */
/*    Creates the Jacobian matrix by computing partial deriv- */
/*    atives by finite central differences.  If the column    */
/*    variable is nonzero, an increment of 2% of the variable */
/*    is used.  STEP is the minimum increment allowed; it is  */
/*    currently set to 1.0E-6.                                */
/*                                                            */
/*  Returns: no return variable                               */
/*                                                            */
/*  Calling sequence:					      */
/*	 buildjacobian(n, index, x, pfunc, value, jacobian)       */
/*                                                            */
/*  Arguments:                                                */
/*                                                            */
/*    Input: n, integer, number of variables                  */
/*                                                            */
/*           x, pointer to array of addresses of the solution */
/*		vector elements				      */
/*                                                            */
/*	     p, array of parameter values		      */
/*                                                            */
/*           pfunc, pointer to function which computes the    */
/*                     deviation from zero of each equation   */
/*                     in the model.                          */
/*                                                            */
/*	     value, pointer to array of addresses of function */
/*		       values				      */
/*                                                            */
/*    Output: jacobian, double, computed jacobian matrix      */
/*                                                            */
/*  Functions called:  user-supplied function with argument   */
/*                     (p) to compute vector of function      */
/*		       values for each equation.	      */
/*		       makevector(), freevector()	      */
/*                                                            */
/*------------------------------------------------------------*/
template <typename F>
void nrn_buildjacobian_thread(NewtonSpace* ns,
                              int n,
                              int* index,
                              F const& func,
                              double* value,
                              double** jacobian,
                              _threadargsproto_) {
    double* high_value = ns->high_value;
    double* low_value = ns->low_value;

    /* Compute partial derivatives by central finite differences */

    for (int j = 0; j < n; j++) {
        double increment = max(fabs(0.02 * (x_(index[j]))), STEP);
        x_(index[j]) += increment;
        func(_threadargs_); // std::invoke in C++17
        for (int i = 0; i < n; i++)
            high_value[ix(i)] = value[ix(i)];
        x_(index[j]) -= 2.0 * increment;
        func(_threadargs_); // std::invoke in C++17
        for (int i = 0; i < n; i++) {
            low_value[ix(i)] = value[ix(i)];

            /* Insert partials into jth column of Jacobian matrix */

            jacobian[i][ix(j)] = (high_value[ix(i)] - low_value[ix(i)]) / (2.0 * increment);
        }

        /* Restore original variable and function values. */

        x_(index[j]) += increment;
        func(_threadargs_); // std::invoke in C++17
    }
}
#undef x_
}
template <typename F>
inline int nrn_newton_thread(NewtonSpace* ns,
                      int n,
                      int* s,
                      F func,
                      double* value,
                      _threadargsproto_) {
    int count = 0, error = 0;
    double change = 1.0, max_dev, temp;
    int done = 0;
    /*
     * Create arrays for Jacobian, variable increments, function values, and
     * permutation vector
     */
    double* delta_x = ns->delta_x;
    double** jacobian = ns->jacobian;
    int* perm = ns->perm;
    /* Iteration loop */
    while (!done) {
        if (count++ >= MAXITERS) {
            error = EXCEED_ITERS;
            done = 2;
        }
        if (!done && change > MAXCHANGE) {
            /*
             * Recalculate Jacobian matrix if solution has changed by more
             * than MAXCHANGE
             */

            detail::nrn_buildjacobian_thread(ns, n, s, func, value, jacobian, _threadargs_);
            for (int i = 0; i < n; i++)
                value[ix(i)] = -value[ix(i)]; /* Required correction to
                                               * function values */
            error = nrn_crout_thread(ns, n, jacobian, perm, _threadargs_);
            if (error != SUCCESS) {
                done = 2;
            }
        }

        if (!done) {
            nrn_scopmath_solve_thread(n, jacobian, value, perm, delta_x, (int*) 0, _threadargs_);

            /* Update solution vector and compute norms of delta_x and value */

            change = 0.0;
            if (s) {
                for (int i = 0; i < n; i++) {
                    if (fabs(s_(i)) > ZERO && (temp = fabs(delta_x[ix(i)] / (s_(i)))) > change)
                        change = temp;
                    s_(i) += delta_x[ix(i)];
                }
            } else {
                for (int i = 0; i < n; i++) {
                    if (fabs(s_(i)) > ZERO && (temp = fabs(delta_x[ix(i)] / (s_(i)))) > change)
                        change = temp;
                    s_(i) += delta_x[ix(i)];
                }
            }
            // Evaulate function values with new solution.
            func(_threadargs_); // std::invoke in C++17
            max_dev = 0.0;
            for (int i = 0; i < n; i++) {
                value[ix(i)] = -value[ix(i)]; /* Required correction to function
                                               * values */
                if ((temp = fabs(value[ix(i)])) > max_dev)
                    max_dev = temp;
            }

            /* Check for convergence or maximum iterations */

            if (change <= CONVERGE && max_dev <= ZERO) {
                // break;
                done = 1;
            }
        }
    } /* end of while loop */

    return (error);
}
#undef ix
#undef s_
}
