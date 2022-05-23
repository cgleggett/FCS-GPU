/*
  Copyright (C) 2002-2021 CERN for the benefit of the ATLAS collaboration
*/

#include "CaloGpuGeneral_omp.h"
#include "GeoRegion.h"
#include "GeoGpu_structs.h"
#include "Hit.h"
#include "Rand4Hits.h"

//#include "gpuQ.h"
#include "Args.h"
#include <chrono>
#include <iostream>
#include <omp.h>

#ifdef USE_KOKKOS
#include <Kokkos_Core.hpp>
#include <Kokkos_Random.hpp>
#endif

#define BLOCK_SIZE 256

#define M_PI 3.14159265358979323846
#define M_2PI 6.28318530717958647692

using namespace CaloGpuGeneral_fnc;

namespace CaloGpuGeneral_omp {

  void simulate_A( float E, int nhits, Chain0_Args& args ) {

    long t;
    const unsigned long ncells = args.ncells;
   
    #pragma omp target //map(args.cells_energy[:ncells]) //TODO: runtime error when mapping cells_energy 
    {
      #pragma omp teams distribute parallel for
      for ( t = 0; t < nhits; t++ ) {
        Hit hit;
        hit.E() = E;
        CenterPositionCalculation_d( hit, args );
        HistoLateralShapeParametrization_d( hit, t, args );
        HitCellMappingWiggle_d( hit, args, t );
      }
    }

  }

  void simulate_ct( Chain0_Args& args ) {

    unsigned long tid;
    const unsigned long ncells = args.ncells;

    #pragma omp target //TODO: map args.hitcells_E[ct]
    {
      #pragma omp teams distribute parallel for
      for ( tid = 0; tid < ncells; tid++ ) {
        if ( args.cells_energy[tid] > 0 ) {
          //unsigned int ct = atomicAdd( args.hitcells_ct, 1 );
          unsigned int ct     = args.hitcells_ct[0];
          Cell_E                ce;
          ce.cellid           = tid;
          ce.energy           = args.cells_energy[tid];
          args.hitcells_E[ct] = ce;
          #pragma omp atomic update
            args.hitcells_ct[0]++;
        }
      }
    }

  }

  void simulate_clean( Chain0_Args& args ) {
 
    int tid; 
    unsigned long ncells = args.ncells;
    //std::cout << "ncells = " << args.ncells << std::endl;  
    /*TODO: Where is args.cells_energy allocated? */
    
    #pragma omp target map(args.hitcells_ct[:1]) map(args.cells_energy[:ncells]) 
    {
      #pragma omp teams distribute parallel for
      for(tid = 0; tid < ncells; tid++) {

        args.cells_energy[tid] = 0.0;
        if ( tid == 0 ) args.hitcells_ct[0] = 0;
      }
    }

  }

  void simulate_hits( float E, int nhits, Chain0_Args& args ) {

    int m_default_device = omp_get_default_device();
    int m_initial_device = omp_get_initial_device();
    std::size_t m_offset = 0;

    simulate_clean ( args );

    simulate_A ( E, nhits, args );

    //  cudaDeviceSynchronize() ;
    //  err = cudaGetLastError();
    // if (err != cudaSuccess) {
    //        std::cout<< "simulate_A "<<cudaGetErrorString(err)<< std::endl;
    //}

    simulate_ct ( args );

    int ct;
    if ( omp_target_memcpy( &ct, args.hitcells_ct, sizeof( int ),
                                    m_offset, m_offset, m_initial_device, m_default_device ) ) { 
      std::cout << "ERROR: copy hitcells_ct. " << std::endl;
    } 
//    gpuQ( cudaMemcpy( &ct, args.hitcells_ct, sizeof( int ), cudaMemcpyDeviceToHost ) );
    if ( omp_target_memcpy( args.hitcells_E_h, args.hitcells_E, ct * sizeof( Cell_E ),
                                    m_offset, m_offset, m_initial_device, m_default_device ) ) { 
      std::cout << "ERROR: copy hitcells_E_h. " << std::endl;
    } 
//    gpuQ( cudaMemcpy( args.hitcells_E_h, args.hitcells_E, ct * sizeof( Cell_E ), cudaMemcpyDeviceToHost ) );


    // pass result back
    args.ct = ct;
    //   args.hitcells_ct_h=hitcells_ct ;
  }

} // namespace CaloGpuGeneral_omp