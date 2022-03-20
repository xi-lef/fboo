add_library(factorio entity.cpp event.cpp game.cpp order.cpp)

target_include_directories(
  factorio
  INTERFACE ${PROJECT_SOURCE_DIR}/include
  PRIVATE ${PROJECT_SOURCE_DIR}/include/fboo)
