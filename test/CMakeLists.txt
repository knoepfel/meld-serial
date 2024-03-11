add_executable(serializer serializer.cpp)
target_link_libraries(serializer PRIVATE meld::serial spdlog::spdlog)
add_test(NAME test_serializer COMMAND serializer)

add_executable(serializer-scary serializer-scary.cpp)
target_link_libraries(serializer-scary PRIVATE meld::serial spdlog::spdlog)
add_test(NAME test_serializer_scary COMMAND serializer-scary)

add_executable(test_timed_busy test_timed_busy.cpp)
target_link_libraries(test_timed_busy PRIVATE meld::serial Catch2::Catch2WithMain)
add_test(NAME test_timed_busy COMMAND test_timed_busy)
