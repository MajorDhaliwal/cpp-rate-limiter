add_executable(unit_tests test_limiter.cpp)

target_link_libraries(unit_tests 
    PRIVATE 
    GTest::gtest 
    GTest::gtest_main
    limiter_core      # Links your logic
    Threads::Threads
)

include(GoogleTest)
gtest_discover_tests(unit_tests)