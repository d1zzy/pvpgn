# these lines create the empty directory structure pvpgn needs installed
# in ./var and then installs it portably using install(DIRECTORY...)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/var/users")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/var/userscdb")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/var/bnmail")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/var/clans")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/var/teams")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/var/reports")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/var/chanlogs")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/var/charinfo")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/var/charsave")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/var/bak/charinfo")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/var/bak/charsave")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/var/ladders")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/var/status")

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/var/ DESTINATION ${LOCALSTATEDIR})

install(FILES ad000001.mng ad000001.smk ad000002.mng ad000003.mng 
    ad000004.mng ad000075.mng chathelp-war3-default.txt 
    chathelp-war3-enUS.txt chathelp-war3-frFR.txt chathelp-war3-ruRU.txt 
    chathelp-war3-zhCN.txt newaccount-default.txt newaccount-enUS.txt 
    newbie.save termsofservice-default.txt termsofservice-enUS.txt 
    tos_default.txt tos-unicode_default.txt
    DESTINATION ${LOCALSTATEDIR}/files)
