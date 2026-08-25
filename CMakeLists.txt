cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version} LANGUAGES C CXX)

include(compilerconfig)
include(defaults)
include(helpers)

find_package(libobs REQUIRED)
find_package(obs-frontend-api REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

add_library(${CMAKE_PROJECT_NAME} MODULE)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
  src/plugin-main.cpp
  src/dashboard-widget.cpp
  src/dashboard-widget.hpp
  src/scene-card.cpp
  src/scene-card.hpp
  src/obs-preview.cpp
  src/obs-preview.hpp
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
  OBS::libobs
  OBS::obs-frontend-api
  Qt6::Core
  Qt6::Widgets
)

target_compile_features(${CMAKE_PROJECT_NAME} PRIVATE cxx_std_17)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE UNICODE _UNICODE)
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON)

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})
