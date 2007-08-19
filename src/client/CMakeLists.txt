add_executable(bnchat bnchat.cpp client.cpp client_connect.cpp udptest.cpp)
target_link_libraries(bnchat common compat)

add_executable(bnftp bnftp.cpp client.cpp)
target_link_libraries(bnftp common compat)

add_executable(bnbot bnbot.cpp client.cpp)
target_link_libraries(bnbot common compat)

add_executable(bnstat bnstat.cpp client.cpp client_connect.cpp udptest.cpp)
target_link_libraries(bnstat common compat)

IF(WIN32)
 target_link_libraries(bnchat ws2_32)
 target_link_libraries(bnftp ws2_32)
 target_link_libraries(bnbot ws2_32)
 target_link_libraries(bnstat ws2_32)
ENDIF(WIN32)
