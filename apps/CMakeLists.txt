add_executable(fboo factorio.cpp)

target_link_libraries(fboo PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(fboo PRIVATE factorio)

foreach(PATH IN ITEMS factory item recipe technology)
  string(TOUPPER ${PATH} NAME)
  get_filename_component(JSON_${NAME} ../json/${PATH}.json REALPATH)
endforeach()

configure_file(paths.h.in paths.h)
target_include_directories(fboo PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
