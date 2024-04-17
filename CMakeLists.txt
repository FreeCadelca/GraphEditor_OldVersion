cmake_minimum_required(VERSION 3.27)
project(GraphEditor)

set(CMAKE_CXX_STANDARD 14)

add_executable(
        GraphEditor
        main.cpp
        Canvas.h
        Canvas.cpp
)

find_package(PkgConfig)
pkg_check_modules(GTKMM gtkmm-3.0)

include_directories(${GTKMM_INCLUDE_DIRS})
link_directories(${GTKMM_LIBRARY_DIRS})

set(SOURCE_FILES main.cpp)
target_link_libraries(${CMAKE_PROJECT_NAME} ${GTKMM_LIBRARIES})