add_executable(resource_limited_diamond resource_limited_diamond.cpp)
target_link_libraries(resource_limited_diamond PRIVATE meld::resource_limiting spdlog::spdlog)
add_test(NAME test_resource_limited_diamond COMMAND resource_limited_diamond)

add_executable(resource_limited_fanout resource_limited_fanout.cpp)
target_link_libraries(resource_limited_fanout PRIVATE meld::resource_limiting spdlog::spdlog)
add_test(NAME test_resource_limited_fanout COMMAND resource_limited_fanout)

add_executable(reduction reduction.cpp)
target_link_libraries(reduction PRIVATE meld::reduction spdlog::spdlog)
add_test(NAME test_reduction COMMAND reduction)

add_executable(test_timed_busy test_timed_busy.cpp)
target_link_libraries(test_timed_busy PRIVATE meld::resource_limiting Catch2::Catch2WithMain)
add_test(NAME test_timed_busy COMMAND test_timed_busy)

add_executable(short_circuiting short_circuiting.cpp)
target_link_libraries(short_circuiting PRIVATE meld::resource_limiting Catch2::Catch2WithMain spdlog::spdlog TBB::tbb)
add_test(NAME short_circuiting COMMAND short_circuiting)
