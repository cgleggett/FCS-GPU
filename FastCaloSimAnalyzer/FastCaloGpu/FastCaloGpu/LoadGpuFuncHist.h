/*
  Copyright (C) 2002-2021 CERN for the benefit of the ATLAS collaboration
*/

#ifndef LOADGPUFUNCHIST_H
#define LOADGPUFUNCHIST_H

#include "FH_structs.h"

#ifdef USE_KOKKOS
#include "FH_views.h"
#endif
#ifdef USE_OMPGPU
#include <omp.h>
#endif

class LoadGpuFuncHist {

public:
  LoadGpuFuncHist();
  ~LoadGpuFuncHist();

  void set_hf(FHs *hf_ptr) { m_hf = hf_ptr; }
  void set_d_hf(FHs *hf_ptr) { m_hf_d = hf_ptr; }
  void set_hf2d(FH2D *hf_ptr) { m_hf2d = hf_ptr; }
  void set_d_hf2d(FH2D *hf_ptr) { m_hf2d_d = hf_ptr; }
  FHs *hf() const {
    return m_hf;
  };
  FHs *hf_h() const {
    return m_hf_h;
  };
  FHs *hf_d() const {
    return m_hf_d;
  };
  FH2D *hf2d() const {
    return m_hf2d;
  };
  FH2D *hf2d_h() const {
    return m_hf2d_h;
  };
  FH2D *hf2d_d() const {
    return m_hf2d_d;
  };

#ifdef USE_KOKKOS
  FHs_v *hf_v() const { return m_hf_v; } // on device
  //  Kokkos::View<FHs>    hf_dv()   const { return m_hf_dv; }      // on device
  FH2D_v *hf2d_v() const { return m_hf2d_v; }
  Kokkos::View<FH2D> hf2d_dv() const { return m_hf2d_dv; }
#endif

  void LD();
  void LD2D();

private:
  struct FHs *m_hf{ nullptr };
  struct FHs *m_hf_d{ nullptr }; // device pointer
  struct FHs *m_hf_h{ nullptr }; // host pointer to struct hold device param
                                 // that is copied to device

  struct FH2D *m_hf2d{ nullptr };
  struct FH2D *m_hf2d_d{ nullptr }; // device pointer
  struct FH2D *m_hf2d_h{ nullptr }; // host poniter struct hold device param to
                                    // be copied to device

#ifdef USE_KOKKOS
  FH2D_v *m_hf2d_v{ 0 };
  Kokkos::View<FH2D> m_hf2d_dv;

  FHs_v *m_hf_v{ 0 };        // on host with device ptrs
  FHs_v *m_hf_v_d{ 0 };      // on device
  Kokkos::View<FHs> m_hf_dv; // on device
#endif

#ifdef USE_ALPAKA
  class Impl;
  Impl* pImpl;
#endif

#ifdef USE_OMPGPU
  int m_default_device = omp_get_default_device();
  int m_initial_device = omp_get_initial_device();
  std::size_t m_offset = 0;
#endif
};

#endif
