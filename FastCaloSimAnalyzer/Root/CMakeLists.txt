# Copyright (C) 2002-2018 CERN for the benefit of the ATLAS collaboration

# Sources
set(FastCaloSimAnalyzer_Srcs
  CaloGeometryFromFile.cxx
  TFCS2DParametrization.cxx
  TFCSAnalyzerBase.cxx
  TFCSAnalyzerHelpers.cxx
  TFCSEnergyInterpolation.cxx
  TFCSFlatNtupleMaker.cxx
  TFCSHistoLateralShapeParametrization.cxx
  TFCSHistoLateralShapeParametrizationFCal.cxx
  TFCSHitCellMappingWiggle.cxx
  TFCSInputValidationPlots.cxx
  TFCSLateralShapeParametrizationHitChain.cxx
  TFCSShapeValidation.cxx
  TFCSValidationEnergy.cxx
  TFCSValidationEnergyAndCells.cxx
  TFCSValidationEnergyAndHits.cxx
  TFCSValidationHitSpy.cxx
  TFCSVertexZPositionStudies.cxx
  TFCSWriteCellsToTree.cxx
)

# # Global include is needed for dictionary generation to work
include_directories(../)

# Add dictionary dependencies
fcs_dictionary_dependency(${AthenaStandalone_LIB})

# ROOT dictionary
root_generate_dictionary(
  ${FastCaloSimAnalyzer_LIB}Dict

  MODULE ${FastCaloSimAnalyzer_LIB}
  LINKDEF LinkDef.h
)

# Define and build the library
message(STATUS "Creating library target '${FastCaloSimAnalyzer_LIB}'")
add_library(${FastCaloSimAnalyzer_LIB} SHARED ${FastCaloSimAnalyzer_Srcs} ${FastCaloSimAnalyzer_LIB}Dict.cxx)
target_compile_definitions(${FastCaloSimAnalyzer_LIB} PRIVATE ${FCS_CommonDefinitions})
target_link_libraries(${FastCaloSimAnalyzer_LIB} ${AthenaStandalone_LIB} ROOT::Core ROOT::Graf ROOT::Tree ROOT::TMVA)

if (CMAKE_BUILD_TYPE MATCHES Debug)
  target_link_libraries(${FastCaloSimAnalyzer_LIB} ROOT::HistPainter)
endif()

if(ENABLE_XROOTD)
  target_include_directories(${FastCaloSimAnalyzer_LIB} PRIVATE ${XROOTD_INCLUDE_DIR})
  target_link_libraries(${FastCaloSimAnalyzer_LIB} ${XROOTD_LIBRARIES})
endif()

fcs_add_dependency(${FastCaloSimAnalyzer_LIB} ${FastCaloSimCommon_LIB})
fcs_add_dependency(${FastCaloSimAnalyzer_LIB} ${AthenaStandalone_LIB})

if(ENABLE_GPU)
fcs_add_dependency(${FastCaloSimAnalyzer_LIB} ${FastCaloGpu_LIB})
elseif(ENABLE_OMPGPU)
fcs_add_dependency(${FastCaloSimAnalyzer_LIB} ${FastCaloGpu_LIB})
endif()
# Install library
install(TARGETS ${FastCaloSimAnalyzer_LIB}
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
