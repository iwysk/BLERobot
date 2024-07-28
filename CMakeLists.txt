cmake_minimum_required(VERSION 3.16.0)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(IDFTest)

set(CMAKE_CXX_STANDARD 11)
set(CONFIG_COMPILER_CXX_EXCEPTIONS ON)
idf_build_set_property(CXX_COMPILE_OPTIONS "-std=gnu++11")