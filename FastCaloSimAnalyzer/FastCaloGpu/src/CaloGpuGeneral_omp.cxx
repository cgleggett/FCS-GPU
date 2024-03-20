/*
  Copyright (C) 2002-2021 CERN for the benefit of the ATLAS collaboration
*/

#include "CaloGpuGeneral_omp.h"
#include "GeoRegion.h"
#include "GeoGpu_structs.h"
#include "Hit.h"
#include "Rand4Hits.h"

#include "gpuQ.h"
#include "Args.h"
#include "DEV_BigMem.h"
// #include "OMP_BigMem.h"
#include <chrono>
#include <mutex>
#include <climits>

#include <cuda_runtime_api.h>
#include <curand.h>
#include <iostream>
#include <omp.h>

#define DEFAULT_BLOCK_SIZE 256

#define M_PI 3.14159265358979323846
#define M_2PI 6.28318530717958647692

static std::once_flag calledGetEnv{};
static int            BLOCK_SIZE{ DEFAULT_BLOCK_SIZE };

static int count{ 0 };

static CaloGpuGeneral::KernelTime timing;

namespace CaloGpuGeneral_omp {

  void Rand4Hits_finish( void* rd4h ) {

    size_t free, total;
    // gpuQ(cudaMemGetInfo(&free, &total));
    std::cout << "TODO GPU memory used(MB): " << ( total - free ) / 1000000 << std::endl;
    if ( (Rand4Hits*)rd4h ) delete (Rand4Hits*)rd4h;

    if ( timing.count > 0 ) {
      std::cout << "kernel timing\n";
      std::cout << timing;
      // std::cout << "\n\n\n";
      // timing.printAll();
    } else {
      std::cout << "no kernel timing available" << std::endl;
    }
  }

  inline long long getDDE( GeoGpu* geo, int sampling, float eta, float phi ) {

    float* distance = 0;
    int*   steps    = 0;

    int              MAX_SAMPLING = geo->max_sample;
    Rg_Sample_Index* SampleIdx    = geo->sample_index;
    GeoRegion*       regions_g    = geo->regions;

    if ( sampling < 0 ) return -1;
    if ( sampling >= MAX_SAMPLING ) return -1;

    int sample_size  = SampleIdx[sampling].size;
    int sample_index = SampleIdx[sampling].index;

    GeoRegion* gr = (GeoRegion*)regions_g;
    if ( sample_size == 0 ) return -1;
    float     dist;
    long long bestDDE = -1;
    if ( !distance ) distance = &dist;
    *distance = +10000000;
    int intsteps;
    int beststeps;
    if ( steps )
      beststeps = ( *steps );
    else
      beststeps = 0;

    if ( sampling < 21 ) {
      for ( int skip_range_check = 0; skip_range_check <= 1; ++skip_range_check ) {
        for ( unsigned int j = sample_index; j < sample_index + sample_size; ++j ) {
          if ( !skip_range_check ) {
            if ( eta < gr[j].mineta() ) continue;
            if ( eta > gr[j].maxeta() ) continue;
          }
          if ( steps )
            intsteps = ( *steps );
          else
            intsteps = 0;
          float     newdist;
          long long newDDE = gr[j].getDDE( eta, phi, &newdist, &intsteps );
          if ( newdist < *distance ) {
            bestDDE   = newDDE;
            *distance = newdist;
            if ( steps ) beststeps = intsteps;
            if ( newdist < -0.1 ) break; // stop, we are well within the hit cell
          }
        }
        if ( bestDDE >= 0 ) break;
      }
    } else {
      return -3;
    }
    if ( steps ) *steps = beststeps;

    return bestDDE;
  }

  inline int find_index_f( float* array, int size, float value ) {
    // fist index (from 0)  have element value > value
    // array[i] > value ; array[i-1] <= value
    // std::upbund( )
    int low     = 0;
    int high    = size - 1;
    int m_index = ( high - low ) / 2;
    while ( m_index != high ) {
      if ( value < array[m_index] )
        high = m_index;
      else
        low = m_index + 1;
      m_index = ( high + low + 1 ) / 2;
    }
    return m_index;
  }

  inline int find_index_uint32( uint32_t* array, int size, uint32_t value ) {
    // fist index i  have element value > value
    // array[i] > value ; array[i-1] <= value
    int low     = 0;
    int high    = size - 1;
    int m_index = ( high - low ) / 2;
    while ( m_index != high ) {
      if ( value < array[m_index] )
        high = m_index;
      else
        low = m_index + 1;
      m_index = ( high + low + 1 ) / 2;
    }
    return m_index;
  }

  inline int find_index_long( long* array, int size, long value ) {
    // find the first index of element which has vaule > value
    int low     = 0;
    int high    = size - 1;
    int m_index = ( high - low ) / 2;
    while ( high != low ) {
      if ( value > array[m_index] )
        low = m_index + 1;
      else if ( value == array[m_index] ) {
        return m_index + 1;
        // return min(m_index + 1, size-1)   ;
      } else
        high = m_index;
      m_index = ( high - low ) / 2 + low;
    }
    return m_index;
  }

  inline void rnd_to_fct2d( float& valuex, float& valuey, float rnd0, float rnd1, FH2D* hf2d ) {

    // printf("-r-r-r-r- RN at %f %f \n", rnd0, rnd1);

    int    nbinsx        = ( *hf2d ).nbinsx;
    int    nbinsy        = ( *hf2d ).nbinsy;
    float* HistoContents = ( *hf2d ).h_contents;
    float* HistoBorders  = ( *hf2d ).h_bordersx;
    float* HistoBordersy = ( *hf2d ).h_bordersy;

    /*
    int ibin = nbinsx*nbinsy-1 ;
    for ( int i=0 ; i < nbinsx*nbinsy ; ++i) {
       if   (HistoContents[i]> rnd0 ) {
            ibin = i ;
            break ;
           }
    }
   */
    int ibin = find_index_f( HistoContents, nbinsx * nbinsy, rnd0 );

    int biny = ibin / nbinsx;
    int binx = ibin - nbinsx * biny;

    float basecont = 0;
    if ( ibin > 0 ) basecont = HistoContents[ibin - 1];

    float dcont = HistoContents[ibin] - basecont;
    if ( dcont > 0 ) {
      valuex = HistoBorders[binx] + ( HistoBorders[binx + 1] - HistoBorders[binx] ) * ( rnd0 - basecont ) / dcont;
    } else {
      valuex = HistoBorders[binx] + ( HistoBorders[binx + 1] - HistoBorders[binx] ) / 2;
    }
    valuey = HistoBordersy[biny] + ( HistoBordersy[biny + 1] - HistoBordersy[biny] ) * rnd1;
  }

  inline float rnd_to_fct1d( float rnd, uint32_t* contents, float* borders, int nbins, uint32_t s_MaxValue ) {

    uint32_t int_rnd = s_MaxValue * rnd;
    /*
      int  ibin=nbins-1 ;
      for ( int i=0 ; i < nbins ; ++i) {
        if   (contents[i]> int_rnd ) {
             ibin = i ;
             break ;
            }
      }
    */
    int ibin = find_index_uint32( contents, nbins, int_rnd );

    int binx = ibin;

    uint32_t basecont = 0;
    if ( ibin > 0 ) basecont = contents[ibin - 1];

    uint32_t dcont = contents[ibin] - basecont;
    if ( dcont > 0 ) {
      return borders[binx] + ( ( borders[binx + 1] - borders[binx] ) * ( int_rnd - basecont ) ) / dcont;
    } else {
      return borders[binx] + ( borders[binx + 1] - borders[binx] ) / 2;
    }
  }

  void load_hitsim_params( void* rd4h, HitParams* hp, long* simbins, int bins ) {

    int         m_default_device = omp_get_default_device();
    int         m_initial_device = omp_get_initial_device();
    std::size_t m_offset         = 0;

    if ( !(Rand4Hits*)rd4h ) {
      std::cout << "Error load hit simulation params ! ";
      exit( 2 );
    }

    HitParams* hp_g      = ( (Rand4Hits*)rd4h )->get_hitparams();
    long*      simbins_g = ( (Rand4Hits*)rd4h )->get_simbins();

    // gpuQ(cudaMemcpy(hp_g, hp, bins*sizeof(HitParams), cudaMemcpyHostToDevice));
    if ( omp_target_memcpy( hp_g, hp, bins * sizeof( HitParams ), m_offset, m_offset, m_default_device,
                            m_initial_device ) ) {
      std::cout << "ERROR: copy hp hp_g. " << std::endl;
    }
    // gpuQ(cudaMemcpy(simbins_g, simbins, bins*sizeof(long), cudaMemcpyHostToDevice));
    if ( omp_target_memcpy( simbins_g, simbins, bins * sizeof( long ), m_offset, m_offset, m_default_device,
                            m_initial_device ) ) {
      std::cout << "ERROR: copy simbins simbins_g. " << std::endl;
    }
  }

  inline void simulate_clean( Sim_Args& args ) {

    auto                cells_energy = args.cells_energy;
    auto                ct           = args.ct;
    const auto          nsims        = args.nsims;
    const unsigned long ncellssims   = args.ncells * nsims;

    int tid;
#pragma omp target           is_device_ptr( cells_energy, ct )      // map(to:nsims) //nowait
#pragma omp teams distribute parallel for num_threads( BLOCK_SIZE ) // num_teams(GRID_SIZE) // num_teams default 1467,
                                                                    // threads default 128
    for ( tid = 0; tid < ncellssims; tid++ ) {
      // printf(" num teams = %d, num threads = %d", omp_get_num_teams(), omp_get_num_threads() );
      cells_energy[tid] = 0.;
      if ( tid < nsims ) ct[tid] = 0;
    }
  }

  inline int highestPowerof2( unsigned int n ) {
    // Invalid input
    if ( n < 1 ) return 0;

    int res = 1;

    // Try all powers starting from 2^1
    for ( unsigned int i = 0; i < 8 * sizeof( unsigned int ); i++ ) {
      unsigned int curr = 1 << i;

      // If current power is more than n, break
      if ( curr > n ) break;

      res = curr;
    }

    return res;
  }

  // inline  void CenterPositionCalculation_g_d(const HitParams hp, Hit& hit, const Sim_Args args) {
  //
  //   hit.setCenter_r( ( 1. - hp.extrapWeight ) * hp.extrapol_r_ent + hp.extrapWeight * hp.extrapol_r_ext );
  //   hit.setCenter_z( ( 1. - hp.extrapWeight ) * hp.extrapol_z_ent + hp.extrapWeight * hp.extrapol_z_ext );
  //   hit.setCenter_eta( ( 1. - hp.extrapWeight ) * hp.extrapol_eta_ent + hp.extrapWeight * hp.extrapol_eta_ext );
  //   hit.setCenter_phi( ( 1. - hp.extrapWeight ) * hp.extrapol_phi_ent + hp.extrapWeight * hp.extrapol_phi_ext );
  // }

  // inline void HistoLateralShapeParametrization_g_d( const HitParams hp, Hit& hit, int t , Sim_Args args ) {
  //
  //   float  charge   = hp.charge;
  //
  //   float center_eta = hit.center_eta();
  //   float center_phi = hit.center_phi();
  //   float center_r   = hit.center_r();
  //   float center_z   = hit.center_z();
  //
  //
  //   float alpha, r, rnd1, rnd2;
  //   rnd1 = args.rand[t];
  //   rnd2 = args.rand[t+args.nhits];
  //
  //   if(hp.is_phi_symmetric) {
  //     if(rnd2>=0.5) { //Fill negative phi half of shape
  //       rnd2-=0.5;
  //       rnd2*=2;
  //       rnd_to_fct2d(alpha,r,rnd1,rnd2,hp.f2d);
  //       alpha=-alpha;
  //     } else { //Fill positive phi half of shape
  //       rnd2*=2;
  //       rnd_to_fct2d(alpha,r,rnd1,rnd2,hp.f2d);
  //     }
  //   } else {
  //     rnd_to_fct2d(alpha,r,rnd1,rnd2, hp.f2d);
  //   }
  //
  //
  //   float delta_eta_mm = r * cos(alpha);
  //   float delta_phi_mm = r * sin(alpha);
  //
  //   // Particles with negative eta are expected to have the same shape as those with positive eta after
  //   transformation:
  //   // delta_eta --> -delta_eta
  //   if(center_eta<0.)delta_eta_mm = -delta_eta_mm;
  //   // Particle with negative charge are expected to have the same shape as positively charged particles after
  //   // transformation: delta_phi --> -delta_phi
  //   if(charge < 0.)  delta_phi_mm = -delta_phi_mm;
  //
  //   float dist000    = sqrt(center_r * center_r + center_z * center_z);
  //   float eta_jakobi = abs(2.0 * exp(-center_eta) / (1.0 + exp(-2 * center_eta)));
  //
  //   float delta_eta = delta_eta_mm / eta_jakobi / dist000;
  //   float delta_phi = delta_phi_mm / center_r;
  //
  //   hit.setEtaPhiZE(center_eta + delta_eta,center_phi + delta_phi,center_z, hit.E());
  //
  //
  // }

  // inline void HitCellMapping_g_d( HitParams hp,Hit& hit,  Sim_Args args, float* cells_energy ) {
  //
  //  long long  cellele= getDDE(args.geo, hp.cs,hit.eta(),hit.phi());
  //
  ////if (hp.index ==0 ) printf("Tid: %d cellId: %ld  nhits: %ld \n" , threadIdx.x ,cellele, hp.nhits ) ;
  //
  // if( cellele < 0) printf("cellele not found %ld \n", cellele ) ;
  // //if( cellele >= 0 )  atomicAdd(&args.cells_energy[cellele+args.ncells*hp.index], hit.E()) ;
  // #pragma omp atomic update
  // cells_energy[cellele + args.ncells*hp.index] += (float)(hit.E());
  //
  //}

  // inline void HitCellMappingWiggle_g_d( HitParams hp,Hit& hit, long t,  Sim_Args args, float* cells_energy  ) {
  //
  //  FHs * f1d = hp.f1d ;
  //  int nhist=(*f1d).nhist;
  //  float*  bin_low_edge = (*f1d ).low_edge ;
  //
  //  float eta =fabs( hit.eta());
  //   if ( eta < bin_low_edge[0] || eta > bin_low_edge[nhist] ) { HitCellMapping_g_d( hp, hit, args, cells_energy ); }
  //
  //  int bin= nhist ;
  //   for (int i =0; i< nhist+1 ; ++i ) {
  //  	if(bin_low_edge[i] > eta ) {
  //	  bin = i ;
  //	  break ;
  //	}
  //   }
  //
  ////  bin=find_index_f(bin_low_edge, nhist+1, eta ) ;
  //
  //  bin -= 1;
  //
  //  uint32_t * contents = (*f1d).h_contents[bin] ;
  //  float* borders = (*f1d).h_borders[bin] ;
  //  int h_size=(*f1d).h_szs[bin] ;
  //  uint32_t s_MaxValue =(*f1d).s_MaxValue ;
  //
  //
  //     float rnd= args.rand[t+2*args.nhits];
  //
  //    float wiggle=rnd_to_fct1d(rnd,contents, borders, h_size, s_MaxValue);
  //
  //    float hit_phi_shifted=hit.phi()+wiggle;
  //    hit.phi()=Phi_mpi_pi(hit_phi_shifted);
  //
  //
  ////  HitCellMapping_g_d(hp, hit,  args) ;
  //
  //}

  inline void simulate_hits_de( const Sim_Args args ) {

    const unsigned long ncells = args.ncells;

    auto cells_energy = args.cells_energy;
    auto ct           = args.ct;
    auto rand         = args.rand;
    auto geo          = args.geo;
    auto nhits        = args.nhits;

    int m_default_device = omp_get_default_device();
    int m_initial_device = omp_get_initial_device();

    /************* A **********/

    long t;
#pragma omp target           is_device_ptr( cells_energy, rand, geo ) map( to : args )
#pragma omp teams distribute parallel for num_threads( BLOCK_SIZE ) // num_teams default 33
    for ( t = 0; t < nhits; t++ ) {

      Hit       hit;
      int       bin = find_index_long( args.simbins, args.nbins, t );
      HitParams hp  = args.hitparams[bin];
      hit.E()       = hp.E;
      // if(bin<3 and t<3) printf("OMP1 bin %d hp.index %d \n",bin, hp.index);

      // CenterPositionCalculation_g_d( hp, hit, args) ;
      hit.setCenter_r( ( 1. - hp.extrapWeight ) * hp.extrapol_r_ent + hp.extrapWeight * hp.extrapol_r_ext );
      hit.setCenter_z( ( 1. - hp.extrapWeight ) * hp.extrapol_z_ent + hp.extrapWeight * hp.extrapol_z_ext );
      hit.setCenter_eta( ( 1. - hp.extrapWeight ) * hp.extrapol_eta_ent + hp.extrapWeight * hp.extrapol_eta_ext );
      hit.setCenter_phi( ( 1. - hp.extrapWeight ) * hp.extrapol_phi_ent + hp.extrapWeight * hp.extrapol_phi_ext );

      // HistoLateralShapeParametrization_g_d(hp, hit, t, args) ;
      float charge     = hp.charge;
      float center_eta = hit.center_eta();
      float center_phi = hit.center_phi();
      float center_r   = hit.center_r();
      float center_z   = hit.center_z();
      float alpha, r, rnd1, rnd2;
      rnd1 = args.rand[t];
      rnd2 = args.rand[t + args.nhits];
      if ( hp.is_phi_symmetric ) {
        if ( rnd2 >= 0.5 ) { // Fill negative phi half of shape
          rnd2 -= 0.5;
          rnd2 *= 2;
          rnd_to_fct2d( alpha, r, rnd1, rnd2, hp.f2d );
          alpha = -alpha;
        } else { // Fill positive phi half of shape
          rnd2 *= 2;
          rnd_to_fct2d( alpha, r, rnd1, rnd2, hp.f2d );
        }
      } else {
        rnd_to_fct2d( alpha, r, rnd1, rnd2, hp.f2d );
      }
      float delta_eta_mm = r * cos( alpha );
      float delta_phi_mm = r * sin( alpha );
      // Particles with negative eta are expected to have the same shape as those with positive eta after
      // transformation: delta_eta --> -delta_eta
      if ( center_eta < 0. ) delta_eta_mm = -delta_eta_mm;
      // Particle with negative charge are expected to have the same shape as positively charged particles after
      // transformation: delta_phi --> -delta_phi
      if ( charge < 0. ) delta_phi_mm = -delta_phi_mm;
      float dist000    = sqrt( center_r * center_r + center_z * center_z );
      float eta_jakobi = abs( 2.0 * exp( -center_eta ) / ( 1.0 + exp( -2 * center_eta ) ) );
      float delta_eta  = delta_eta_mm / eta_jakobi / dist000;
      float delta_phi  = delta_phi_mm / center_r;
      hit.setEtaPhiZE( center_eta + delta_eta, center_phi + delta_phi, center_z, hit.E() );

      // if(bin<3 and t<3) printf("OMP2 tid=%ld args.nbins=%d energy=%f hp.index %d \n", t, args.nbins,hit.E(),
      // hp.index) ;

      // if( hp.cmw)HitCellMappingWiggle_g_d ( hp, hit, t, args, cells_energy ) ;
      if ( hp.cmw ) {
        FHs*   f1d          = hp.f1d;
        int    nhist        = ( *f1d ).nhist;
        float* bin_low_edge = ( *f1d ).low_edge;

        float eta = fabs( hit.eta() );
        // if ( eta < bin_low_edge[0] || eta > bin_low_edge[nhist] ) { HitCellMapping_g_d( hp, hit, args, cells_energy
        // ); }
        if ( eta < bin_low_edge[0] || eta > bin_low_edge[nhist] ) {
          long long cellele = getDDE( args.geo, hp.cs, hit.eta(), hit.phi() );
          if ( cellele < 0 ) printf( "cellele not found %ld \n", cellele );
#pragma omp atomic update
          cells_energy[cellele + args.ncells * hp.index] += (float)( hit.E() );
        }

        int bin = nhist;
        for ( int i = 0; i < nhist + 1; ++i ) {
          if ( bin_low_edge[i] > eta ) {
            bin = i;
            break;
          }
        }

        //  bin=find_index_f(bin_low_edge, nhist+1, eta ) ;

        bin -= 1;

        uint32_t* contents   = ( *f1d ).h_contents[bin];
        float*    borders    = ( *f1d ).h_borders[bin];
        int       h_size     = ( *f1d ).h_szs[bin];
        uint32_t  s_MaxValue = ( *f1d ).s_MaxValue;

        float rnd = args.rand[t + 2 * args.nhits];

        float wiggle = rnd_to_fct1d( rnd, contents, borders, h_size, s_MaxValue );

        float hit_phi_shifted = hit.phi() + wiggle;
        hit.phi()             = Phi_mpi_pi( hit_phi_shifted );
      }

      // HitCellMapping_g_d(hp, hit, args, cells_energy) ;
      long long cellele = getDDE( args.geo, hp.cs, hit.eta(), hit.phi() );
      if ( cellele < 0 ) printf( "cellele not found %ld \n", cellele );
#pragma omp atomic update
      cells_energy[cellele + ncells * hp.index] += hit.E();
      // if(bin<3 and t<3) printf("OMP3 energy %f cellele=%ld ncell=%d hp.index=%d \n", hit.E(), cellele, ncells,
      // hp.index);
    }
  }

  inline void simulate_hits_ct( const Sim_Args args ) {

    const unsigned long ncells = args.ncells;
    const unsigned long nsims  = args.nsims;

    auto cells_energy = args.cells_energy;
    auto argsct       = args.ct;
    auto hitcells_E   = args.hitcells_E;

#pragma omp target           is_device_ptr( cells_energy, argsct, hitcells_E ) // nowait
#pragma omp teams distribute parallel for num_threads( BLOCK_SIZE ) // num_teams(GRID_SIZE) //thread_limit(128)
                                                                    // //num_teams default 1467, threads default 128
    for ( int tid = 0; tid < ncells * nsims; tid++ ) {
      unsigned long cellid = tid % ncells;
      int           sim    = tid / ncells;
      if ( cells_energy[tid] > 0. ) {
        unsigned int ct = 0;
#pragma omp atomic capture
        ct = argsct[sim]++;

        Cell_E ce;
        ce.cellid                       = cellid;
        ce.energy                       = cells_energy[tid];
        hitcells_E[ct + sim * MAXHITCT] = ce;
        // if(sim==0) printf ( "OMP4 sim: %d ct %d cellid %d energy %f \n", sim, ct, cellid, ce.energy);
      }
    }
  }

  void simulate_hits_gr( Sim_Args& args ) {

    int         m_default_device = omp_get_default_device();
    int         m_initial_device = omp_get_initial_device();
    std::size_t m_offset         = 0;

    std::call_once( calledGetEnv, []() {
      if ( const char* env_p = std::getenv( "FCS_BLOCK_SIZE" ) ) {
        std::string bs( env_p );
        BLOCK_SIZE = std::stoi( bs );
      }
      if ( BLOCK_SIZE != DEFAULT_BLOCK_SIZE ) { std::cout << "kernel BLOCK_SIZE: " << BLOCK_SIZE << std::endl; }
    } );

    int blocksize   = BLOCK_SIZE;
    int threads_tot = args.ncells * args.nsims;
    int nblocks     = ( threads_tot + blocksize - 1 ) / blocksize;

    auto t0 = std::chrono::system_clock::now();
    CaloGpuGeneral_omp::simulate_clean( args );

    blocksize   = BLOCK_SIZE;
    threads_tot = args.nhits;

    auto t1 = std::chrono::system_clock::now();
    nblocks = ( threads_tot + blocksize - 1 ) / blocksize;
    CaloGpuGeneral_omp::simulate_hits_de( args );

    auto t2 = std::chrono::system_clock::now();
    nblocks = ( args.ncells * args.nsims + blocksize - 1 ) / blocksize;
    CaloGpuGeneral_omp::simulate_hits_ct( args );

    // cpy result back

    auto t3 = std::chrono::system_clock::now();
    if ( omp_target_memcpy( args.ct_h, args.ct, args.nsims * sizeof( int ), m_offset, m_offset, m_initial_device,
                            m_default_device ) ) {
      std::cout << "ERROR: copy hitcells_ct. " << std::endl;
    }

    if ( omp_target_memcpy( args.hitcells_E_h, args.hitcells_E, MAXHITCT * MAX_SIM * sizeof( Cell_E ), m_offset,
                            m_offset, m_initial_device, m_default_device ) ) {
      std::cout << "ERROR: copy hitcells_ct. " << std::endl;
    }

    auto t4 = std::chrono::system_clock::now();

#ifdef DUMP_HITCELLS
    std::cout << "nsim: " << args.nsims << "\n";
    for ( int isim = 0; isim < args.nsims; ++isim ) {
      std::cout << "  nhit: " << args.ct_h[isim] << "\n";
      std::map<unsigned int, float> cm;
      for ( int ihit = 0; ihit < args.ct_h[isim]; ++ihit ) {
        cm[args.hitcells_E_h[ihit + isim * MAXHITCT].cellid] = args.hitcells_E_h[ihit + isim * MAXHITCT].energy;
      }

      int i = 0;
      for ( auto& em : cm ) {
        std::cout << "   " << isim << " " << i++ << "  cell: " << em.first << "  " << em.second << std::endl;
      }
    }
#endif

    timing.add( t1 - t0, t2 - t1, t3 - t2, t4 - t3 );
  }

} // namespace CaloGpuGeneral_omp
