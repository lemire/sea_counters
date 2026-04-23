# Test target for bench
add_executable(test_bench test_bench.c)
set_target_properties(test_bench PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED YES)

# Use the interface target for include directories
target_link_libraries(test_bench PRIVATE sea_counters::sea_counters)

add_test(NAME bench_test COMMAND test_bench)
