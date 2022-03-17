# Copyright (C) 2002-2020 CERN for the benefit of the ATLAS collaboration

# Set a default build type if none was specified
set(FCS_default_build_type "RelWithDebInfo")
 
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${FCS_default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${FCS_default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "RelWithDebInfo")
endif()

# Verbose debug logging
set(DEBUG_LOGGING OFF CACHE BOOL "Enable verbose debug logging")

# Input path override
set(INPUT_PATH "" CACHE STRING "Override all inputs path")

# Setup ROOT
set(ROOT_VERSION 6.14.08 CACHE STRING "ROOT version required")
message(STATUS "Building with ROOT version ${ROOT_VERSION}")
include(ROOT)

# Common ATLAS (modified) macros
include(ATLAS)

# Setup paths
include(Locations)

# Supported projects
set(AthenaStandalone_LIB AthenaStandalone)
set(FastCaloSimCommon_LIB FastCaloSimCommon)
set(FastCaloSimAnalyzer_LIB FastCaloSimAnalyzer)
set(EnergyParametrization_LIB EnergyParametrization)

if(ENABLE_GPU) 
  set(FastCaloGpu_LIB FastCaloGpu)
elseif(ENABLE_SYCL)
  set(FastCaloSycl_LIB FastCaloSycl)
endif() 

# Common definitions
set(FCS_CommonDefinitions -D__FastCaloSimStandAlone__)
if(DEBUG_LOGGING)
  message(STATUS "Verbose debug logging enabled")
  set(FCS_CommonDefinitions ${FCS_CommonDefinitions} -DFCS_DEBUG)
endif()
if(NOT INPUT_PATH STREQUAL "")
  message(STATUS "Overriding all inputs path to '${INPUT_PATH}'")
  set(FCS_CommonDefinitions ${FCS_CommonDefinitions} -DFCS_INPUT_PATH=\"${INPUT_PATH}\")
endif()

if(ENABLE_GPU) 
  set(FCS_CommonDefinitions ${FCS_CommonDefinitions} -DUSE_GPU )
elseif(ENABLE_SYCL)
  set(FCS_CommonDefinitions ${FCS_CommonDefinitions} -DUSE_SYCL )
endif() 

# Common includes
set(${FastCaloSimCommon_LIB}_Includes
  ${CMAKE_SOURCE_DIR}/FastCaloSimCommon
  ${CMAKE_SOURCE_DIR}/FastCaloSimCommon/dependencies
  ${CMAKE_SOURCE_DIR}/FastCaloSimCommon/src
)
set(${AthenaStandalone_LIB}_Includes
  ${ATHENA_PATH}/Calorimeter/CaloGeoHelpers
  ${ATHENA_PATH}/Simulation/ISF/ISF_FastCaloSim/ISF_FastCaloSimEvent
  ${ATHENA_PATH}/Simulation/ISF/ISF_FastCaloSim/ISF_FastCaloSimParametrization
  ${ATHENA_PATH}/Simulation/ISF/ISF_FastCaloSim/ISF_FastCaloSimParametrization/tools
  ${ATHENA_PATH}/Simulation/ISF/ISF_FastCaloSim/ISF_FastCaloSimParametrization/tools/CaloDetDescr
)
set(${EnergyParametrization_LIB}_Includes
  ${CMAKE_SOURCE_DIR}/EnergyParametrization/src
)
set(${FastCaloSimAnalyzer_LIB}_Includes
  ${CMAKE_SOURCE_DIR}
)

if(ENABLE_GPU)
  set(${FastCaloGpu_LIB}_Includes
  ${CMAKE_SOURCE_DIR}/FastCaloGpu
)
elseif(ENABLE_SYCL)
  set(${FastCaloSycl_LIB}_Includes
  ${CMAKE_SOURCE_DIR}/FastCaloSycl
)
endif() 


# Setup helpers
include(Helpers)

# Make tarball if requested
add_custom_target(tarball tar -C ${CMAKE_SOURCE_DIR} -cvjf ${PROJECT_NAME}.tar.bz2 --exclude .git* --exclude .clang* ../${PROJECT_NAME} ${${AthenaStandalone_LIB}_Includes}
                  WORKING_DIRECTORY ${CMAKE_BUILD_DIR}
                  COMMENT "Bulding tarball"
                  VERBATIM)
