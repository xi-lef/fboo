cmake_minimum_required(VERSION 3.20)

project(
  FactorioOptimizer
  VERSION 1.0
  DESCRIPTION "Factorio optimizer for AdvPT"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json 3.2.0 REQUIRED)

add_subdirectory(apps)
add_subdirectory(src)
