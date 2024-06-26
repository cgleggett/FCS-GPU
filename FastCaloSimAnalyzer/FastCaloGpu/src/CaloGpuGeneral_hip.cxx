#include "hip/hip_runtime.h"
/*
  Copyright (C) 2002-2021 CERN for the benefit of the ATLAS collaboration
*/

#include "CaloGpuGeneral_cu.h"
#include "GeoRegion.h"
#include "GeoGpu_structs.h"
#include "Hit.h"
#include "Rand4Hits.h"

#include "gpuQ.h"
#include "Args.h"
#include "DEV_BigMem.h"
#include <chrono>
#include <climits>
#include <mutex>

#define DEFAULT_BLOCK_SIZE 256

#define M_PI 3.14159265358979323846
#define M_2PI 6.28318530717958647692

using namespace CaloGpuGeneral_fnc;

static std::once_flag calledGetEnv{};
static int BLOCK_SIZE{ DEFAULT_BLOCK_SIZE };

static int count{ 0 };

static CaloGpuGeneral::KernelTime timing;

namespace CaloGpuGeneral_cu {

__host__ void Rand4Hits_finish(void *rd4h) {

  size_t free, total;
  gpuQ(hipMemGetInfo(&free, &total));
  std::cout << "GPU memory used(MB): " << (total - free) / 1000000 << std::endl;
  if ((Rand4Hits *)rd4h)
    delete (Rand4Hits *)rd4h;

  if (timing.count > 0) {
    std::cout << "kernel timing\n";
    std::cout << timing;
    // std::cout << "\n\n\n";
    // timing.printAll();
  } else {
    std::cout << "no kernel timing available" << std::endl;
  }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

__global__ void simulate_hits_de(const Sim_Args args) {

  long tid = threadIdx.x + blockIdx.x * blockDim.x;
  if (tid < args.nhits) {

    Hit hit;

    int bin = find_index_long(args.simbins, args.nbins, tid);
    HitParams hp = args.hitparams[bin];
    hit.E() = hp.E;

    CenterPositionCalculation_g_d(hp, hit, tid, args);
    HistoLateralShapeParametrization_g_d(hp, hit, tid, args);
    if (hp.cmw)
      HitCellMappingWiggle_g_d(hp, hit, tid, args);
    HitCellMapping_g_d(hp, hit, tid, args);
  }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

__global__ void simulate_hits_ct(const Sim_Args args) {

  unsigned long tid = threadIdx.x + blockIdx.x * blockDim.x;
  int sim = tid / args.ncells;
  unsigned long cellid = tid % args.ncells;

  if (tid < args.ncells * args.nsims) {
    if (args.cells_energy[tid] > 0) {
      unsigned int ct = atomicAdd(&args.ct[sim], 1);
      Cell_E ce;
      ce.cellid = cellid;
      ce.energy = args.cells_energy[tid];
      args.hitcells_E[ct + sim * MAXHITCT] = ce;
      // if(sim==0) printf("sim: %d  ct=%d cellid=%ld e=%f\n", sim, ct, cellid,
      // ce.energy);
    }
  }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

__global__ void simulate_clean(Sim_Args args) {
  unsigned long tid = threadIdx.x + blockIdx.x * blockDim.x;
  if (tid < args.ncells * args.nsims) {
    args.cells_energy[tid] = 0.0;
  }
  if (tid < args.nsims)
    args.ct[tid] = 0;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

__host__ void simulate_hits_gr(Sim_Args &args) {

  std::call_once(calledGetEnv, []() {
    if (const char *env_p = std::getenv("FCS_BLOCK_SIZE")) {
      std::string bs(env_p);
      BLOCK_SIZE = std::stoi(bs);
    }
    if (BLOCK_SIZE != DEFAULT_BLOCK_SIZE) {
      std::cout << "kernel BLOCK_SIZE: " << BLOCK_SIZE << std::endl;
    }
  });

  hipError_t err = hipGetLastError();

  int blocksize = BLOCK_SIZE;
  int threads_tot = args.ncells * args.nsims;
  int nblocks = (threads_tot + blocksize - 1) / blocksize;

  // clean workspace
  auto t0 = std::chrono::system_clock::now();
  hipLaunchKernelGGL(simulate_clean, nblocks, blocksize, 0, 0, args);
  gpuQ(hipGetLastError());
  gpuQ(hipDeviceSynchronize());

  // main simulation
  blocksize = BLOCK_SIZE;
  threads_tot = args.nhits;
  nblocks = (threads_tot + blocksize - 1) / blocksize;
  auto t1 = std::chrono::system_clock::now();
  hipLaunchKernelGGL(simulate_hits_de, nblocks, blocksize, 0, 0, args);
  gpuQ(hipGetLastError());
  gpuQ(hipDeviceSynchronize());

  // stream compaction
  nblocks = (args.ncells * args.nsims + blocksize - 1) / blocksize;
  auto t2 = std::chrono::system_clock::now();
  hipLaunchKernelGGL(simulate_hits_ct, nblocks, blocksize, 0, 0, args);
  gpuQ(hipGetLastError());
  gpuQ(hipDeviceSynchronize());

  // copy back to host
  auto t3 = std::chrono::system_clock::now();
  gpuQ(hipMemcpy(args.ct_h, args.ct, args.nsims * sizeof(int),
                  hipMemcpyDeviceToHost));
  gpuQ(hipMemcpy(args.hitcells_E_h, args.hitcells_E,
                  MAXHITCT * args.nsims * sizeof(Cell_E),
                  hipMemcpyDeviceToHost));

  auto t4 = std::chrono::system_clock::now();

#ifdef DUMP_HITCELLS
  std::cout << "nsim: " << args.nsims << "\n";
  for (int isim = 0; isim < args.nsims; ++isim) {
    std::cout << "  nhit: " << args.ct_h[isim] << "\n";
    std::map<unsigned int, float> cm;
    for (int ihit = 0; ihit < args.ct_h[isim]; ++ihit) {
      cm[args.hitcells_E_h[ihit + isim * MAXHITCT].cellid] =
          args.hitcells_E_h[ihit + isim * MAXHITCT].energy;
    }

    int i = 0;
    for (auto &em : cm) {
      std::cout << "   " << isim << " " << i++ << "  cell: " << em.first << "  "
                << em.second << std::endl;
    }
  }
#endif

  timing.add(t1 - t0, t2 - t1, t3 - t2, t4 - t3);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

__host__ void load_hitsim_params(void *rd4h, HitParams *hp, long *simbins,
                                 int bins) {

  if (!(Rand4Hits *)rd4h) {
    std::cout << "Error load hit simulation params ! ";
    exit(2);
  }

  HitParams *hp_g = ((Rand4Hits *)rd4h)->get_hitparams();
  long *simbins_g = ((Rand4Hits *)rd4h)->get_simbins();

  gpuQ(hipMemcpy(hp_g, hp, bins * sizeof(HitParams), hipMemcpyHostToDevice));
  gpuQ(hipMemcpy(simbins_g, simbins, bins * sizeof(long),
                  hipMemcpyHostToDevice));
}

} // namespace CaloGpuGeneral_cu