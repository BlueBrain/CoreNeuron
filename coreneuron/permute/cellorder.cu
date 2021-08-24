#ifdef ENABLE_CUDA_

//#include <iostream>
//#include <stdio.h>

#include "coreneuron/permute/cellorder.hpp"
#include "coreneuron/network/tnode.hpp"
#include "coreneuron/sim/multicore.hpp"

namespace coreneuron {

#define GPU_A(i)      nt->_actual_a[i]
#define GPU_B(i)      nt->_actual_b[i]
#define GPU_D(i)      nt->_actual_d[i]
#define GPU_RHS(i)    nt->_actual_rhs[i]
#define GPU_PARENT(i) nt->_v_parent_index[i]

__device__
void triang_interleaved2_device(NrnThread* nt, int icore, int ncycle, int* stride, int lastnode)
{
    int icycle = ncycle - 1;
    int istride = stride[icycle];
    int i = lastnode - istride + icore;

    int ip;
    double p;
    while(icycle >= 0) {
        // most efficient if istride equal warpsize, else branch divergence!
        if (icore < istride) {
            ip = GPU_PARENT(i);
            p = GPU_A(i) / GPU_D(i);
            atomicAdd(&GPU_D(ip), - p * GPU_B(i));
            atomicAdd(&GPU_RHS(ip), - p * GPU_RHS(i));
        }
        --icycle;
        istride = stride[icycle];
        i -= istride;
    }
}

__device__
void bksub_interleaved2_device(NrnThread* nt,
                               int root,
                               int lastroot,
                               int icore,
                               int ncycle,
                               int* stride,
                               int firstnode)
{
    for (int i = root; i < lastroot; i += warpsize) {
        GPU_RHS(i) /= GPU_D(i);  // the root
    }

    int i = firstnode + icore;

    int ip;
    for (int icycle = 0; icycle < ncycle; ++icycle) {
        int istride = stride[icycle];
        if (icore < istride) {
            ip = GPU_PARENT(i);
            GPU_RHS(i) -= GPU_B(i) * GPU_RHS(ip);
            GPU_RHS(i) /= GPU_D(i);
        }
        i += istride;
    }
}

__global__
void solve_interleaved2_kernel(NrnThread* nt, InterleaveInfo* ii, int ncore)
{
    int icore = blockDim.x * blockIdx.x + threadIdx.x;
    
    int* ncycles = ii->cellsize;         // nwarp of these
    int* stridedispl = ii->stridedispl;  // nwarp+1 of these
    int* strides = ii->stride;           // sum ncycles of these (bad since ncompart/warpsize)
    int* rootbegin = ii->firstnode;      // nwarp+1 of these
    int* nodebegin = ii->lastnode;       // nwarp+1 of these

    int iwarp = icore / warpsize;     // figure out the >> value
    int ic = icore & (warpsize - 1);  // figure out the & mask
    int ncycle = ncycles[iwarp];
    int* stride = strides + stridedispl[iwarp];
    int root = rootbegin[iwarp];
    int lastroot = rootbegin[iwarp + 1];
    int firstnode = nodebegin[iwarp];
    int lastnode = nodebegin[iwarp + 1];

    triang_interleaved2_device(nt, ic, ncycle, stride, lastnode);
    bksub_interleaved2_device(nt, root + ic, lastroot, ic, ncycle, stride, firstnode);
}

void solve_interleaved2_launcher(NrnThread* nt, InterleaveInfo* info, int ncore)
{
    cudaDeviceSynchronize();
    int threadsPerBlock = warpsize;
    int blocksPerGrid = (ncore + threadsPerBlock - 1) / threadsPerBlock;

    solve_interleaved2_kernel<<<blocksPerGrid,threadsPerBlock>>>(nt, info, ncore);
    cudaDeviceSynchronize();
}

} // namespace coreneuron

#endif // ENABLE_CUDA_
