add_executable(serial_diamond serial_diamond.cpp)
target_link_libraries(serial_diamond PRIVATE meld::serial spdlog::spdlog)
add_test(NAME test_serial_diamond COMMAND serial_diamond)

add_executable(serial_fanout serial_fanout.cpp)
target_link_libraries(serial_fanout PRIVATE meld::serial spdlog::spdlog)
add_test(NAME test_serial_fanout COMMAND serial_fanout)

add_executable(reduction reduction.cpp)
target_link_libraries(reduction PRIVATE meld::reduction spdlog::spdlog)
add_test(NAME test_reduction COMMAND reduction)

add_executable(test_timed_busy test_timed_busy.cpp)
target_link_libraries(test_timed_busy PRIVATE meld::serial Catch2::Catch2WithMain)
add_test(NAME test_timed_busy COMMAND test_timed_busy)

add_executable(short_circuiting short_circuiting.cpp)
target_link_libraries(short_circuiting PRIVATE meld::serial Catch2::Catch2WithMain spdlog::spdlog TBB::tbb)
add_test(NAME short_circuiting COMMAND short_circuiting)
