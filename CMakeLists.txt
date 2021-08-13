cmake_minimum_required(VERSION 3.9)

set(MAJOR_VERSION 2)
set(MINOR_VERSION 1)
set(PATCH_VERSION 2)

project(G4NumpyAnalysis VERSION ${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}  DESCRIPTION "A wrapper for the g4cnpy library for use in Geant4 simulations")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(G4NumpyAnalysis SHARED
		${PROJECT_SOURCE_DIR}/NumpyAnalysisManager.cc
		${PROJECT_SOURCE_DIR}/g4cnpy.cpp)

find_package(ZLIB REQUIRED)
if(WIN32)
	find_package(LIBZIP CONFIG REQUIRED)
	set(LIBZIP_LIBRARY libzip::zip)
else()
	include(${PROJECT_SOURCE_DIR}/cmake/FindZipLib.cmake)
endif()

include_directories(${ZLIB_INCLUDE_DIRS})
include_directories(${LIBZIP_INCLUDE_DIRS})

target_link_libraries(G4NumpyAnalysis ${ZLIB_LIBRARIES} ${LIBZIP_LIBRARY})

set_target_properties(G4NumpyAnalysis PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(G4NumpyAnalysis PROPERTIES PUBLIC_HEADER ${PROJECT_SOURCE_DIR}/NumpyAnalysisManager.hh)

include(GNUInstallDirs)

install(TARGETS G4NumpyAnalysis
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${PROJECT_SOURCE_DIR}/g4cnpy.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


configure_file("${PROJECT_SOURCE_DIR}/cmake/G4NumpyAnalysis-config.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/g4numpyanalysis-config.cmake" @ONLY IMMEDIATE)
install(FILES "${PROJECT_BINARY_DIR}/g4numpyanalysis-config.cmake" DESTINATION "lib/cmake/g4numpyanalysis-${PROJECT_VERSION}")

configure_file("${PROJECT_SOURCE_DIR}/cmake/G4NumpyAnalysis-config-version.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/g4numpyanalysis-config-version.cmake" @ONLY IMMEDIATE)
install(FILES "${PROJECT_BINARY_DIR}/g4numpyanalysis-config-version.cmake" DESTINATION "lib/cmake/g4numpyanalysis-${PROJECT_VERSION}")


add_executable(test test.cc ${sources} ${headers})
target_link_libraries(test ${ZLIB_LIBRARIES} G4NumpyAnalysis ${LIBZIP_LIBRARY})

add_executable(test_cont_save test_cont_save.cc ${sources} ${headers})
target_link_libraries(test_cont_save ${ZLIB_LIBRARIES} G4NumpyAnalysis ${LIBZIP_LIBRARY})
