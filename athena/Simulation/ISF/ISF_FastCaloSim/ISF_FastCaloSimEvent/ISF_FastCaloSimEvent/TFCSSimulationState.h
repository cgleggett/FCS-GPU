/*
  Copyright (C) 2002-2018 CERN for the benefit of the ATLAS collaboration
*/

#ifndef ISF_FASTCALOSIMEVENT_TFCSSimulationState_h
#define ISF_FASTCALOSIMEVENT_TFCSSimulationState_h

#include <TObject.h>
#include "ISF_FastCaloSimEvent/FastCaloSim_CaloCell_ID.h"
#include <map>
#include <vector>

class CaloDetDescrElement;

namespace CLHEP
{
 class HepRandomEngine;
}



class TFCSSimulationState:public TObject
{
  public:
    TFCSSimulationState(CLHEP::HepRandomEngine* randomEngine = nullptr);

    CLHEP::HepRandomEngine* randomEngine() { return m_randomEngine; }
    void   setRandomEngine(CLHEP::HepRandomEngine *engine) { m_randomEngine = engine; }

    bool   is_valid() const {return m_Ebin>=0;};
    double E() const {return m_Etot;};
    double E(int sample)     const {return m_E[sample];};
    double Efrac(int sample) const {return m_Efrac[sample];};
    int    Ebin() const {return m_Ebin;};

    void set_Ebin(int bin) {m_Ebin=bin;};
    void set_E(int sample,double Esample)     { m_E[sample]=Esample; } ;
    void set_Efrac(int sample,double Efracsample) { m_Efrac[sample]=Efracsample; } ;
    void set_E(double E) { m_Etot=E; } ;
    void add_E(int sample,double Esample) { m_E[sample]+=Esample;m_Etot+=Esample; };

    typedef std::map<const CaloDetDescrElement*,float> Cellmap_t;

    Cellmap_t& cells() {return m_cells;};
    const Cellmap_t& cells() const {return m_cells;};
    void deposit(const CaloDetDescrElement* cellele, float E);
    
    void Print(Option_t *option="") const;
    void set_SF(double mysf) {m_SF = mysf;}
    double get_SF() {return m_SF;}
#ifdef USE_GPU
    struct EventStatus {
    	long int ievent ;
  	bool is_first ;
  	bool is_last ; 
    } ;
    void * get_gpu_rand(){ return m_gpu_rand; };
    void set_gpu_rand(void * rand ){ m_gpu_rand=rand ; } ;
    void * get_geold() { return m_geold; };
    void set_geold(void * geold ){ m_geold=geold ; } ;
    void set_es(EventStatus * es ) {m_es= es ;} ; 
    EventStatus * get_es() {return m_es; } ;
#endif
    void clear();
  private:
    CLHEP::HepRandomEngine* m_randomEngine;
      
    int    m_Ebin;
    double m_Etot;
    // TO BE CLEANED UP! SHOULD ONLY STORE EITHER E OR EFRAC!!!
    double m_SF;
    double m_E[CaloCell_ID_FCS::MaxSample];
    double m_Efrac[CaloCell_ID_FCS::MaxSample];
#ifdef USE_GPU
    void * m_gpu_rand ;
    void * m_geold ;
    EventStatus * m_es ;
#endif
    Cellmap_t m_cells;
    
  ClassDef(TFCSSimulationState,1)  //TFCSSimulationState
};

#if defined(__ROOTCLING__) && defined(__FastCaloSimStandAlone__)
#pragma link C++ class TFCSSimulationState+;
#endif

#endif
