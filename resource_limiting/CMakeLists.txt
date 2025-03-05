add_library(meld_resource_limiting SHARED
  timed_busy.cpp
)
target_include_directories(meld_resource_limiting PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(meld_resource_limiting PRIVATE TBB::tbb)

# Interface library
add_library(meld_resource_limiting_int INTERFACE)
target_include_directories(meld_resource_limiting_int INTERFACE
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
  "$<INSTALL_INTERFACE:include>")
target_link_libraries(meld_resource_limiting_int INTERFACE meld_resource_limiting TBB::tbb)

add_library(meld::resource_limiting ALIAS meld_resource_limiting_int)
