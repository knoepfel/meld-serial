add_library(meld_serial SHARED
  serializer_node.cpp
  timed_busy.cpp 
)
target_include_directories(meld_serial PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(meld_serial PRIVATE TBB::tbb)

# Interface library
add_library(meld_serial_int INTERFACE)
target_include_directories(meld_serial_int INTERFACE
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
  "$<INSTALL_INTERFACE:include>")
target_link_libraries(meld_serial_int INTERFACE meld_serial TBB::tbb)

add_library(meld::serial ALIAS meld_serial_int)
