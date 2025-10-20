cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(SLabSimu)

set(CMAKE_CXX_STANDARD 17) 
set(CMAKE_CXX_STANDARD_REQUIRED on) 

set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")


#----------------------------------------------------------------------------
# Find Geant4 package, activating all available UI and Vis drivers by default
# You can set WITH_GEANT4_UIVIS to OFF via the command line or ccmake/cmake-gui
# to build a batch mode only executable
#
option(WITH_GEANT4_UIVIS "Enable Geant4 UI and Visualization" ON)
if(WITH_GEANT4_UIVIS)
    find_package(Geant4 REQUIRED ui_all vis_all)
else()
    find_package(Geant4 REQUIRED)
endif()
include(${Geant4_USE_FILE})

#----------------------------------------------------------------------------
# Find ROOT (required package)
#
find_package(ROOT REQUIRED COMPONENTS Geom TMVA MathMore)
if(NOT ROOT_FOUND)
    message(FATAL_ERROR "Can not find ROOT!")
endif()

# Find packages used in the project
find_package(yaml-cpp)
find_package(spdlog)

# source directory
add_subdirectory(src)

# executable
add_executable(SLabSimu ${PROJECT_SOURCE_DIR}/SLabSimu.cc)
target_link_libraries(SLabSimu SLabSimuLib)

#----------------------------------------------------------------------------
# Install the executable to 'bin' directory under CMAKE_INSTALL_PREFIX
#
install(TARGETS SLabSimu DESTINATION ${CMAKE_INSTALL_PREFIX})
