get_filename_component(DIR_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${DIR_NAME})

find_package(Qt5 REQUIRED COMPONENTS Widgets)

file(GLOB_RECURSE ALL_FILE "*.h" "*.cpp" "*.hpp")
add_library(${DIR_NAME} SHARED ${ALL_FILE})
target_link_libraries(${DIR_NAME} PRIVATE Qt5::Widgets)
