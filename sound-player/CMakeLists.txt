CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
SET(PROJECT_NAME sound-player)
PROJECT(${PROJECT_NAME} C CXX)

SET(PACKAGE ${PROJECT_NAME})
SET(PKGNAME "${SP_PKG_NAME}")
SET(PREFIX "${INSTALL_DIR}/${PKGNAME}")
SET(BINDIR "${PREFIX}/bin")
SET(RESDIR "${PREFIX}/res")
SET(DATADIR "${SP_DATA_PREFIX}/data")
SET(LOCALEDIR "${RESDIR}/locale")
SET(ICONDIR "${RESDIR}/icons/default/small")
SET(EDJDIR "${RESDIR}/edje")

IF("${CMAKE_BUILD_TYPE}" STREQUAL "")
	SET(CMAKE_BUILD_TYPE "Release")
ENDIF("${CMAKE_BUILD_TYPE}" STREQUAL "")
MESSAGE("Build type: ${CMAKE_BUILD_TYPE}")


INCLUDE(FindPkgConfig)
pkg_check_modules(sp_pkgs REQUIRED
    elementary
    ecore
    dlog
    edbus
    drm-client
    ui-gadget-1
    syspopup-caller
	capi-system-power
    vconf
    libxml-2.0
    eina
    ecore-imf
    ecore-x
    ecore-file
    ecore-input
    libcrypto
    edje
    evas
	capi-appfw-application
	capi-appfw-app-manager
	capi-media-player
	capi-media-sound-manager
	capi-media-metadata-extractor
	capi-system-media-key
	capi-content-media-content
	minicontrol-provider
	capi-network-bluetooth
	capi-system-system-settings
	utilX
	notification

)

INCLUDE_DIRECTORIES(
	${GLOBAL_INCLUDE}
	${CMAKE_SOURCE_DIR}/src/include
	${CMAKE_SOURCE_DIR}/src/common/include
	${CMAKE_SOURCE_DIR}/src/core/include
	${CMAKE_SOURCE_DIR}/src/view/include
	${CMAKE_SOURCE_DIR}/src/widget/include

	${CMAKE_CURRENT_SOURCE_DIR}/src/include
)

SET(SRCS
    ${CMAKE_SOURCE_DIR}/src/mp-main.c

	${CMAKE_SOURCE_DIR}/src/core/mp-file-tag-info.c
	${CMAKE_SOURCE_DIR}/src/core/mp-player-control.c
	${CMAKE_SOURCE_DIR}/src/core/mp-player-mgr.c
	${CMAKE_SOURCE_DIR}/src/core/mp-player-drm.c
	${CMAKE_SOURCE_DIR}/src/core/mp-setting-ctrl.c
	${CMAKE_SOURCE_DIR}/src/core/mp-playlist-mgr.c
	${CMAKE_SOURCE_DIR}/src/core/mp-streaming-mgr.c
	${CMAKE_SOURCE_DIR}/src/core/mp-lyric-mgr.c
	${CMAKE_SOURCE_DIR}/src/core/mp-http-mgr.c
	${CMAKE_SOURCE_DIR}/src/core/mp-avrcp.c
	${CMAKE_SOURCE_DIR}/src/core/mp-volume.c

	${CMAKE_SOURCE_DIR}/src/common/mp-item.c
	${CMAKE_SOURCE_DIR}/src/common/mp-ta.c
	${CMAKE_SOURCE_DIR}/src/common/mp-menu.c
	${CMAKE_SOURCE_DIR}/src/common/mp-ug-launch.c
	${CMAKE_SOURCE_DIR}/src/common/mp-widget.c
	${CMAKE_SOURCE_DIR}/src/common/mp-util.c
	${CMAKE_SOURCE_DIR}/src/common/mp-play.c
	${CMAKE_SOURCE_DIR}/src/common/mp-app.c
	${CMAKE_SOURCE_DIR}/src/common/mp-media-info.c

	${CMAKE_SOURCE_DIR}/src/view/mp-play-view.c
	${CMAKE_SOURCE_DIR}/src/view/mp-lyric-view.c

	${CMAKE_SOURCE_DIR}/src/widget/mp-smart-event-box.c
	${CMAKE_SOURCE_DIR}/src/widget/mp-popup.c
	${CMAKE_SOURCE_DIR}/src/widget/mp-minicontroller.c
	${CMAKE_SOURCE_DIR}/src/widget/mp-volume-widget.c
	${CMAKE_SOURCE_DIR}/src/widget/mp-ctxpopup.c

	${CMAKE_SOURCE_DIR}/src/core/mp-language-mgr.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/sp-view-manager.c

)


FOREACH(flag ${sp_pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag} -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")
SET(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")

SET(CMAKE_C_FLAGS_DEBUG "-O0 -g -fpie")
SET(CMAKE_C_FLAGS_RELEASE "-O2 -fpie")

ADD_DEFINITIONS("-DMP_SOUND_PLAYER")
ADD_DEFINITIONS("-DPACKAGE=\"${PACKAGE}\"")

SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--as-needed -pie")
SET(CMAKE_LDFLAGS "-Wl,-zdefs" )

ADD_EXECUTABLE(${PROJECT_NAME} ${SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${sp_pkgs_LDFLAGS} -lm)


INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${BINDIR})
INSTALL(DIRECTORY DESTINATION ${DATADIR})
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${PKGNAME}.png DESTINATION ${CMAKE_DESKTOP_ICON_DIR})

