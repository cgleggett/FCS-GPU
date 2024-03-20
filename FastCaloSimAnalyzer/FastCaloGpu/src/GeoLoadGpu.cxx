/*
  Copyright (C) 2002-2021 CERN for the benefit of the ATLAS collaboration
*/

#include "GeoLoadGpu.h"

bool GeoLoadGpu::LoadGpu() {

#if defined(USE_STDPAR)
  return LoadGpu_sp();
#elif defined(USE_KOKKOS)
  return LoadGpu_kk();
#elif defined(USE_ALPAKA)
  return LoadGpu_al();
#elif defined(USE_OMPGPU)
  return LoadGpu_omp();
#else
  return LoadGpu_cu();
#endif
}

Rg_Sample_Index* GeoLoadGpu::get_sample_index_h() {
#if defined(USE_ALPAKA)
  return get_sample_index_h_al();
#else
  return nullptr;
#endif
}

GeoRegion* GeoLoadGpu::get_regions() {
#if defined(USE_ALPAKA)
  return get_regions_al();
#else
  return nullptr;
#endif
}

long long* GeoLoadGpu::get_cell_grid(int neta, int nphi)
{
#if defined(USE_ALPAKA)
  return get_cell_grid_al(neta, nphi);
#else
  return nullptr;
#endif
}
