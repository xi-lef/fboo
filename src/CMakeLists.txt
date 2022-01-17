add_library(factorio entity.cpp event.cpp)

target_include_directories(
  factorio
  INTERFACE ${PROJECT_SOURCE_DIR}/include
  PRIVATE ${PROJECT_SOURCE_DIR}/include/fboo)

add_compile_options(-Wall -Wextra -pedantic)
