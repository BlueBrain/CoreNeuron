/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#ifndef nrnmpi_h
#define nrnmpi_h

#include <cassert>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

#include "coreneuron/mpi/nrnmpiuse.h"

namespace coreneuron {
/* by default nrnmpi_numprocs_world = nrnmpi_numprocs = nrnmpi_numsubworlds and
   nrnmpi_myid_world = nrnmpi_myid and the bulletin board and network communication do
   not easily coexist. ParallelContext.subworlds(nsmall) divides the world into
   nrnmpi_numprocs_world/small subworlds of size nsmall.
*/
extern int nrnmpi_numprocs_world; /* size of entire world. total size of all subworlds */
extern int nrnmpi_myid_world;     /* rank in entire world */
extern int nrnmpi_numprocs;       /* size of subworld */
extern int nrnmpi_myid;           /* rank in subworld */
extern int nrnmpi_numprocs_bbs;   /* number of subworlds */
extern int nrnmpi_myid_bbs;       /* rank in nrn_bbs_comm of rank 0 of a subworld */

extern int nrnmpi_local_rank();
extern int nrnmpi_local_size();
}  // namespace coreneuron

#ifdef NRNMPI

namespace coreneuron {
typedef struct {
    int gid;
    double spiketime;
} NRNMPI_Spike;

extern bool nrnmpi_use; /* NEURON does MPI init and terminate?*/

// Those functions and classes are part of a mechanism to dynamically or statically load mpi functions
struct mpi_function_base;

struct mpi_manager_t {
  void register_function(mpi_function_base* ptr) {
    m_function_ptrs.push_back(ptr);
  }
  void resolve_symbols(void* dlsym_handle);
private:
  std::vector<mpi_function_base*> m_function_ptrs;
};

inline mpi_manager_t& mpi_manager() {
  static mpi_manager_t x;
  return x;
}

struct mpi_function_base {
  void resolve(void* dlsym_handle);
  operator bool() const { return m_fptr; }
  mpi_function_base(const char* name)
  : m_name{name} {
    mpi_manager().register_function(this);
  }
protected:
  void* m_fptr{};
  const char* m_name;
};

// This could be done with a simpler
//   template <auto fptr> struct function : function_base { ... };
// pattern in C++17...
template <typename>
struct mpi_function {};

#define cnrn_make_integral_constant_t(x) std::integral_constant<std::decay_t<decltype(x)>, x>

template <typename function_ptr, function_ptr fptr>
struct mpi_function<std::integral_constant<function_ptr, fptr>> : mpi_function_base {
    using mpi_function_base::mpi_function_base;
    template <typename... Args> // in principle deducible from `function_ptr`
    auto operator()(Args&&... args) const {
#ifdef CORENRN_ENABLE_DYNAMIC_MPI
        // Dynamic MPI, m_fptr should have been initialised via dlsym.
        assert(m_fptr);
        std::cout << "Calling dynamically loaded " << m_name << std::endl;
        return (*reinterpret_cast<decltype(fptr)>(m_fptr))(std::forward<Args>( args )...);
#else
        // No dynamic MPI, use `fptr` directly. Will produce link errors if libmpi.so is not linked.
        std::cout << "Calling directly linked " << m_name << std::endl;
        return (*fptr)(std::forward<Args>(args)...);
#endif
    }
};
}  // namespace coreneuron
#include "coreneuron/mpi/nrnmpidec.h"

#endif /*NRNMPI*/
#endif /*nrnmpi_h*/
