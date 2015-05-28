CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
SET(PROJECT_NAME ug-music-player-efl)
PROJECT(${PROJECT_NAME} C CXX)

SET(SRCS
	src/ug-mp-efl.c
	src/ug-mp-track-list.c
	src/ug-mp-group-list.c
	src/ug-mp-library-view.c
	src/ug-mp-common.c
	src/ug-mp-widget.c
	${CMAKE_SOURCE_DIR}/src/common/mp-media-info.c
)

INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_SOURCE_DIR}/src/include
)

INCLUDE(FindPkgConfig)
pkg_check_modules(pkgs REQUIRED
	elementary
	ui-gadget-1
	dlog
	vconf
	)

FOREACH(flag ${pkgs_CFLAGS})
	SET(EXTRA_CXXFLAGS "${EXTRA_CXXFLAGS} ${flag}")
ENDFOREACH(flag)

SET(EXTRA_CXXFLAGS "${EXTRA_CXXFLAGS} -fvisibility=hidden -Wall")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CXXFLAGS}")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_CXX_FLAGS}")
SET(CMAKE_LDFLAGS "-Wl,-zdefs" )
ADD_DEFINITIONS("-DPREFIX=\"${CMAKE_INSTALL_PREFIX}\"")

ADD_LIBRARY(${PROJECT_NAME} SHARED ${SRCS})
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES SOVERSION 0.1.0)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${pkgs_LDFLAGS})

# set extra defines
SET(IMAGES_DIR "${UG_PREFIX}/res/images/ug-music-player-efl")
SET(UG_EDJ_NAME "${UG_PREFIX}/res/edje/ug-music-player-efl/layout/ug-music-player-efl.edj")
ADD_DEFINITIONS("-DPREFIX=\"${CMAKE_INSTALL_PREFIX}\"")
ADD_DEFINITIONS("-DIMAGES_DIR=\"${IMAGES_DIR}\"")
ADD_DEFINITIONS("-DUG_EDJ_NAME=\"${UG_EDJ_NAME}\"")
ADD_CUSTOM_TARGET(ug-music-player-efl.edj
		COMMAND edje_cc -id ${CMAKE_CURRENT_SOURCE_DIR}/images
		${CMAKE_CURRENT_SOURCE_DIR}/layout/ug-music-player-efl.edc ${CMAKE_BINARY_DIR}/layout/ug-music-player-efl.edj
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/layout/ug-music-player-efl.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} ug-music-player-efl.edj)

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${UG_PREFIX}/lib)
message(UG_PREFIX ${UG_PREFIX})
INSTALL(FILES ${CMAKE_BINARY_DIR}/layout/ug-music-player-efl.edj DESTINATION ${UG_PREFIX}/res/edje/${PROJECT_NAME})
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/ug-music-player-efl.xml DESTINATION ${DESKTOP_DIR}/packages)
