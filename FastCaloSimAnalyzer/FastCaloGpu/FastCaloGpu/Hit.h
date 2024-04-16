/*
  Copyright (C) 2002-2021 CERN for the benefit of the ATLAS collaboration
*/

#ifndef HIT_G_H
#define HIT_G_H

#include "HostDevDef.h"

class Hit {
public:
  __HOSTDEV__ Hit() {}; // for hits with the same energy, m_E should normalized to E(layer)/nhit
  __HOSTDEV__ Hit( float eta, float phi, float E )
      : m_eta_x( eta )
      , m_phi_y( phi )
      , m_E( E )
      , m_useXYZ( false ) {};
  __HOSTDEV__ Hit( float x, float y, float z, float E )
      : m_eta_x( x )
      , m_phi_y( y )
      , m_z( z )
      , m_E( E )
      , m_useXYZ( true ) {};
  
  __HOSTDEV__ __INLINE__ void setEtaPhiZ( float eta, float phi, float z) {
    m_eta_x  = eta;
    m_phi_y  = phi;
    m_z      = z;
    m_useXYZ = false;
  }
  __HOSTDEV__ __INLINE__ void setEtaPhiZE( float eta, float phi, float z, float E ) {
    m_eta_x  = eta;
    m_phi_y  = phi;
    m_z      = z;
    m_E      = E;
    m_useXYZ = false;
  }
  __HOSTDEV__ __INLINE__ void setXYZE( float x, float y, float z, float E ) {
    m_eta_x  = x;
    m_phi_y  = y;
    m_z      = z;
    m_E      = E;
    m_useXYZ = true;
  }

  __HOSTDEV__ __INLINE__ void reset() {
    m_eta_x  = 0.;
    m_phi_y  = 0.;
    m_z      = 0.;
    m_E      = 0.;
    m_useXYZ = false;
  }

  __HOSTDEV__ __INLINE__ float& eta() { return m_eta_x; };
  __HOSTDEV__ __INLINE__ float& phi() { return m_phi_y; };
  __HOSTDEV__ __INLINE__ float& x() { return m_eta_x; };
  __HOSTDEV__ __INLINE__ float& y() { return m_phi_y; };
  __HOSTDEV__ __INLINE__ float& E() { return m_E; };
  __HOSTDEV__ __INLINE__ float& z() { return m_z; }
  __HOSTDEV__ __INLINE__ float  r() const {
    if ( m_useXYZ )
      return sqrt( m_eta_x * m_eta_x + m_phi_y * m_phi_y );
    else
      return m_z / sinh( m_eta_x );
  }
  __HOSTDEV__ __INLINE__ float& center_r() { return m_center_r; }
  __HOSTDEV__ __INLINE__ float& center_z() { return m_center_z; }
  __HOSTDEV__ __INLINE__ float& center_eta() { return m_center_eta; }
  __HOSTDEV__ __INLINE__ float& center_phi() { return m_center_phi; }
  __HOSTDEV__ __INLINE__ void   setCenter_r( float r ) { m_center_r = r; }
  __HOSTDEV__ __INLINE__ void   setCenter_z( float z ) { m_center_z = z; }
  __HOSTDEV__ __INLINE__ void   setCenter_eta( float eta ) { m_center_eta = eta; }
  __HOSTDEV__ __INLINE__ void   setCenter_phi( float phi ) { m_center_phi = phi; }

  void print() const {
    printf("hit- E: %f  eta: %f  phi: %f  r: %f  z: %f\n",m_E, m_eta_x, m_phi_y, this->r(), m_z);
  }
  
private:
  float m_eta_x      {0.}; // eta for barrel and end-cap, x for FCal
  float m_phi_y      {0.}; // phi for barrel and end-cap, y for FCal
  float m_z          {0.};
  float m_E          {0.};
  bool  m_useXYZ     {false};
  // Variables used to store extrapolated position
  float m_center_r   {0.};
  float m_center_z   {0.};
  float m_center_eta {0.};
  float m_center_phi {0.};
};

#endif
