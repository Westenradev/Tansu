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
	src/ItemRowView.cpp
	src/ShelfModel.cpp
	src/Settings.cpp
)

target_include_directories(Tansu PRIVATE src)

target_link_libraries(Tansu be tracker device translation localestub)

add_custom_command(TARGET Tansu POST_BUILD
	COMMAND rc -o ${CMAKE_CURRENT_BINARY_DIR}/Tansu.rsrc
		${CMAKE_CURRENT_SOURCE_DIR}/resources/app.rdef
	COMMAND xres -o $<TARGET_FILE:Tansu> ${CMAKE_CURRENT_BINARY_DIR}/Tansu.rsrc
	COMMAND mimeset -f $<TARGET_FILE:Tansu>
	COMMENT "Adding resources and MIME signature to Tansu")

add_library(TrackerAddOn SHARED
	addon/TrackerAddOn.cpp
)

set_target_properties(TrackerAddOn PROPERTIES
	PREFIX ""
	SUFFIX ".so"
	OUTPUT_NAME "Add to Tansu"
)

target_link_libraries(TrackerAddOn be tracker)

install(TARGETS Tansu RUNTIME DESTINATION apps)
install(TARGETS TrackerAddOn LIBRARY DESTINATION "add-ons/Tracker")
