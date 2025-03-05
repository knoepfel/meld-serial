cmake_minimum_required(VERSION 3.22)

project(meld-serial VERSION 0.0.1 LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# add_compile_options(
#   -fno-omit-frame-pointer
#   -fsanitize=address
#   -fsanitize=undefined
# )

# add_link_options(
#   -fno-omit-frame-pointer
#   -fsanitize=address
#   -fsanitize=undefined
# )


set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)
set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "Use external fmt library")

include(FetchContent)

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG        10.2.1
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(fmt)

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        v1.13.0
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.5.2
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(Catch2)

find_package(TBB REQUIRED)

add_compile_options(-Wall -Werror -Wunused -Wunused-parameter -pedantic)
add_subdirectory(reduction)
add_subdirectory(resource_limiting)

include(CTest)
add_subdirectory(test)
