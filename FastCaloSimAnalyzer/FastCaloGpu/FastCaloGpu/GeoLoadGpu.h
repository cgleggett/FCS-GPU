/*
  Copyright (C) 2002-2021 CERN for the benefit of the ATLAS collaboration
*/

#ifndef GeoLoadGpu_H
#define GeoLoadGpu_H

// This header can be use both gcc and nvcc host part

#include <map>
#include <vector>

#include "GeoGpu_structs.h"

#ifdef USE_KOKKOS
#  include <Kokkos_Core.hpp>
#  include <Kokkos_Random.hpp>
#endif

typedef std::map<Identifier, const CaloDetDescrElement*> t_cellmap;

class GeoLoadGpu {
public:
  GeoLoadGpu() = default;

  ~GeoLoadGpu() {
    delete m_cellid_array;
#ifdef USE_KOKKOS
    for ( auto p : m_reg_vec ) { delete p; }
#endif
  }

  void                       set_ncells( unsigned long nc ) { m_ncells = nc; };
  void                       set_nregions( unsigned int nr ) { m_nregions = nr; };
  void                       set_cellmap( t_cellmap* cm ) { m_cells = cm; };
  void                       set_regions( GeoRegion* r ) { m_regions = r; };
  void                       set_g_regions( GeoRegion* gr ) { m_regions_d = gr; };
  void                       set_cells_g( CaloDetDescrElement* gc ) { m_cells_d = gc; };
  void                       set_max_sample( int s ) { m_max_sample = s; };
  void                       set_sample_index_h( Rg_Sample_Index* s ) { m_sample_index_h = s; };
  const CaloDetDescrElement* index2cell( unsigned long index ) { return ( *m_cells )[m_cellid_array[index]]; };

  bool LoadGpu() {
#ifdef USE_KOKKOS
    return LoadGpu_kk();
#else
    return LoadGpu_cu();
#endif
  }
  bool LoadGpu_kk();
  bool LoadGpu_cu();

  void    set_geoPtr( GeoGpu* ptr ) { m_geo_d = ptr; }
  GeoGpu* get_geoPtr() const { return m_geo_d; }

  unsigned long get_ncells() const { return m_ncells; }

  // bool LoadGpu_Region(GeoRegion * ) ;

private:
  bool TestGeo();
  bool SanityCheck();

protected:
  unsigned long        m_ncells{0};         // number of cells
  unsigned int         m_nregions{0};       // number of regions
  t_cellmap*           m_cells{0};          // from Geometry class
  GeoRegion*           m_regions{0};        // array of regions on host
  GeoRegion*           m_regions_d{0};      // array of region on GPU
  CaloDetDescrElement* m_cells_d{0};        // Cells in GPU
  Identifier*          m_cellid_array{0};   // cell id to Indentifier lookup table
  int                  m_max_sample{0};     // Max number of samples
  Rg_Sample_Index*     m_sample_index_h{0}; // index for flatout of  GeoLookup over sample

  GeoGpu* m_geo_d{0};

#ifdef USE_KOKKOS
  Kokkos::View<CaloDetDescrElement*> m_cells_vd;
  Kokkos::View<GeoRegion*>           m_regions_vd;
  Kokkos::View<Rg_Sample_Index*>     m_sample_index_vd;
  Kokkos::View<GeoGpu>               m_gptr_v{"geometry dev ptr"};

  std::vector<Kokkos::View<long long*>*> m_reg_vec;
#endif
};
#endif
