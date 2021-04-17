IF (NOT APPLICATION_NAME)
   MESSAGE(STATUS "${PROJECT_NAME} is used as APPLICATION_NAME")
   SET(APPLICATION_NAME ${PROJECT_NAME})
ENDIF (NOT APPLICATION_NAME)

SET(EXEC_INSTALL_PREFIX
  "${CMAKE_INSTALL_PREFIX}"
  CACHE PATH  "Base directory for executables and libraries"
  FORCE
)

SET(SHARE_INSTALL_PREFIX
  "${CMAKE_INSTALL_PREFIX}/share"
  CACHE PATH "Base directory for files which go to share/"
  FORCE
)

SET(DATA_INSTALL_PREFIX
  "${SHARE_INSTALL_PREFIX}/${APPLICATION_NAME}"
  CACHE PATH "The parent directory where applications can install their data"
  FORCE
)

SET(BIN_INSTALL_DIR
  "${EXEC_INSTALL_PREFIX}/bin"
  CACHE PATH "The ${APPLICATION_NAME} binary install dir (default prefix/bin)"
  FORCE
)

#***********************************#

if(WIN32)
  SET(LOCALSTATE_INSTALL_DIR
    "${EXEC_INSTALL_PREFIX}/var"
    CACHE PATH "The ${APPLICATION_NAME} local state install dir (default prefix/var)"
    FORCE
  )
else()
  SET(LOCALSTATE_INSTALL_DIR
    "${EXEC_INSTALL_PREFIX}/var/${APPLICATION_NAME}"
    CACHE PATH "The ${APPLICATION_NAME} local state install dir (default prefix/var)"
    FORCE
  )
endif()

SET(MAN_INSTALL_DIR
  "${SHARE_INSTALL_PREFIX}/man"
  CACHE PATH "The ${APPLICATION_NAME} man install dir (default prefix/man)"
  FORCE
)

if(WIN32)
	SET(SBIN_INSTALL_DIR
	  "${EXEC_INSTALL_PREFIX}"
	  CACHE PATH "The ${APPLICATION_NAME} sbin install dir (default prefix/sbin)"
	  FORCE
	)
else()
	SET(SBIN_INSTALL_DIR
	  "${EXEC_INSTALL_PREFIX}/sbin"
	  CACHE PATH "The ${APPLICATION_NAME} sbin install dir (default prefix/sbin)"
	  FORCE
	)
endif()

if(WIN32)
  SET(SYSCONF_INSTALL_DIR 
    "${EXEC_INSTALL_PREFIX}/conf"
    CACHE PATH "The ${APPLICATION_NAME} sysconfig install dir (default conf)"
    FORCE
  )
else()
  SET(SYSCONF_INSTALL_DIR
    "${EXEC_INSTALL_PREFIX}/etc/${APPLICATION_NAME}"
    CACHE PATH "The ${APPLICATION_NAME} sysconfig install dir (default prefix/etc)"
    FORCE
  )
endif()