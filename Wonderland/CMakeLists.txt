cmake_minimum_required(VERSION 3.25)
project(Wonderland VERSION 1.0.0)

if(NOT DEFINED IPLUG2_DIR)
  set(IPLUG2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../iPlug2" CACHE PATH "iPlug2 root directory")
endif()

include(${IPLUG2_DIR}/iPlug2.cmake)

find_package(iPlug2 REQUIRED)

iplug_add_plugin(${PROJECT_NAME}
  SOURCES
    Wonderland.cpp
    Wonderland.h
    WonderlandDSP.h
    WonderlandControls.h
    resources/resource.h
  RESOURCES
    resources/fonts/Poppins-ExtraLight.ttf
    resources/fonts/Poppins-Light.ttf
    resources/fonts/Poppins-Regular.ttf
    resources/fonts/Poppins-Medium.ttf
  FORMATS
    MINIMAL_PLUGINS APP WASM
)