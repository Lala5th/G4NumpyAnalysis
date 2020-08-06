cmake_minimum_required(VERSION 3.9)

set(MAJOR_VERSION 1)
set(MINOR_VERSION 0)
set(PATCH_VERSION 0)

project(G4NumpyAnalysis VERSION ${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}  DESCRIPTION "A wrapper for the g4cnpy library for use in Geant4 simulations")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(G4NumpyAnalysis SHARED
		${PROJECT_SOURCE_DIR}/NumpyAnalysisManager.cc
		${PROJECT_SOURCE_DIR}/cnpy.cpp)



set_target_properties(G4NumpyAnalysis PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(G4NumpyAnalysis PROPERTIES PUBLIC_HEADER ${PROJECT_SOURCE_DIR}/NumpyAnalysisManager.hh)

include(GNUInstallDirs)

install(TARGETS G4NumpyAnalysis
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


configure_file("${PROJECT_SOURCE_DIR}/cmake/G4NumpyAnalysis-config.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/g4numpyanalysis-config.cmake" @ONLY IMMEDIATE)
install(FILES "${PROJECT_BINARY_DIR}/g4numpyanalysis-config.cmake" DESTINATION "lib/cmake/g4numpyanalysis-${PROJECT_VERSION}")
