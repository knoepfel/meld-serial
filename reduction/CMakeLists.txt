# Interface library
add_library(meld_reduction_int INTERFACE)
target_include_directories(meld_reduction_int INTERFACE
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
  "$<INSTALL_INTERFACE:include>")
target_link_libraries(meld_reduction_int INTERFACE TBB::tbb)

add_library(meld::reduction ALIAS meld_reduction_int)
