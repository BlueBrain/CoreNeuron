/******************************************************************************
 *
 * File: newton.c
 *
 * Copyright (c) 1987, 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/

/*------------------------------------------------------------*/
/*                                                            */
/*  NEWTON                                        	      */
/*                                                            */
/*    Iteratively solves simultaneous nonlinear equations by  */
/*    Newton's method, using a Jacobian matrix computed by    */
/*    finite differences.				      */
/*                                                            */
/*  Returns: 0 if no error; 2 if matrix is singular or ill-   */
/*			     conditioned; 1 if maximum	      */
/*			     iterations exceeded	      */
/*                                                            */
/*  Calling sequence: newton(n, x, p, pfunc, value)	      */
/*                                                            */
/*  Arguments:                                                */
/*                                                            */
/*    Input: n, integer, number of variables to solve for.    */
/*                                                            */
/*           x, pointer to array  of the solution             */
/*		vector elements	possibly indexed by index     */
/*                                                            */
/*	     p,  array of parameter values		      */
/*                                                            */
/*           pfunc, pointer to function which computes the    */
/*               deviation from zero of each equation in the  */
/*               model.                                       */
/*                                                            */
/*	     value, pointer to array to array  of             */
/*		 the function values.			      */
/*                                                            */
/*    Output: x contains the solution value or the most       */
/*               recent iteration's result in the event of    */
/*               an error.                                    */
/*                                                            */
/*  Functions called: makevector, freevector, makematrix,     */
/*		      freematrix			      */
/*		      buildjacobian, crout, solve	      */
/*                                                            */
/*------------------------------------------------------------*/

#include <math.h>
#include <stdlib.h>

#include "coreneuron/mechanism/mech/mod2c_core_thread.hpp"
#include "coreneuron/sim/scopmath/errcodes.h"
#include "coreneuron/sim/scopmath/newton_struct.h"
#include "coreneuron/sim/scopmath/newton_thread.hpp"
#include "coreneuron/utils/nrnoc_aux.hpp"

namespace coreneuron {
NewtonSpace* nrn_cons_newtonspace(int n, int n_instance) {
    NewtonSpace* ns = (NewtonSpace*) emalloc(sizeof(NewtonSpace));
    ns->n = n;
    ns->n_instance = n_instance;
    ns->delta_x = makevector(n * n_instance * sizeof(double));
    ns->jacobian = makematrix(n, n * n_instance);
    ns->perm = (int*) emalloc((unsigned) (n * n_instance * sizeof(int)));
    ns->high_value = makevector(n * n_instance * sizeof(double));
    ns->low_value = makevector(n * n_instance * sizeof(double));
    ns->rowmax = makevector(n * n_instance * sizeof(double));
    nrn_newtonspace_copyto_device(ns);
    return ns;
}

void nrn_destroy_newtonspace(NewtonSpace* ns) {
    nrn_newtonspace_delete_from_device(ns);
    free((char*) ns->perm);
    freevector(ns->delta_x);
    freematrix(ns->jacobian);
    freevector(ns->high_value);
    freevector(ns->low_value);
    freevector(ns->rowmax);
    free((char*) ns);
}
}  // namespace coreneuron
