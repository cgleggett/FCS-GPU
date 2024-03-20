/*
  Copyright (C) 2002-2018 CERN for the benefit of the ATLAS collaboration
*/

#include "CLHEP/Random/RandFlat.h"
#include "CLHEP/Random/RandPoisson.h"

#include "FastCaloSimAnalyzer/TFCSHistoLateralShapeParametrization.h"
#include "ISF_FastCaloSimEvent/FastCaloSim_CaloCell_ID.h"
#include "ISF_FastCaloSimEvent/TFCSSimulationState.h"
#include "ISF_FastCaloSimEvent/TFCSExtrapolationState.h"

#include "TFile.h"
#include "TMath.h"
#include "TH2.h"

#include "HepPDT/ParticleData.hh"
#include "HepPDT/ParticleDataTable.hh"

//=============================================
//======= TFCSHistoLateralShapeParametrization =========
//=============================================

TFCSHistoLateralShapeParametrization::TFCSHistoLateralShapeParametrization( const char* name, const char* title )
    : TFCSLateralShapeParametrizationHitBase( name, title ), m_nhits( 0 ) {
  reset_phi_symmetric();
}

TFCSHistoLateralShapeParametrization::~TFCSHistoLateralShapeParametrization() {
#ifdef USE_GPU
delete m_LdFH ;
#endif 
}

int TFCSHistoLateralShapeParametrization::get_number_of_hits( TFCSSimulationState& simulstate,
                                                              const TFCSTruthState* /*truth*/,
                                                              const TFCSExtrapolationState* /*extrapol*/ ) const {
  if ( !simulstate.randomEngine() ) { return -1; }

  return CLHEP::RandPoisson::shoot(simulstate.randomEngine(), m_nhits);
}

void TFCSHistoLateralShapeParametrization::set_number_of_hits( float nhits ) { m_nhits = nhits; }

FCSReturnCode TFCSHistoLateralShapeParametrization::simulate_hit( Hit& hit, TFCSSimulationState& simulstate,
                                                                  const TFCSTruthState*         truth,
                                                                  const TFCSExtrapolationState* /*extrapol*/ ) {
  if ( !simulstate.randomEngine() ) { return FCSFatal; }
  
  const int     pdgId    = truth->pdgid();
  const double  charge   = HepPDT::ParticleID(pdgId).charge();

  const int cs=calosample();
  const double center_eta = hit.center_eta(); 
  const double center_phi = hit.center_phi();
  const double center_r   = hit.center_r();
  const double center_z   = hit.center_z();
  
  if ( TMath::IsNaN( center_r ) or TMath::IsNaN( center_z ) or TMath::IsNaN( center_eta ) or
       TMath::IsNaN( center_phi ) ) { // Check if extrapolation fails
    return FCSFatal;
  }

  float alpha, r, rnd1, rnd2;
  rnd1 = CLHEP::RandFlat::shoot(simulstate.randomEngine());
  rnd2 = CLHEP::RandFlat::shoot(simulstate.randomEngine());

  if(is_phi_symmetric()) {
    if(rnd2>=0.5) { //Fill negative phi half of shape
      rnd2-=0.5;
      rnd2*=2;
      m_hist.rnd_to_fct(alpha,r,rnd1,rnd2);
      alpha=-alpha;
    } else { //Fill positive phi half of shape
      rnd2*=2;
      m_hist.rnd_to_fct(alpha,r,rnd1,rnd2);
    }
  } else {
    m_hist.rnd_to_fct(alpha,r,rnd1,rnd2);
  }
  if(TMath::IsNaN(alpha) || TMath::IsNaN(r)) {
    ATH_MSG_ERROR( "  Histogram: " << m_hist.get_HistoBordersx().size() - 1 << "*"
                                   << m_hist.get_HistoBordersy().size() - 1 << " bins, #hits=" << m_nhits
                                   << " alpha=" << alpha << " r=" << r << " rnd1=" << rnd1 << " rnd2=" << rnd2 );
    alpha=0;
    r=0.001;

    ATH_MSG_ERROR("  This error could probably be retried");
    return FCSFatal;
  }
  
  
  float delta_eta_mm = r * cos(alpha);
  float delta_phi_mm = r * sin(alpha);
  
  // Particles with negative eta are expected to have the same shape as those with positive eta after transformation:
  // delta_eta --> -delta_eta
  if(center_eta<0.)delta_eta_mm = -delta_eta_mm;
  // Particle with negative charge are expected to have the same shape as positively charged particles after
  // transformation: delta_phi --> -delta_phi
  if(charge < 0.)  delta_phi_mm = -delta_phi_mm;

  const float dist000    = TMath::Sqrt(center_r * center_r + center_z * center_z);
  const float eta_jakobi = TMath::Abs(2.0 * TMath::Exp(-center_eta) / (1.0 + TMath::Exp(-2 * center_eta)));

  const float delta_eta = delta_eta_mm / eta_jakobi / dist000;
  const float delta_phi = delta_phi_mm / center_r;

  hit.setEtaPhiZE(center_eta + delta_eta,center_phi + delta_phi,center_z, hit.E());

  ATH_MSG_DEBUG( "HIT: E=" << hit.E() << " cs=" << cs << " eta=" << hit.eta() << " phi=" << hit.phi()
                           << " z=" << hit.z() << " r=" << r << " alpha=" << alpha );

  return FCSSuccess;
}

bool TFCSHistoLateralShapeParametrization::Initialize( TH2* hist ) {
  if(!hist) return false;
	m_hist.Initialize(hist);
	if(m_hist.get_HistoContents().size()==0) return false;
	
	set_number_of_hits(hist->Integral());

  return true;
}

bool TFCSHistoLateralShapeParametrization::Initialize( const char* filepath, const char* histname ) {
  // input file with histogram to fit
  std::unique_ptr<TFile> inputfile(TFile::Open( filepath, "READ" ));
  if (inputfile == NULL) return false;

  // histogram with hit pattern
  TH2 *inputShape = (TH2*)inputfile->Get(histname);
  if (inputShape == NULL) return false;

  bool OK=Initialize(inputShape);

  inputfile->Close();

  return OK;
}

void TFCSHistoLateralShapeParametrization::Print( Option_t* option ) const {
  TString opt(option);
  bool shortprint=opt.Index("short")>=0;
  bool longprint=msgLvl(MSG::DEBUG) || (msgLvl(MSG::INFO) && !shortprint);
  TString optprint   = opt;
  optprint.ReplaceAll( "short", "" );
  TFCSLateralShapeParametrizationHitBase::Print(option);

  if(longprint) {
    if(is_phi_symmetric()) {
      ATH_MSG_INFO( optprint << "  Histo: " << m_hist.get_HistoBordersx().size() - 1 << "*"
                             << m_hist.get_HistoBordersy().size() - 1 << " bins, #hits=" << m_nhits
                             << " (phi symmetric)" );
    } else {
      ATH_MSG_INFO( optprint << "  Histo: " << m_hist.get_HistoBordersx().size() - 1 << "*"
                             << m_hist.get_HistoBordersy().size() - 1 << " bins, #hits=" << m_nhits
                             << " (not phi symmetric)" );
    }
  }  
}

#ifdef USE_GPU
void TFCSHistoLateralShapeParametrization::LoadHistFuncs() {

 if (m_LdFH ){
//  std::cout<<"2D functionHisto Already loaded " << m_LdFH << std::endl ;
         return;
  }

  m_LdFH = new LoadGpuFuncHist() ;
  FH2D fh = {0, 0,0,0,0} ;

  fh.nbinsx=m_hist.get_HistoBordersx().size()-1 ;
  fh.nbinsy=m_hist.get_HistoBordersy().size()-1 ;

  fh.h_bordersx= &(m_hist.get_HistoBordersx()[0]) ;
  fh.h_bordersy= &(m_hist.get_HistoBordersy()[0]) ;

  fh.h_contents= &(m_hist.get_HistoContents()[0]) ;

   m_LdFH->set_hf2d( &fh) ;
//   std::cout<< "*m_LdFH.hf2d() " << (*(m_LdFH->hf2d())).h_bordersx <<  std::endl ;
   m_LdFH->LD2D() ;

}

#endif
