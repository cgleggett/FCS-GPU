/*
  Copyright (C) 2002-2018 CERN for the benefit of the ATLAS collaboration
*/

#include <fstream>
#include <iostream>
#include <sstream>

#include "TFCSSampleDiscovery.h"

std::string TFCSSampleDiscovery::m_baseDir = "";

TFCSSampleDiscovery::TFCSSampleDiscovery() :
  m_invalid( FCS::DSIDInfo( -1 ) ) {

  if (m_baseDir == "") {
    throw std::runtime_error("DataDir not set for input files either via arg or env var");
  }

}

TFCSSampleDiscovery::TFCSSampleDiscovery(const std::string& dir, const std::string& fileName,
                                         bool debug)
  : m_invalid( FCS::DSIDInfo( -1 ) ), m_dsidDB(fileName) {

    m_baseDir = dir;
    
    std::cout << "Reading input files from " << m_baseDir << std::endl;
    std::cout << "Initialising DSID DB..." << std::endl;

  // init the DB
  std::ifstream file = openFile( fileName );
  if ( !file ) {
    std::cerr << "DB initialisation failed: " << fileName << " not found :(((" << std::endl;
    exit( 1 );
  }

  std::string line;
  while ( getline( file, line ) ) {
    std::stringstream linestream( line );
    int               pdgId, energy, z, dsid;
    float             eta;
    linestream >> pdgId >> energy >> eta >> z >> dsid;

    m_dbDSID.emplace_back( dsid, pdgId, energy, eta, z );
  }

  if ( debug ) {
    for (const FCS::DSIDInfo &info : m_dbDSID) {
      std::cout << info << std::endl;
    }
  }

  std::cout << "DB ready" << std::endl;
}

const FCS::DSIDInfo &TFCSSampleDiscovery::findDSID(int pdgId, int energy,
                                                   float eta, int zVertex) const
{
  for ( const FCS::DSIDInfo& info : m_dbDSID ) {
    if (pdgId == info.pdgId && energy == info.energy && eta == info.eta &&
        zVertex == info.zVertex) {
      return info;
  }
  }

  std::cerr << " Error: no DSID matching pid " << pdgId << " energy " << energy
            << " eta " << eta << " zVertex " << zVertex << " found in DSID DB \'"
            << m_dsidDB << "\'" << std::endl;

  return m_invalid;
}

int TFCSSampleDiscovery::getEnergy(int dsid) const
{
  for ( const FCS::DSIDInfo& info : m_dbDSID ) {
    if (info.dsid == dsid) {
      return info.energy;
    }
  }

  return 0;
}

int TFCSSampleDiscovery::getPdgId(int dsid) const
{
  for ( const FCS::DSIDInfo& info : m_dbDSID ) {
    if (info.dsid == dsid) {
      return info.pdgId;
    }
  }

  return -1;
}

FCS::SampleInfo TFCSSampleDiscovery::findSample( int inDSID, const std::string& fileName ) const {
  int         dsid, pdgId, energy, zVertex;
  std::string location, label;
  float       etaMin, etaMax;

  std::ifstream file = openFile( fileName );
  if ( !file ) {
    std::cerr << " Error: Unable to open file \'" << fileName
              << "\' with input samples data" << std::endl;
    exit( 1 );
  }

  while ( !file.eof() ) {
    file >> dsid >> location >> label >> pdgId >> energy
          >> etaMin >> etaMax >> zVertex;

    if ( dsid == inDSID ) {
      return FCS::SampleInfo( dsid, m_baseDir + "/" + FCS::DIR_INPUTS + location, label,
                              pdgId, energy, etaMin, etaMax, zVertex );
    }
  }

  std::cerr << " Error: DSID " << inDSID << " not found in input sample file \'"
            << fileName << "\'" << std::endl;
  
  return FCS::SampleInfo( -1 );
}

std::string TFCSSampleDiscovery::getBaseName(int dsid) const
{
  for ( const FCS::DSIDInfo& info : m_dbDSID ) {
    if ( info.dsid == dsid ) {
      std::stringstream s;
      s << "mc16_13TeV." << dsid << ".ParticleGun_pid" << info.pdgId
        << "_E" << info.energy << "_disj_eta_m" << ( info.eta + 5 )
        << "_m" << info.eta << "_" << info.eta << "_"
        << ( info.eta + 5 ) << "_zv_" << info.zVertex;
      return s.str();
    }
  }

  return "";
}

std::string TFCSSampleDiscovery::getFirstPCAAppName(int dsid,
                                                    const std::string &version) const {
  return getName( dsid, "firstPCA_App", m_baseDir + "/" + FCS::DIR_FIRSTPCA, version );
}

std::string TFCSSampleDiscovery::getSecondPCAName(int dsid,
                                                  const std::string &version) const
{
  return getName( dsid, "secondPCA", m_baseDir + "/" + FCS::DIR_DSID, version );
}

std::string TFCSSampleDiscovery::getShapeName( int dsid,
                                              const std::string &version) const
{
  return getName( dsid, "shapepara", m_baseDir + "/" + FCS::DIR_DSID, version );
}

std::string TFCSSampleDiscovery::getAvgSimShapeName( int dsid,
                                                    const std::string &version) const
{
  return getName( dsid, "AvgSimShape", m_baseDir + "/" + FCS::DIR_DSID, version );
}

std::string TFCSSampleDiscovery::getEinterpolMeanName( int pdgId,
                                                      const std::string &version) const
{
  return m_baseDir + "/" + FCS::DIR_INTERPOLATION + "mc16_13TeV.pid" + std::to_string( pdgId ) + ".EinterpolMean." + version + ".root";
}

std::string TFCSSampleDiscovery::getParametrizationName(const std::string &version)
{
  return m_baseDir + "/" + FCS::DIR_PARAMETRIZATION  + "TFCSparam_" + version + ".root";
}

std::string TFCSSampleDiscovery::getWiggleName(const std::string &etaRange,
                                               int sampling,
                                                bool isNewWiggle,
                                               const std::string &version)
{
  if ( isNewWiggle ) {
    return m_baseDir + "/" + FCS::DIR_WIGGLE + "Wiggle/" + etaRange + "." + version + ".root";
  } else {
    return m_baseDir + "/" + FCS::DIR_WIGGLE + "Wiggle_old/" + etaRange +
      "/wiggle_input_deriv_Sampling_" + std::to_string( sampling ) + ".ver03.root";    
  }
}

std::string TFCSSampleDiscovery::geometryTree()
{
  return "ATLAS-R2-2016-01-00-01";
}

std::string TFCSSampleDiscovery::geometryName()
{
  return m_baseDir + "/" + FCS::DIR_GEOMETRY + "Geometry-ATLAS-R2-2016-01-00-01.root";
}

std::array<std::string, 3> TFCSSampleDiscovery::geometryNameFCal() {
  return {
    m_baseDir + "/" + FCS::DIR_GEOMETRY + "FCal1-electrodes.sorted.HV.09Nov2007.dat",
    m_baseDir + "/" + FCS::DIR_GEOMETRY + "FCal2-electrodes.sorted.HV.April2011.dat",
    m_baseDir + "/" + FCS::DIR_GEOMETRY + "FCal3-electrodes.sorted.HV.09Nov2007.dat",
  };
}

std::string TFCSSampleDiscovery::geometryMap() {
  return m_baseDir + "/" + FCS::DIR_GEOMETRY + "cellId_vs_cellHashId_map.txt";
}

std::ifstream TFCSSampleDiscovery::openFile(const std::string &fileName) const
{
  // first try directly
  std::ifstream file( fileName );
  // if not found try common data folder
  if ( !file && fileName.find( "/" ) == std::string::npos ) {
    std::string path = std::getenv( "FCSSTANDALONE" );
    return std::ifstream( path + "/data/" + fileName );
  }

  return file;
}

std::string TFCSSampleDiscovery::getName( int dsid, const std::string& label,
                                          const std::string& basedir,
                                          const std::string& version ) const {
  return basedir + getBaseName( dsid ) + "." + label + "." + version + ".root";
}
