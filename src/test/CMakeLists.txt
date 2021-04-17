add_executable(bnetsrp3_test bnetsrp3_test.cpp )
target_link_libraries(bnetsrp3_test PRIVATE common)
add_test(bnetsrp3_test bnetsrp3_test)

add_executable(bigint bigint.cpp )
target_link_libraries(bigint PRIVATE common)
add_test(bigint bigint)
