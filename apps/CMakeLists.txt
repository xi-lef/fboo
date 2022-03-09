add_executable(fboo factorio.cpp)

target_link_libraries(fboo PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(fboo PRIVATE factorio)

foreach(PATH IN ITEMS factory item recipe technology)
  string(TOUPPER ${PATH} NAME)
  get_filename_component(JSON_${NAME} ../json/${PATH}.json REALPATH)
endforeach()
get_filename_component(JSON_CHALLENGE1 ../json/challenges/challenge-1.json REALPATH)
get_filename_component(JSON_CHALLENGE2 ../json/challenges/challenge-2.json REALPATH)

configure_file(paths.h.in paths.h)
target_include_directories(fboo PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
