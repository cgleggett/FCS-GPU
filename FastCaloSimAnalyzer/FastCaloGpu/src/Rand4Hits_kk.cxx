/*
  Copyright (C) 2002-2021 CERN for the benefit of the ATLAS collaboration
*/

#include "Rand4Hits.h"
#include "Rand4Hits_cpu.cxx"

#include <Kokkos_Core.hpp>
#include <Kokkos_Random.hpp>
#include <vector>

#define kok_randgen_t Kokkos::Random_XorShift64_Pool<>

void Rand4Hits::allocate_simulation( long long /*maxhits*/, unsigned short /*maxbins*/, unsigned short maxhitct,
                                     unsigned long n_cells ) {

  m_cells_energy_v = Kokkos::View<float*>( "Cell energies", n_cells );
  m_cells_energy   = m_cells_energy_v.data();

  m_cell_e_v = Kokkos::View<Cell_E*>( "Cells", maxhitct );
  m_cell_e   = m_cell_e_v.data();

  m_cell_e_h = new Cell_E[maxhitct];
  
  m_ct_v = Kokkos::View<int>( "hit count" );
  m_ct   = m_ct_v.data();

}

Rand4Hits::~Rand4Hits() {
  if ( m_useCPU ) {
    destroyCPUGen();
  } else {
    delete (kok_randgen_t*)m_gen;
  }
  delete m_cell_e_h;
};

void Rand4Hits::rd_regen() {
  Kokkos::View<float*> devData_v( m_rand_ptr, 3 * m_total_a_hits );
  if ( m_useCPU ) {
    genCPU( 3 * m_total_a_hits );
    Kokkos::View<float *, Kokkos::HostSpace> rhst(m_rnd_cpu->data(), 3 * m_total_a_hits);
    Kokkos::deep_copy( m_rand_ptr_v, rhst );
  } else {
    Kokkos::fill_random( devData_v, *( (kok_randgen_t*)m_gen ), 1.f );
  }
};

void Rand4Hits::create_gen( unsigned long long seed, size_t num, bool useCPU ) {

  m_rand_ptr_v = Kokkos::View<float*>( "Random numbers", num );

  m_useCPU = useCPU;
  
  if ( m_useCPU ) {
    createCPUGen( seed );
    genCPU( num );
    Kokkos::View<float*, Kokkos::HostSpace> rhst( m_rnd_cpu->data(), num );
    Kokkos::deep_copy( m_rand_ptr_v, rhst );
  } else {
    kok_randgen_t* gen{nullptr};
    gen = new kok_randgen_t( seed );
    Kokkos::fill_random( m_rand_ptr_v, *gen, 1.f );
    m_gen = (void*)gen;
  }

  m_rand_ptr = m_rand_ptr_v.data();
}
