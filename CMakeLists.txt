cmake_minimum_required(VERSION 3.16)
project(Tansu CXX)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Haiku")
	message(FATAL_ERROR "Tansu can only be built on Haiku OS.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra)

add_executable(Tansu
	src/main.cpp
	src/App.cpp
	src/ShelfWindow.cpp
	src/ShelfView.cpp
	src/ShelfListItem.cpp
	src/ShelfItem.cpp
	src/Settings.cpp
)

target_include_directories(Tansu PRIVATE src)

target_link_libraries(Tansu be tracker device translation localestub)

set(TANSU_RSRC ${CMAKE_CURRENT_BINARY_DIR}/Tansu.rsrc)

add_custom_command(OUTPUT ${TANSU_RSRC}
	COMMAND rc -o ${TANSU_RSRC} ${CMAKE_CURRENT_SOURCE_DIR}/resources/app.rdef
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/resources/app.rdef
	COMMENT "Compiling resources")
add_custom_target(TansuResources DEPENDS ${TANSU_RSRC})
add_dependencies(Tansu TansuResources)

# relink (and so re-run the POST_BUILD step) whenever the resources change
set_target_properties(Tansu PROPERTIES LINK_DEPENDS ${TANSU_RSRC})

add_custom_command(TARGET Tansu POST_BUILD
	COMMAND xres -o $<TARGET_FILE:Tansu> ${TANSU_RSRC}
	COMMAND mimeset -f $<TARGET_FILE:Tansu>
	COMMENT "Adding resources and MIME signature to Tansu")

add_library(TrackerAddOn SHARED
	addon/TrackerAddOn.cpp
)

set_target_properties(TrackerAddOn PROPERTIES
	PREFIX ""
	SUFFIX ""
	OUTPUT_NAME "Add to Tansu"
)

target_include_directories(TrackerAddOn PRIVATE src)

target_link_libraries(TrackerAddOn be tracker)

install(TARGETS Tansu RUNTIME DESTINATION apps)
install(TARGETS TrackerAddOn LIBRARY DESTINATION "add-ons/Tracker")
