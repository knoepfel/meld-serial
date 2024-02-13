add_executable(serializer serializer.cpp)
target_link_libraries(serializer PRIVATE meld::serial)
add_test(NAME test_serializer COMMAND serializer)
