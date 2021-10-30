/******************************************************************************
 *
 * File: sparse.c
 *
 * Copyright (c) 1989, 1990
 *   Duke University
 *
 ******************************************************************************/
#include <stdlib.h>
#include "coreneuron/mechanism/mech/cfile/scoplib.h"
#include "coreneuron/mechanism/mech/mod2c_core_thread.hpp" /* _threadargs, _STRIDE, etc. */
#include "coreneuron/sim/scopmath/errcodes.h"
#include "coreneuron/utils/nrnoc_aux.hpp"

/* Aug 2016 coreneuron : very different prototype and memory organization */
/* Jan 2008 thread safe */
/* 4/23/93 converted to object so many models can use it */
/*-----------------------------------------------------------------------------
 *
 *  sparse()
 *
 *  Abstract:
 *  This is an experimental numerical method for SCoP-3 which integrates kinetic
 *  rate equations.  It is intended to be used only by models generated by MODL,
 *  and its identity is meant to be concealed from the user.
 *
 *
 *  Calling sequence:
 *	sparse(n, s, d, t, dt, fun, prhs, linflag)
 *
 *  Arguments:
 * 	n		number of state variables
 * 	s		array of pointers to the state variables
 * 	d		array of pointers to the derivatives of states
 * 	t		pointer to the independent variable
 * 	dt		the time step
 * 	fun		pointer to the function corresponding to the
 *			kinetic block equations
 * 	prhs		pointer to right hand side vector (answer on return)
 *			does not have to be allocated by caller.
 * 	linflag		solve as linear equations
 *			when nonlinear, all states are forced >= 0
 *
 *
 *  Returns:	nothing
 *
 *  Functions called: IGNORE(), printf(), create_coef_list(), fabs()
 *
 *  Files accessed:  none
 *
 */

#if LINT
#define IGNORE(arg) \
    {               \
        if (arg)    \
            ;       \
    }
#else
#define IGNORE(arg) arg
#endif

#if __TURBOC__ || VMS
#define Free(arg) myfree((void*) arg)
#else
#define Free(arg) myfree((char*) arg)
#endif
#define nrn_malloc_lock()   /**/
#define nrn_malloc_unlock() /**/

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/* note: solution order refers to the following
        diag[varord[row]]->row = row = diag[varord[row]]->col
        rowst[varord[row]]->row = row
        varord[el->row] < varord[el->c_right->row]
        varord[el->col] < varord[el->r_down->col]
*/
namespace coreneuron {
static int matsol(SparseObj* so, int _iml);
static void subrow(SparseObj* so, Elm* pivot, Elm* rowsub, int _iml);
static void bksub(SparseObj* so, int _iml);
static void initeqn(SparseObj* so, unsigned maxeqn);
static void free_elm(SparseObj* so);
static Elm* getelm(SparseObj* so, unsigned row, unsigned col, Elm* new_elem);
static void create_coef_list(SparseObj* so, int n, SPFUN fun, _threadargsproto_);
static void init_coef_list(SparseObj* so, int _iml);
static void init_minorder(SparseObj* so);
static void increase_order(SparseObj* so, unsigned row);
static void reduce_order(SparseObj* so, unsigned row);
static void spar_minorder(SparseObj* so);
static void get_next_pivot(SparseObj* so, unsigned i);
static Item* newitem();
static List* newlist();
static void freelist(List* list);
static void linkitem(Item* item, Item* i);
static void insert(SparseObj* so, Item* item);
static void delete_item(Item* item);
static void* myemalloc(unsigned n);
static void myfree(void*);
static void check_assert(SparseObj* so);
static void re_link(SparseObj* so, unsigned i);
static SparseObj* create_sparseobj();

#if defined(_OPENACC)
#undef emalloc
#undef ecalloc
#define emalloc(arg)        malloc(arg)
#define ecalloc(arg1, arg2) malloc((arg1) * (arg2))
#endif

static Elm* nrn_pool_alloc(void* arg) {
    return (Elm*) emalloc(sizeof(Elm));
}

/* sparse matrix dynamic allocation:
create_coef_list makes a list for fast setup, does minimum ordering and
ensures all elements needed are present */
/* this could easily be made recursive but it isn't right now */

void* nrn_cons_sparseobj(SPFUN fun, int n, Memb_list* ml, _threadargsproto_) {
    SparseObj* so;
    // fill in the unset _threadargsproto_ assuming _iml = 0;
    _iml = 0; /* from _threadargsproto_ */
    _p = ml->data;
    _ppvar = ml->pdata;
    _v = _nt->_actual_v[ml->nodeindices[_iml]];

    so = create_sparseobj();
    so->_cntml_padded = _cntml_padded;
    create_coef_list(so, n, fun, _threadargs_);
    nrn_sparseobj_copyto_device(so);
    return so;
}

int sparse_thread(SparseObj* so,
                  int n,
                  int* s,
                  int* d,
                  double* t,
                  double dt,
                  SPFUN fun,
                  int linflag,
                  _threadargsproto_) {
#define ix(arg) ((arg) *_STRIDE)
#define s_(arg) _p[ix(s[arg])]
#define d_(arg) _p[ix(d[arg])]

    int i, j, ierr;
    double err;

    for (i = 0; i < n; i++) { /*save old state*/
        d_(i) = s_(i);
    }
    for (err = 1, j = 0; err > CONVERGE; j++) {
        init_coef_list(so, _iml);
        spfun(fun, so, so->rhs);
        if ((ierr = matsol(so, _iml))) {
            return ierr;
        }
        for (err = 0., i = 1; i <= n; i++) { /* why oh why did I write it from 1 */
            s_(i - 1) += so->rhs[ix(i)];
            if (!linflag && s_(i - 1) < 0.) {
                s_(i - 1) = 0.;
            }
            err += fabs(so->rhs[ix(i)]);
        }
        if (j > MAXSTEPS) {
            return EXCEED_ITERS;
        }
        if (linflag)
            break;
    }
    init_coef_list(so, _iml);
    spfun(fun, so, so->rhs);
    for (i = 0; i < n; i++) { /*restore Dstate at t+dt*/
        d_(i) = (s_(i) - d_(i)) / dt;
    }
    return SUCCESS;
}

/* for solving ax=b */
int _cvode_sparse_thread(void** v, int n, int* x, SPFUN fun, _threadargsproto_)
#define x_(arg) _p[x[arg] * _STRIDE]
{
    SparseObj* so = (SparseObj*) (*v);
    if (!so) {
        so = create_sparseobj();
        *v = (void*) so;
    }
    if (so->oldfun != fun) {
        so->oldfun = fun;
        create_coef_list(so, n, fun, _threadargs_); /* calls fun twice */
    }
    init_coef_list(so, _iml);
    spfun(fun, so, so->rhs);
    int ierr;
    if ((ierr = matsol(so, _iml))) {
        return ierr;
    }
    for (int i = 1; i <= n; i++) { /* why oh why did I write it from 1 */
        x_(i - 1) = so->rhs[i];
    }
    return SUCCESS;
}

static int matsol(SparseObj* so, int _iml) {
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

static void subrow(SparseObj* so, Elm* pivot, Elm* rowsub, int _iml) {
    int _cntml_padded = so->_cntml_padded;
    double r = rowsub->value[_iml] / pivot->value[_iml];
    so->rhs[ix(rowsub->row)] -= so->rhs[ix(pivot->row)] * r;
    so->numop++;
    for (auto el = pivot->c_right; el; el = el->c_right) {
        for (rowsub = rowsub->c_right; rowsub->col != el->col; rowsub = rowsub->c_right) {
            ;
        }
        rowsub->value[_iml] -= el->value[_iml] * r;
        so->numop++;
    }
}

static void bksub(SparseObj* so, int _iml) {
    int _cntml_padded = so->_cntml_padded;
    for (unsigned i = so->neqn; i >= 1; i--) {
        for (Elm* el = so->diag[i]->c_right; el; el = el->c_right) {
            so->rhs[ix(el->row)] -= el->value[_iml] * so->rhs[ix(el->col)];
            so->numop++;
        }
        so->rhs[ix(so->diag[i]->row)] /= so->diag[i]->value[_iml];
        so->numop++;
    }
}

static void initeqn(SparseObj* so, unsigned maxeqn) /* reallocate space for matrix */
{
    if (maxeqn == so->neqn)
        return;
    free_elm(so);
    so->neqn = maxeqn;
    if (so->rowst)
        Free(so->rowst);
    if (so->diag)
        Free(so->diag);
    if (so->varord)
        Free(so->varord);
    if (so->rhs)
        Free(so->rhs);
    if (so->ngetcall)
        free(so->ngetcall);
    so->elmpool = nullptr;
    so->rowst = so->diag = nullptr;
    so->varord = nullptr;
    so->rowst = (Elm**) myemalloc((maxeqn + 1) * sizeof(Elm*));
    so->diag = (Elm**) myemalloc((maxeqn + 1) * sizeof(Elm*));
    so->varord = (unsigned*) myemalloc((maxeqn + 1) * sizeof(unsigned));
    so->rhs = (double*) myemalloc((maxeqn + 1) * so->_cntml_padded * sizeof(double));
    so->ngetcall = (unsigned*) ecalloc(so->_cntml_padded, sizeof(unsigned));
    for (unsigned i = 1; i <= maxeqn; i++) {
        so->varord[i] = i;
        so->diag[i] = (Elm*) nrn_pool_alloc(so->elmpool);
        so->diag[i]->value = (double*) ecalloc(so->_cntml_padded, sizeof(double));
        so->rowst[i] = so->diag[i];
        so->diag[i]->row = i;
        so->diag[i]->col = i;
        so->diag[i]->r_down = so->diag[i]->r_up = ELM0;
        so->diag[i]->c_right = so->diag[i]->c_left = ELM0;
    }
    unsigned nn = so->neqn * so->_cntml_padded;
    for (unsigned i = 0; i < nn; ++i) {
        so->rhs[i] = 0.;
    }
}

static void free_elm(SparseObj* so) {
    /* free all elements */
    for (unsigned i = 1; i <= so->neqn; i++) {
        so->rowst[i] = ELM0;
        so->diag[i] = ELM0;
    }
}

/* see check_assert in minorder for info about how this matrix is supposed
to look.  If new is nonzero and an element would otherwise be created, new
is used instead. This is because linking an element is highly nontrivial
The biggest difference is that elements are no longer removed and this
saves much time allocating and freeing during the solve phase
*/

static Elm* getelm(SparseObj* so, unsigned row, unsigned col, Elm* new_elem)
/* return pointer to row col element maintaining order in rows */
{
    Elm *el, *elnext;

    unsigned vrow = so->varord[row];
    unsigned vcol = so->varord[col];

    if (vrow == vcol) {
        return so->diag[vrow]; /* a common case */
    }
    if (vrow > vcol) { /* in the lower triangle */
        /* search downward from diag[vcol] */
        for (el = so->diag[vcol];; el = elnext) {
            elnext = el->r_down;
            if (!elnext) {
                break;
            } else if (elnext->row == row) { /* found it */
                return elnext;
            } else if (so->varord[elnext->row] > vrow) {
                break;
            }
        }
        /* insert below el */
        if (!new_elem) {
            new_elem = (Elm*) nrn_pool_alloc(so->elmpool);
            new_elem->value = (double*) ecalloc(so->_cntml_padded, sizeof(double));
            increase_order(so, row);
        }
        new_elem->r_down = el->r_down;
        el->r_down = new_elem;
        new_elem->r_up = el;
        if (new_elem->r_down) {
            new_elem->r_down->r_up = new_elem;
        }
        /* search leftward from diag[vrow] */
        for (el = so->diag[vrow];; el = elnext) {
            elnext = el->c_left;
            if (!elnext) {
                break;
            } else if (so->varord[elnext->col] < vcol) {
                break;
            }
        }
        /* insert to left of el */
        new_elem->c_left = el->c_left;
        el->c_left = new_elem;
        new_elem->c_right = el;
        if (new_elem->c_left) {
            new_elem->c_left->c_right = new_elem;
        } else {
            so->rowst[vrow] = new_elem;
        }
    } else { /* in the upper triangle */
        /* search upward from diag[vcol] */
        for (el = so->diag[vcol];; el = elnext) {
            elnext = el->r_up;
            if (!elnext) {
                break;
            } else if (elnext->row == row) { /* found it */
                return elnext;
            } else if (so->varord[elnext->row] < vrow) {
                break;
            }
        }
        /* insert above el */
        if (!new_elem) {
            new_elem = (Elm*) nrn_pool_alloc(so->elmpool);
            new_elem->value = (double*) ecalloc(so->_cntml_padded, sizeof(double));
            increase_order(so, row);
        }
        new_elem->r_up = el->r_up;
        el->r_up = new_elem;
        new_elem->r_down = el;
        if (new_elem->r_up) {
            new_elem->r_up->r_down = new_elem;
        }
        /* search right from diag[vrow] */
        for (el = so->diag[vrow];; el = elnext) {
            elnext = el->c_right;
            if (!elnext) {
                break;
            } else if (so->varord[elnext->col] > vcol) {
                break;
            }
        }
        /* insert to right of el */
        new_elem->c_right = el->c_right;
        el->c_right = new_elem;
        new_elem->c_left = el;
        if (new_elem->c_right) {
            new_elem->c_right->c_left = new_elem;
        }
    }
    new_elem->row = row;
    new_elem->col = col;
    return new_elem;
}

double* _nrn_thread_getelm(SparseObj* so, int row, int col, int _iml) {
    if (!so->phase) {
        return so->coef_list[so->ngetcall[_iml]++];
    }
    Elm* el = getelm(so, (unsigned) row, (unsigned) col, ELM0);
    if (so->phase == 1) {
        so->ngetcall[_iml]++;
    } else {
        so->coef_list[so->ngetcall[_iml]++] = el->value;
    }
    return el->value;
}

static void create_coef_list(SparseObj* so, int n, SPFUN fun, _threadargsproto_) {
    initeqn(so, (unsigned) n);
    so->phase = 1;
    so->ngetcall[0] = 0;
    spfun(fun, so, so->rhs);
    if (so->coef_list) {
        free(so->coef_list);
    }
    so->coef_list_size = so->ngetcall[0];
    so->coef_list = (double**) myemalloc(so->ngetcall[0] * sizeof(double*));
    spar_minorder(so);
    so->phase = 2;
    so->ngetcall[0] = 0;
    spfun(fun, so, so->rhs);
    so->phase = 0;
}

static void init_coef_list(SparseObj* so, int _iml) {
    so->ngetcall[_iml] = 0;
    for (unsigned i = 1; i <= so->neqn; i++) {
        for (Elm* el = so->rowst[i]; el; el = el->c_right) {
            el->value[_iml] = 0.;
        }
    }
}

static void init_minorder(SparseObj* so) {
    /* matrix has been set up. Construct the orderlist and orderfind
       vector.
    */

    so->do_flag = 1;
    if (so->roworder) {
        for (unsigned i = 1; i <= so->nroworder; ++i) {
            Free(so->roworder[i]);
        }
        Free(so->roworder);
    }
    so->roworder = (Item**) myemalloc((so->neqn + 1) * sizeof(Item*));
    so->nroworder = so->neqn;
    if (so->orderlist)
        freelist(so->orderlist);
    so->orderlist = newlist();
    for (unsigned i = 1; i <= so->neqn; i++) {
        so->roworder[i] = newitem();
    }
    for (unsigned i = 1; i <= so->neqn; i++) {
        unsigned j = 0;
        for (auto el = so->rowst[i]; el; el = el->c_right) {
            j++;
        }
        so->roworder[so->diag[i]->row]->elm = so->diag[i];
        so->roworder[so->diag[i]->row]->norder = j;
        insert(so, so->roworder[so->diag[i]->row]);
    }
}

static void increase_order(SparseObj* so, unsigned row) {
    /* order of row increases by 1. Maintain the orderlist. */

    if (!so->do_flag)
        return;
    Item* order = so->roworder[row];
    delete_item(order);
    order->norder++;
    insert(so, order);
}

static void reduce_order(SparseObj* so, unsigned row) {
    /* order of row decreases by 1. Maintain the orderlist. */

    if (!so->do_flag)
        return;
    Item* order = so->roworder[row];
    delete_item(order);
    order->norder--;
    insert(so, order);
}

static void spar_minorder(SparseObj* so) { /* Minimum ordering algorithm to determine the order
                        that the matrix should be solved. Also make sure
                        all needed elements are present.
                        This does not mess up the matrix
                      */
    check_assert(so);
    init_minorder(so);
    for (unsigned i = 1; i <= so->neqn; i++) {
        get_next_pivot(so, i);
    }
    so->do_flag = 0;
    check_assert(so);
}

static void get_next_pivot(SparseObj* so, unsigned i) {
    /* get varord[i], etc. from the head of the orderlist. */
    Item* order = so->orderlist->next;
    assert(order != so->orderlist);

    unsigned j;
    if ((j = so->varord[order->elm->row]) != i) {
        /* push order lists down by 1 and put new diag in empty slot */
        assert(j > i);
        Elm* el = so->rowst[j];
        for (; j > i; j--) {
            so->diag[j] = so->diag[j - 1];
            so->rowst[j] = so->rowst[j - 1];
            so->varord[so->diag[j]->row] = j;
        }
        so->diag[i] = order->elm;
        so->rowst[i] = el;
        so->varord[so->diag[i]->row] = i;
        /* at this point row links are out of order for diag[i]->col
           and col links are out of order for diag[i]->row */
        re_link(so, i);
    }

    /* now make sure all needed elements exist */
    for (Elm* el = so->diag[i]->r_down; el; el = el->r_down) {
        for (Elm* pivot = so->diag[i]->c_right; pivot; pivot = pivot->c_right) {
            IGNORE(getelm(so, el->row, pivot->col, ELM0));
        }
        reduce_order(so, el->row);
    }

#if CORENRN_DEBUG
    {
        int j;
        Item* _or;
        printf("%d  ", i);
        for (_or = so->orderlist->next, j = 0; j < 5 && _or != so->orderlist;
             j++, _or = _or->next) {
            printf("(%d, %d)  ", _or->elm->row, _or->norder);
        }
        printf("\n");
    }
#endif
    delete_item(order);
}

/* The following routines support the concept of a list.
modified from modl
*/

/* Implementation
  The list is a doubly linked list. A special item with element 0 is
  always at the tail of the list and is denoted as the List pointer itself.
  list->next point to the first item in the list and
  list->prev points to the last item in the list.
        i.e. the list is circular
  Note that in an empty list next and prev points to itself.

It is intended that this implementation be hidden from the user via the
following function calls.
*/

static Item* newitem() {
    Item* i = (Item*) myemalloc(sizeof(Item));
    i->prev = ITEM0;
    i->next = ITEM0;
    i->norder = 0;
    i->elm = nullptr;
    return i;
}

static List* newlist() {
    Item* i = newitem();
    i->prev = i;
    i->next = i;
    return (List*) i;
}

static void freelist(List* list) /*free the list but not the elements*/
{
    Item* i2;
    for (Item* i1 = list->next; i1 != list; i1 = i2) {
        i2 = i1->next;
        Free(i1);
    }
    Free(list);
}

static void linkitem(Item* item, Item* i) /*link i before item*/
{
    i->prev = item->prev;
    i->next = item;
    item->prev = i;
    i->prev->next = i;
}

static void insert(SparseObj* so, Item* item) {
    Item* i;

    for (i = so->orderlist->next; i != so->orderlist; i = i->next) {
        if (i->norder >= item->norder) {
            break;
        }
    }
    linkitem(i, item);
}

static void delete_item(Item* item) {
    item->next->prev = item->prev;
    item->prev->next = item->next;
    item->prev = ITEM0;
    item->next = ITEM0;
}

static void* myemalloc(unsigned n) { /* check return from malloc */
    nrn_malloc_lock();
    void* p = malloc(n);
    nrn_malloc_unlock();
    if (p == (void*) 0) {
        abort_run(LOWMEM);
    }
    return (void*) p;
}

void myfree(void* ptr) {
    nrn_malloc_lock();
    free(ptr);
    nrn_malloc_unlock();
}

static void check_assert(SparseObj* so) {
    /* check that all links are consistent */
    for (unsigned i = 1; i <= so->neqn; i++) {
        assert(so->diag[i]);
        assert(so->diag[i]->row == so->diag[i]->col);
        assert(so->varord[so->diag[i]->row] == i);
        assert(so->rowst[i]->row == so->diag[i]->row);
        for (Elm* el = so->rowst[i]; el; el = el->c_right) {
            if (el == so->rowst[i]) {
                assert(el->c_left == ELM0);
            } else {
                assert(el->c_left->c_right == el);
                assert(so->varord[el->c_left->col] < so->varord[el->col]);
            }
        }
        for (Elm* el = so->diag[i]->r_down; el; el = el->r_down) {
            assert(el->r_up->r_down == el);
            assert(so->varord[el->r_up->row] < so->varord[el->row]);
        }
        for (Elm* el = so->diag[i]->r_up; el; el = el->r_up) {
            assert(el->r_down->r_up == el);
            assert(so->varord[el->r_down->row] > so->varord[el->row]);
        }
    }
}

/* at this point row links are out of order for diag[i]->col
   and col links are out of order for diag[i]->row */
static void re_link(SparseObj* so, unsigned i) {
    for (Elm* el = so->rowst[i]; el; el = el->c_right) {
        /* repair hole */
        if (el->r_up)
            el->r_up->r_down = el->r_down;
        if (el->r_down)
            el->r_down->r_up = el->r_up;
    }

    for (Elm* el = so->diag[i]->r_down; el; el = el->r_down) {
        /* repair hole */
        if (el->c_right)
            el->c_right->c_left = el->c_left;
        if (el->c_left)
            el->c_left->c_right = el->c_right;
        else
            so->rowst[so->varord[el->row]] = el->c_right;
    }

    for (Elm* el = so->diag[i]->r_up; el; el = el->r_up) {
        /* repair hole */
        if (el->c_right)
            el->c_right->c_left = el->c_left;
        if (el->c_left)
            el->c_left->c_right = el->c_right;
        else
            so->rowst[so->varord[el->row]] = el->c_right;
    }

    /* matrix is consistent except that diagonal row elements are unlinked from
    their columns and the diagonal column elements are unlinked from their
    rows.
    For simplicity discard all knowledge of links and use getelm to relink
    */
    Elm *dright, *dleft, *dup, *ddown, *elnext;

    so->rowst[i] = so->diag[i];
    dright = so->diag[i]->c_right;
    dleft = so->diag[i]->c_left;
    dup = so->diag[i]->r_up;
    ddown = so->diag[i]->r_down;
    so->diag[i]->c_right = so->diag[i]->c_left = ELM0;
    so->diag[i]->r_up = so->diag[i]->r_down = ELM0;
    for (Elm* el = dright; el; el = elnext) {
        elnext = el->c_right;
        IGNORE(getelm(so, el->row, el->col, el));
    }
    for (Elm* el = dleft; el; el = elnext) {
        elnext = el->c_left;
        IGNORE(getelm(so, el->row, el->col, el));
    }
    for (Elm* el = dup; el; el = elnext) {
        elnext = el->r_up;
        IGNORE(getelm(so, el->row, el->col, el));
    }
    for (Elm* el = ddown; el; el = elnext) {
        elnext = el->r_down;
        IGNORE(getelm(so, el->row, el->col, el));
    }
}

static SparseObj* create_sparseobj() {
    SparseObj* so = (SparseObj*) myemalloc(sizeof(SparseObj));
    nrn_malloc_lock();
    nrn_malloc_unlock();
    so->rowst = 0;
    so->diag = 0;
    so->neqn = 0;
    so->_cntml_padded = 0;
    so->varord = 0;
    so->rhs = 0;
    so->oldfun = 0;
    so->ngetcall = 0;
    so->phase = 0;
    so->coef_list = 0;
    so->roworder = 0;
    so->nroworder = 0;
    so->orderlist = 0;
    so->do_flag = 0;

    return so;
}

void _nrn_destroy_sparseobj_thread(SparseObj* so) {
    if (!so) {
        return;
    }
    nrn_sparseobj_delete_from_device(so);
    if (so->rowst)
        Free(so->rowst);
    if (so->diag)
        Free(so->diag);
    if (so->varord)
        Free(so->varord);
    if (so->rhs)
        Free(so->rhs);
    if (so->coef_list)
        Free(so->coef_list);
    if (so->roworder) {
        for (int i = 1; i <= so->nroworder; ++i) {
            Free(so->roworder[i]);
        }
        Free(so->roworder);
    }
    if (so->orderlist)
        freelist(so->orderlist);
    Free(so);
}
}  // namespace coreneuron
