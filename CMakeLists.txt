cmake_minimum_required(VERSION 3.18.4)

set(QF_BUILD_QML_PLUGINS ON CACHE BOOL "Build with QML Plugins support")

project(quickbox LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

include(CTest)
set(CMAKE_SHARED_LIBRARY_PREFIX "") # we don't want CMake to prepend "lib" to our libraries, we prefer adding that ourselves

# Use ccache as a compiler launcher when available. This makes clean rebuilds and
# branch switches dramatically faster by reusing previously compiled objects.
# Opt out with -DUSE_CCACHE=OFF.
option(USE_CCACHE "Use ccache to speed up (re)builds when available" ON)
if(USE_CCACHE AND NOT CMAKE_CXX_COMPILER_LAUNCHER)
	find_program(CCACHE_PROGRAM ccache)
	if(CCACHE_PROGRAM)
		message(STATUS "ccache found, using it as compiler launcher: ${CCACHE_PROGRAM}")
		set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
		set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
	endif()
endif()

# COMMIT_SHA changes on every commit. Scope it to the single source file that
# reads it (Core/src/coreplugin.cpp) instead of defining it globally, otherwise
# every object's compile command would change on each commit and bust the whole
# ccache (and force a full rebuild). The definition is applied next to that file.

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic ${CMAKE_CXX_FLAGS}")
endif()

if(APPLE)
	# For macOS app bundles, set rpath to find bundled frameworks
	set(CMAKE_INSTALL_RPATH "@executable_path/../Frameworks")
	set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)  # Use install rpath at build time
	set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)  # Don't use build paths
	set(CMAKE_MACOSX_RPATH TRUE)
endif()

if (WIN32)
	if(CMAKE_BUILD_TYPE STREQUAL "Release")
		set(CMAKE_WIN32_EXECUTABLE ON) # Don't open a console window for Windows apps on Relase mode.
	endif()
endif (WIN32)

if (NOT TARGET libnecrolog)
	add_subdirectory(3rdparty/necrolog)
endif()

set(USE_QT6 ON)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Sql Qml Xml LinguistTools PrintSupport Svg SerialPort Multimedia Network)
set(Qt_FOUND ${Qt6_FOUND})

if(Qt_FOUND)
	set(CMAKE_AUTOMOC ON)
	set(CMAKE_AUTOUIC ON)
	set(CMAKE_AUTORCC ON)

	# Emit all .qm translation files into a single "translations" directory next to
	# the quickevent executable already at build time, so the app can load them from
	# QCoreApplication::applicationDirPath()/translations during development (this
	# mirrors the install layout). The individual library subprojects reference this
	# variable instead of hard-coding a path back to the application.
	set(QE_TRANSLATIONS_OUTPUT_DIR "${CMAKE_BINARY_DIR}/quickevent/app/quickevent/translations"
		CACHE INTERNAL "Build-tree output directory for .qm translation files")

	add_subdirectory(libqf)
	if(QF_LIB_ONLY)
		message(STATUS "Building libqf only")
	elseif(QF_QSQLMON_ONLY)
		message(STATUS "Building libqf and qsqlmon only")
		add_subdirectory(tools/qsqlmon)
	else()
		add_subdirectory(libsiut)
		add_subdirectory(libquickevent)
		add_subdirectory(quickevent)
		add_subdirectory(quickhttpd)
		add_subdirectory(quickshow)
		add_subdirectory(tools/qsqlmon)
	endif()
else()
	message(FATAL_ERROR "Qt not found")
endif()

# Directories where Qt Creator can find QML files.
# (Not needed for builds, but makes Qt Creator code completion happy.)
list(APPEND QML_IMPORT_PATH ${CMAKE_BINARY_DIR}/libqf/plugins)
#list(APPEND QML_IMPORT_PATH "/second/example/path/to/qml")

# Prevent adding duplicate values at each run of CMake.
list(REMOVE_DUPLICATES QML_IMPORT_PATH)

# The variable is cached in ${BUILD_DIR}/CMakeCache.txt. We need FORCE to
# change it there immediately. Also, add a comment to the cache file.
set(QML_IMPORT_PATH ${QML_IMPORT_PATH}
	CACHE STRING "Qt Creator extra qml import paths"
	FORCE
)
