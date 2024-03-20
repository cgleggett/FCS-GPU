/*
  Copyright (C) 2002-2021 CERN for the benefit of the ATLAS collaboration
*/
#include "gpuQ.h"
#include "Rand4Hits.h"
#include "DEV_BigMem.h"

#include <omp.h>
#include <cuda_runtime_api.h>
#include <curand.h>

#include "GpuParams.h"
#include "Rand4Hits_cpu.cxx"

#define CURAND_CALL( x )                                                                                               \
  if ( ( x ) != CURAND_STATUS_SUCCESS ) {                                                                              \
    printf( "Error at %s:%d\n", __FILE__, __LINE__ );                                                                  \
    exit( EXIT_FAILURE );                                                                                              \
  }

void Rand4Hits::allocate_simulation( int maxbins, int maxhitct, unsigned long n_cells ) {

  int m_default_device = omp_get_default_device();
  int m_initial_device = omp_get_initial_device();

  CELL_ENE_T* Cells_Energy;
  Cells_Energy   = (float*)omp_target_alloc( MAX_SIM * n_cells * sizeof( CELL_ENE_T ), m_default_device );
  m_cells_energy = Cells_Energy;

  Cell_E* cell_e;
  cell_e     = (Cell_E*)omp_target_alloc( MAX_SIM * maxhitct * sizeof( Cell_E ), m_default_device );
  m_cell_e   = cell_e;
  m_cell_e_h = (Cell_E*)malloc( MAX_SIM * maxhitct * sizeof( Cell_E ) );

  long* simbins;
  simbins   = (long*)omp_target_alloc( MAX_SIMBINS * sizeof( long ), m_default_device );
  m_simbins = simbins;

  HitParams* hitparams;
  hitparams   = (HitParams*)omp_target_alloc( MAX_SIMBINS * sizeof( HitParams ), m_default_device );
  m_hitparams = hitparams;

  int* ct_ptr;
  ct_ptr = (int*)omp_target_alloc( MAX_SIM * sizeof( int ), m_default_device );
  m_ct   = ct_ptr;
  m_ct_h = (int*)malloc( MAX_SIM * sizeof( int ) );

  DEV_BigMem* bm     = new DEV_BigMem( M_SEG_SIZE );
  DEV_BigMem::bm_ptr = bm;

  printf( " -- R4H ncells: %lu  cells_energy: %p   hitcells_E: %p  hitcells_ct: "
          "%p\n",
          n_cells, (void*)m_cells_energy, (void*)m_cell_e, (void*)m_ct );
}

void Rand4Hits::allocateGenMem( size_t num ) {
  m_rnd_cpu = new std::vector<float>;
  m_rnd_cpu->resize( num );
  std::cout << "m_rnd_cpu: " << m_rnd_cpu << "  " << m_rnd_cpu->data() << std::endl;
}

Rand4Hits::~Rand4Hits() {

  delete ( m_rnd_cpu );
  if ( DEV_BigMem::bm_ptr ) {
    std::cout << "BigMem allocated: " << DEV_BigMem::bm_ptr->size() << "  used: " << DEV_BigMem::bm_ptr->used()
              << "  lost: " << DEV_BigMem::bm_ptr->lost() << std::endl;
    delete DEV_BigMem::bm_ptr;
  }
  omp_target_free( m_rand_ptr, m_select_device );
  if ( m_useCPU ) {
    destroyCPUGen();
  } else {
    CURAND_CALL( curandDestroyGenerator( *( (curandGenerator_t*)m_gen ) ) );
    delete (curandGenerator_t*)m_gen;
  }
};

void Rand4Hits::rd_regen() {
  if ( m_useCPU ) {
    genCPU( 3 * m_total_a_hits );
    if ( omp_target_memcpy( m_rand_ptr, m_rnd_cpu->data(), 3 * m_total_a_hits * sizeof( float ), m_offset, m_offset,
                            m_select_device, m_initial_device ) ) {
      std::cout << "ERROR: copy random numbers from cpu to gpu " << std::endl;
    }
  } else {
    CURAND_CALL( curandGenerateUniform( *( (curandGenerator_t*)m_gen ), m_rand_ptr, 3 * m_total_a_hits ) );
  }
};

void Rand4Hits::create_gen( unsigned long long seed, size_t num, bool useCPU ) {

  float* f{ nullptr };

  m_useCPU = useCPU;

  if ( m_useCPU ) {
    allocateGenMem( num );
    createCPUGen( seed );
    genCPU( num );
    f = (float*)omp_target_alloc( num * sizeof( float ), m_select_device );
    if ( f == NULL ) { std::cout << " ERROR: No space left on device." << std::endl; }
    if ( omp_target_memcpy( f, m_rnd_cpu->data(), num * sizeof( float ), m_offset, m_offset, m_select_device,
                            m_initial_device ) ) {
      std::cout << "ERROR: copy random numbers from cpu to gpu " << std::endl;
    }
  } else {
    gpuQ( cudaMalloc( &f, num * sizeof( float ) ) );
    curandGenerator_t* gen = new curandGenerator_t;
    CURAND_CALL( curandCreateGenerator( gen, CURAND_RNG_PSEUDO_DEFAULT ) );
    CURAND_CALL( curandSetPseudoRandomGeneratorSeed( *gen, seed ) );
    CURAND_CALL( curandGenerateUniform( *gen, f, num ) );
    m_gen = (void*)gen;
  }

  m_rand_ptr = f;

  std::cout << "R4H m_rand_ptr: " << m_rand_ptr << std::endl;
}
