cmake_minimum_required(VERSION 3.22)

project(meld-serial VERSION 0.0.1 LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
add_compile_options(-Wall -Werror -Wunused -Wunused-parameter -pedantic)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)

find_package(TBB REQUIRED)

add_subdirectory(serial)

include(CTest)
add_subdirectory(test)
