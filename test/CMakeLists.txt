add_executable(serializer serializer.cpp)
target_link_libraries(serializer PRIVATE meld::serial spdlog::spdlog)
add_test(NAME test_serializer COMMAND serializer)

add_executable(serializer-scary serializer-scary.cpp)
target_link_libraries(serializer-scary PRIVATE meld::serial spdlog::spdlog)
add_test(NAME test_serializer-scary COMMAND serializer-scary)
