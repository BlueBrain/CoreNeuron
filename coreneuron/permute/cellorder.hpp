#ifndef cellorder_h
#define cellorder_h

#include "coreneuron/utils/memory.h"
#include <algorithm>
namespace coreneuron {
int* interleave_order(int ith, int ncell, int nnode, int* parent);

void create_interleave_info();
void destroy_interleave_info();
extern void solve_interleaved(int ith);

class InterleaveInfo {
  public:
    InterleaveInfo() = default;
    InterleaveInfo(const InterleaveInfo&) = delete;
    InterleaveInfo& operator=(const InterleaveInfo&) = delete;
    virtual ~InterleaveInfo();
    int nwarp = 0;  // used only by interleave2
    int nstride = 0;
    int* stridedispl = nullptr;  // interleave2: nwarp+1
    int* stride = nullptr;       // interleave2: stride  length is stridedispl[nwarp]
    int* firstnode = nullptr;    // interleave2: rootbegin nwarp+1 displacements
    int* lastnode = nullptr;     // interleave2: nodebegin nwarp+1 displacements
    int* cellsize = nullptr;     // interleave2: ncycles nwarp

    // statistics (nwarp of each)
    size_t* nnode = nullptr;
    size_t* ncycle = nullptr;
    size_t* idle = nullptr;
    size_t* cache_access = nullptr;
    size_t* child_race = nullptr;
};

// interleaved from cellorder2.cpp
int* node_order(int ncell,
                int nnode,
                int* parents,
                int& nwarp,
                int& nstride,
                int*& stride,
                int*& firstnode,
                int*& lastnode,
                int*& cellsize,
                int*& stridedispl);

#define INTERLEAVE_DEBUG 0

#if INTERLEAVE_DEBUG
void mk_cell_indices();
#endif
}  // namespace coreneuron
#endif
