################################################################################
# Check packages
################################################################################

include(FindPkgConfig)

pkg_check_modules (V4L2 REQUIRED libv4l2)

################################################################################
# Includes
################################################################################

include_directories(
	.
	include_directories(${V4L2_INCLUDE_DIR})
)

################################################################################
# Sources
################################################################################

set(SOURCES
	main.cpp
	CameraBackend.cpp
	CameraEnumerator.cpp
	Camera.cpp
)

################################################################################
# Targets
################################################################################

add_executable(${PROJECT_NAME} ${SOURCES})

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

################################################################################
# Libraries
################################################################################

set(XENBE_LIB xenbe)

target_link_libraries(${PROJECT_NAME}
	${XENBE_LIB}
	${V4L2_LIBRARY}
	pthread
)

################################################################################
# Debug display support
################################################################################

if (WITH_DBG_DISPLAY)
	pkg_check_modules (WAYLAND REQUIRED wayland-client)
	pkg_check_modules (DRM REQUIRED libdrm)

	add_definitions(-DWITH_DBG_DISPLAY)

	include_directories(
		displ_be/src/displayBackend
		displ_be/src/displayBackend/wayland
		${DRM_INCLUDE_DIRS}
	)

	add_subdirectory(displ_be/src/displayBackend/wayland)

	target_link_libraries(${PROJECT_NAME}
		display_wayland
	)
endif()
