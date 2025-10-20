file(GLOB sources 
     ${CMAKE_CURRENT_SOURCE_DIR}/**/*.cc 
     ${CMAKE_CURRENT_SOURCE_DIR}/**/**/*.cc 
     ${CMAKE_CURRENT_SOURCE_DIR}/**/*.cpp)

add_library(SLabSimuLib STATIC
            ${sources})

target_include_directories(SLabSimuLib PUBLIC
                           ${PROJECT_SOURCE_DIR}/include
                           ${ROOT_INCLUDE_DIRS})

target_link_libraries(SLabSimuLib PUBLIC
                      ${Geant4_LIBRARIES}
                      ${ROOT_LIBRARIES}
                      spdlog::spdlog
                      yaml-cpp)
