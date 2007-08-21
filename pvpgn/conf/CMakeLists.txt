# generate the config files replacing the path variables
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bnetd.conf.cmake ${CMAKE_CURRENT_BINARY_DIR}/bnetd.conf)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/d2cs.conf.cmake ${CMAKE_CURRENT_BINARY_DIR}/d2cs.conf)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/d2dbs.conf.cmake ${CMAKE_CURRENT_BINARY_DIR}/d2dbs.conf)

if(WITH_BNETD)
    set(BNETD_CONFS bnetd.conf ad.conf anongame_infos.conf address_translation.conf 
    autoupdate.conf bnalias.conf bnban.conf bnetd_default_user.cdb 
    bnetd_default_user.plain bnhelp.conf bnissue.txt bnmaps.conf bnmotd.txt 
    bnxpcalc.conf bnxplevel.conf channel.conf command_groups.conf news.txt 
    realm.conf sql_DB_layout.conf supportfile.conf topics.conf tournament.conf 
    versioncheck.conf)
endif(WITH_BNETD)

if(WITH_D2CS)
    set(D2CS_CONFS d2cs.conf d2server.ini anongame_infos.conf)
endif(WITH_D2CS)

if(WITH_D2DBS)
    set(D2DBS_CONFS d2dbs.conf)
endif(WITH_D2DBS)

install(FILES ${BNETD_CONFS} ${D2CS_CONFS} ${D2DBS_CONFS}
    DESTINATION ${SYSCONFDIR})
