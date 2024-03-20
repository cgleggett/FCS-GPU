/*
  Copyright (C) 2002-2021 CERN for the benefit of the ATLAS collaboration
*/

#include "GeoRegion.h"
#include <iostream>
#include <algorithm>

#define PI 3.14159265358979323846
#define TWOPI 2 * 3.14159265358979323846

__HOSTDEV__ double Phi_mpi_pi(double x) {
  while (x >= PI)
    x -= TWOPI;
  while (x < -PI)
    x += TWOPI;
  return x;
}

__HOSTDEV__ bool GeoRegion::index_range_adjust(int &ieta, int &iphi) const {
  while (iphi < 0) {
    iphi += m_cell_grid_phi;
  };
  while (iphi >= m_cell_grid_phi) {
    iphi -= m_cell_grid_phi;
  };
  if (ieta < 0) {
    ieta = 0;
    return false;
  }
  if (ieta >= m_cell_grid_eta) {
    ieta = m_cell_grid_eta - 1;
    return false;
  }
  return true;
}

__HOSTDEV__ float
GeoRegion::calculate_distance_eta_phi(const long long DDE, float eta, float phi,
                                      float &dist_eta0,
                                      float &dist_phi0) const {

// this is necessary to ensure that std::max is not used on GPU devices, but is
// used on CPUs (kokkos serial, openmp, pthreads, etc)
// FIXME: replace with KOKKOS_IF_ON_HOST when it's available
#ifdef KOKKOS_ACTIVE_EXECUTION_MEMORY_SPACE_HOST
  using std::max;
#endif
#ifdef USE_STDPAR
  using std::max;
#endif
#ifdef USE_ALPAKA
  using std::max;
#endif
#ifdef USE_OMPGPU
  using std::max;
#endif
  dist_eta0 = (eta - m_all_cells[DDE].eta()) / m_deta_double;
  dist_phi0 = (Phi_mpi_pi(phi - m_all_cells[DDE].phi())) / m_dphi_double;
  float abs_dist_eta0 = abs(dist_eta0);
  float abs_dist_phi0 = abs(dist_phi0);
  return max(abs_dist_eta0, abs_dist_phi0) - 0.5;
}

__HOSTDEV__ long long GeoRegion::getDDE(float eta, float phi, float *distance,
                                        int *steps) {

  float dist;
  long long bestDDE = -1;
  if (!distance)
    distance = &dist;
  *distance = +10000000;
  int intsteps = 0;
  if (!steps)
    steps = &intsteps;

  float best_eta_corr = m_eta_correction;
  float best_phi_corr = m_phi_correction;

  float raw_eta = eta + best_eta_corr;
  float raw_phi = phi + best_phi_corr;

  int ieta = raw_eta_position_to_index(raw_eta);
  int iphi = raw_phi_position_to_index(raw_phi);
  index_range_adjust(ieta, iphi);

  long long newDDE = m_cells_g[ieta * m_cell_grid_phi + iphi];
  float bestdist = +10000000;
  ++(*steps);
  int nsearch = 0;
  while ((newDDE >= 0) && nsearch < 3) {
    float dist_eta0, dist_phi0;

    *distance =
        calculate_distance_eta_phi(newDDE, eta, phi, dist_eta0, dist_phi0);

    bestDDE = newDDE;
    bestdist = *distance;

    if (*distance < 0)
      return newDDE;

    // correct ieta and iphi by the observed difference to the hit cell
    ieta += round(dist_eta0);
    iphi += round(dist_phi0);
    index_range_adjust(ieta, iphi);
    long long oldDDE = newDDE;
    newDDE = m_cells_g[ieta * m_cell_grid_phi + iphi];
    ++(*steps);
    ++nsearch;
    if (oldDDE == newDDE)
      break;
  }
  float minieta = ieta + floor(m_mineta_correction / cell_grid_eta());
  float maxieta = ieta + ceil(m_maxeta_correction / cell_grid_eta());
  float miniphi = iphi + floor(m_minphi_correction / cell_grid_phi());
  float maxiphi = iphi + ceil(m_maxphi_correction / cell_grid_phi());
  if (minieta < 0)
    minieta = 0;
  if (maxieta >= m_cell_grid_eta)
    maxieta = m_cell_grid_eta - 1;
  for (int iieta = minieta; iieta <= maxieta; ++iieta) {
    for (int iiphi = miniphi; iiphi <= maxiphi; ++iiphi) {
      ieta = iieta;
      iphi = iiphi;
      index_range_adjust(ieta, iphi);
      newDDE = m_cells_g[ieta * m_cell_grid_phi + iphi];
      ++(*steps);
      if (newDDE >= 0) {
        float dist_eta0, dist_phi0;
        *distance =
            calculate_distance_eta_phi(newDDE, eta, phi, dist_eta0, dist_phi0);

        if (*distance < 0)
          return newDDE;
        if (*distance < bestdist) {
          bestDDE = newDDE;
          bestdist = *distance;
        }
      } else {
        printf("GeoRegin::getDDE, windows search ieta=%d iphi=%d is empty\n",
               ieta, iphi);
      }
    }
  }
  *distance = bestdist;
  return bestDDE;
}
