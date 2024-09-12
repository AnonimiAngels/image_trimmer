# Function to collect all subdirectories
function(collect_subdirectories BASE_DIR OUTPUT_VAR)
	string(REPLACE "\\" "/" BASE_DIR ${BASE_DIR})  # Replace backslashes with forward slashes for Windows compatibility

	if(NOT IS_DIRECTORY ${BASE_DIR})
		message(FATAL_ERROR "BASE_DIR: ${BASE_DIR} is not a valid directory")
	endif()

	file(GLOB_RECURSE ENTRIES RELATIVE ${BASE_DIR} ${BASE_DIR}/*)
	set(DIRS "")

	# Since GLOB_RECURSE returns files and not directories we strip the file name from the path
	foreach(FILE ${ENTRIES})
		get_filename_component(DIR ${FILE} DIRECTORY)
		list(APPEND DIRS ${BASE_DIR}${DIR})
	endforeach()
	list(REMOVE_DUPLICATES DIRS)
	set(${OUTPUT_VAR} ${DIRS} PARENT_SCOPE)
endfunction()

function(build_executable EXECUTABLE_NAME SOURCES INCLUDE_DIRS LIBRARIES)
	add_executable(${EXECUTABLE_NAME} ${SOURCES})
	target_include_directories(${EXECUTABLE_NAME} PRIVATE ${INCLUDE_DIRS})
	target_link_libraries(${EXECUTABLE_NAME} PRIVATE ${LIBRARIES})
endfunction()