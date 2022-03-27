cmake_minimum_required(VERSION 3.16)

project(
  FactorioOptimizer
  VERSION 1.0
  DESCRIPTION "Factorio optimizer for AdvPT"
  LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
  add_compile_options(-march=native)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

add_compile_options(-Wall -Wextra -pedantic)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json 3.2.0 REQUIRED)

add_subdirectory(apps)
add_subdirectory(src)
