CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(music-player C CXX)

EXEC_PROGRAM(uname ARGS -m OUTPUT_VARIABLE CMAKE_SYSTEM_VERSION)
message("Building by ${CMAKE_SYSTEM_VERSION} machine")

SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(BINDIR "${PREFIX}/bin")
SET(LIBDIR "${PREFIX}/lib")
SET(RESDIR "${PREFIX}/res")
SET(DATADIR "${DATA_PREFIX}/data")
SET(LOCALEDIR "${RESDIR}/locale")
SET(ICONDIR "${RESDIR}/icons/default/small")
SET(EDJDIR "${RESDIR}/edje")

message("CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}")
message("PREFIX ${PREFIX}")
message("BINDIR ${BINDIR}")
message("DATADIR ${DATADIR}")

SET(COMMON_SRCS
	src/mp-main.c

	src/core/mp-file-tag-info.c
	src/core/mp-player-control.c
	src/core/mp-player-mgr.c
	src/core/mp-player-drm.c
	src/core/mp-setting-ctrl.c
	src/core/mp-streaming-mgr.c
	src/core/mp-lyric-mgr.c
	src/core/mp-avrcp.c
	src/core/mp-volume.c
	src/core/mp-http-mgr.c
	src/core/mp-language-mgr.c
	src/core/mp-playlist-mgr.c
	src/common/mp-common.c
	src/common/mp-item.c
	src/common/mp-ta.c
	src/common/mp-menu.c
	src/common/mp-ug-launch.c
	src/common/mp-widget.c
	src/common/mp-util.c
	src/common/mp-play.c
	src/common/mp-app.c
	src/common/mp-view-manager.c
	src/common/mp-media-info.c
	src/view/mp-library.c
	src/view/mp-track-view.c
	src/view/mp-group-view.c
	src/view/mp-playlist-view.c
	src/view/mp-play-view.c
	src/view/mp-lyric-view.c
	src/view/mp-edit-view.c
	src/widget/mp-search.c
	src/widget/mp-smart-event-box.c
	src/widget/mp-popup.c
	src/widget/mp-view-layout.c
	src/widget/mp-minicontroller.c
	src/widget/mp-volume-widget.c
	src/widget/mp-ctxpopup.c

)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include
	src/include
	src/common/include
	src/core/include
	src/list/include
	src/view/include
	src/widget/include
)

SET(SRCS ${COMMON_SRCS} ${RICH_INFO_SRCS})

INCLUDE(FindPkgConfig)
pkg_check_modules(pkgs REQUIRED
	elementary
	ecore
	dlog
	vconf
	drm-client
	glib-2.0
	ui-gadget-1
	syspopup-caller
	capi-system-power
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
	capi-media-sound-manager
	capi-media-player
	capi-media-metadata-extractor
	capi-content-media-content
	minicontrol-provider
	capi-network-bluetooth
	capi-system-system-settings
	capi-system-media-key
	utilX
	notification

)

FOREACH(flag ${pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)
SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} -DMACHINE_$ENV{MACHINE} -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64")

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -fPIC")
SET(CMAKE_C_FLAGS_DEBUG "-O0 -g -fpie")
SET(CMAKE_C_FLAGS_RELEASE "-O2 -Wall -g -fpie")
SET(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -Xassembler -W -fPIC")
SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed -pie")

ADD_DEFINITIONS("-DPREFIX=\"${CMAKE_INSTALL_PREFIX}\"")
ADD_DEFINITIONS("-DPKG_NAME=\"${PKG_NAME}\"")
ADD_DEFINITIONS("-DDATA_PREFIX=\"${DATA_PREFIX}\"")
ADD_DEFINITIONS("-DFACTORYFS=\"$ENV{FACTORYFS}\"")
ADD_DEFINITIONS("-DDESKTOP_ICON=\"${CMAKE_DESKTOP_ICON_DIR}/${PKG_NAME}.png\"")

ADD_EXECUTABLE(${PROJECT_NAME} ${SRCS})

MESSAGE("### CMAKE_SYSTEM_VERSION :: ${CMAKE_SYSTEM_VERSION} ###")
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${pkgs_LDFLAGS} -lm -ldl)

ADD_CUSTOM_TARGET(mp-custom-winset-style.edj
		COMMAND edje_cc -id ${CMAKE_CURRENT_SOURCE_DIR}/images/music_player -sd ${CMAKE_CURRENT_SOURCE_DIR}/images/sounds
		${CMAKE_SOURCE_DIR}/layout/mp-custom-winset-style.edc ${CMAKE_BINARY_DIR}/layout/mp-custom-winset-style.edj
		DEPENDS ${CMAKE_SOURCE_DIR}/layout/mp-custom-winset-style.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} mp-custom-winset-style.edj)

ADD_CUSTOM_TARGET(mp-library.edj
		COMMAND edje_cc -id ${CMAKE_CURRENT_SOURCE_DIR}/images/music_player
		${CMAKE_SOURCE_DIR}/layout/mp-library.edc ${CMAKE_BINARY_DIR}/layout/mp-library.edj
		DEPENDS ${CMAKE_SOURCE_DIR}/layout/mp-library.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} mp-library.edj)

ADD_CUSTOM_TARGET(music.edj
		COMMAND edje_cc -id ${CMAKE_CURRENT_SOURCE_DIR}/images/music_player
		${CMAKE_SOURCE_DIR}/layout/music.edc ${CMAKE_BINARY_DIR}/layout/music.edj
		DEPENDS ${CMAKE_SOURCE_DIR}/layout/music.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} music.edj)

ADD_CUSTOM_TARGET(mp-minicontroller.edj
		COMMAND edje_cc -id ${CMAKE_CURRENT_SOURCE_DIR}/images/music_player
		${CMAKE_SOURCE_DIR}/layout/mp-minicontroller.edc ${CMAKE_BINARY_DIR}/layout/mp-minicontroller.edj
		DEPENDS ${CMAKE_SOURCE_DIR}/layout/mp-minicontroller.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} mp-minicontroller.edj)

ADD_CUSTOM_TARGET(mp-images.edj
		COMMAND edje_cc -id ${CMAKE_CURRENT_SOURCE_DIR}/images/music_player
		${CMAKE_SOURCE_DIR}/layout/mp-images.edc ${CMAKE_BINARY_DIR}/layout/mp-images.edj
		DEPENDS ${CMAKE_SOURCE_DIR}/layout/mp-images.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} mp-images.edj)

ADD_DEFINITIONS("-DPREFIX=\"${PREFIX}\"")
ADD_DEFINITIONS("-DRESDIR=\"${RESDIR}\"")
ADD_DEFINITIONS("-DDATADIR=\"${DATADIR}\"")
ADD_DEFINITIONS("-DLOCALEDIR=\"${LOCALEDIR}\"")
ADD_DEFINITIONS("-DICONDIR=\"${ICONDIR}\"")
ADD_DEFINITIONS("-DEDJDIR=\"${EDJDIR}\"")
ADD_DEFINITIONS("-DLIBDIR=\"${LIBDIR}\"")


INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${BINDIR})
INSTALL(FILES ${CMAKE_BINARY_DIR}/layout/mp-custom-winset-style.edj DESTINATION ${EDJDIR})
INSTALL(FILES ${CMAKE_BINARY_DIR}/layout/music.edj DESTINATION ${EDJDIR})
INSTALL(FILES ${CMAKE_BINARY_DIR}/layout/mp-library.edj DESTINATION ${EDJDIR})
INSTALL(FILES ${CMAKE_BINARY_DIR}/layout/mp-minicontroller.edj DESTINATION ${EDJDIR})
INSTALL(FILES ${CMAKE_BINARY_DIR}/layout/mp-images.edj DESTINATION ${EDJDIR})
INSTALL(FILES ${CMAKE_BINARY_DIR}/layout/mp-library.edj DESTINATION ${EDJDIR})
INSTALL(DIRECTORY DESTINATION ${DATADIR})

# install desktop file & icon
INSTALL(FILES ${CMAKE_SOURCE_DIR}/data/${PKG_NAME}.png DESTINATION ${CMAKE_DESKTOP_ICON_DIR})
INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/target_images/music_player DESTINATION ${RESDIR}/images)

#install xml file
INSTALL(FILES ${CMAKE_SOURCE_DIR}/org.tizen.music-player.xml DESTINATION ${DESKTOP_DIR}/packages)

# i18n
ADD_SUBDIRECTORY(po)

# Sound Player
ADD_SUBDIRECTORY(sound-player)

# Music ug for appcontrol
ADD_SUBDIRECTORY(ug-music-player-efl)

#install rule file
INSTALL(FILES ${CMAKE_SOURCE_DIR}/${PKG_NAME}.rule DESTINATION  /opt/etc/smack/accesses.d)
