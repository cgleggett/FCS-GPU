/*
  Copyright (C) 2002-2018 CERN for the benefit of the ATLAS collaboration
*/

#ifndef TFCSHistoLateralShapeParametrization_h
#define TFCSHistoLateralShapeParametrization_h

#include "ISF_FastCaloSimEvent/TFCSLateralShapeParametrizationHitBase.h"
#include "ISF_FastCaloSimEvent/TFCS2DFunctionHistogram.h"
#include "ISF_FastCaloSimEvent/TFCSTruthState.h"

#ifdef USE_GPU
#  include "FastCaloGpu/FastCaloGpu/LoadGpuFuncHist.h"
#endif

class TH2;

class TFCSHistoLateralShapeParametrization : public TFCSLateralShapeParametrizationHitBase {
public:
  TFCSHistoLateralShapeParametrization( const char* name = nullptr, const char* title = nullptr );
  ~TFCSHistoLateralShapeParametrization();

  /// Status bit for FCS needs
  enum FCSStatusBits {
    k_phi_symmetric = BIT( 15 ) ///< Set this bit to simulate phi symmetric histograms
  };

  bool         is_phi_symmetric() const { return TestBit( k_phi_symmetric ); };
  virtual void set_phi_symmetric() { SetBit( k_phi_symmetric ); };
  virtual void reset_phi_symmetric() { ResetBit( k_phi_symmetric ); };

  /// set the integral of the histogram to the desired number of hits
  void set_number_of_hits( float nhits );

  float get_number_of_expected_hits() const { return m_nhits; };

  /// default for this class is to simulate poisson(integral histogram) hits
  int get_number_of_hits(TFCSSimulationState& simulstate,const TFCSTruthState* truth, const TFCSExtrapolationState* extrapol) const override;

  /// simulated one hit position with weight that should be put into simulstate
  /// sometime later all hit weights should be resacled such that their final sum is simulstate->E(sample)
  /// someone also needs to map all hits into cells
  virtual FCSReturnCode simulate_hit(Hit& hit,TFCSSimulationState& simulstate,const TFCSTruthState* truth, const TFCSExtrapolationState* extrapol) override;

  /// Init from histogram. The integral of the histogram is used as number of expected hits to be generated
  bool Initialize( TH2* hist );
  bool Initialize( const char* filepath, const char* histname );

  TFCS2DFunctionHistogram&       histogram() { return m_hist; };
  const TFCS2DFunctionHistogram& histogram() const { return m_hist; };

#ifdef USE_GPU
  void             set_d_HistFunc( FH2D* hf_ptr ) { m_d_HistFunc = hf_ptr; };
  const FH2D*      d_HistFunc() { return m_d_HistFunc; };
  LoadGpuFuncHist* LdFH() { return m_LdFH; };
  void             LoadHistFuncs();
#endif


  void Print(Option_t *option = "") const override;
protected:
  /// Histogram to be used for the shape simulation
  TFCS2DFunctionHistogram m_hist;
  float                   m_nhits;

#ifdef USE_GPU
  FH2D*            m_d_HistFunc = nullptr;
  LoadGpuFuncHist* m_LdFH       = nullptr;

#endif

private:
  ClassDefOverride( TFCSHistoLateralShapeParametrization, 1 ) // TFCSHistoLateralShapeParametrization
};

#if defined( __ROOTCLING__ ) && defined( __FastCaloSimStandAlone__ )
#  pragma link C++ class TFCSHistoLateralShapeParametrization + ;
#endif

#endif
