add_executable(serializer serializer.cpp)
target_link_libraries(serializer PRIVATE meld::serial spdlog::spdlog)
add_test(NAME test_serializer COMMAND serializer)

add_executable(reduction reduction.cpp)
target_link_libraries(reduction PRIVATE meld::reduction spdlog::spdlog)
add_test(NAME test_reduction COMMAND reduction)

add_executable(serializer_scary serializer_scary.cpp)
target_link_libraries(serializer_scary PRIVATE meld::serial spdlog::spdlog)
add_test(NAME test_serializer_scary COMMAND serializer_scary)

add_executable(test_timed_busy test_timed_busy.cpp)
target_link_libraries(test_timed_busy PRIVATE meld::serial Catch2::Catch2WithMain)
add_test(NAME test_timed_busy COMMAND test_timed_busy)

add_executable(short_circuiting short_circuiting.cpp)
target_link_libraries(short_circuiting PRIVATE meld::serial Catch2::Catch2WithMain spdlog::spdlog TBB::tbb)
add_test(NAME short_circuiting COMMAND short_circuiting)
