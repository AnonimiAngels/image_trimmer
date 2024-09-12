cmake_minimum_required(VERSION 3.29)

include(./utilities.cmake)

project(${PROJECT_NAME})

collect_subdirectories(${PROJECT_DIR}/src SRC_DIRS)
collect_subdirectories(${PROJECT_DIR}/inc INC_DIRS)

# For each source directory, collect all source files
foreach(SRC_DIR ${SRC_DIRS})
	file(GLOB_RECURSE SRC_FILES ${SRC_DIR}/*.cpp ${SRC_DIR}/*.c)
	list(APPEND PROJECT_SOURCES ${SRC_FILES})
endforeach()

# For each include directory, collect all header files
foreach(INC_DIR ${INC_DIRS})
	file(GLOB_RECURSE INC_FILES ${INC_DIR}/*.hpp ${INC_DIR}/*.h)
	list(APPEND PROJECT_HEADERS ${INC_FILES})
endforeach()

find_package(spdlog REQUIRED)

set(LIBRARIES spdlog::spdlog)

# Set the output directory for the built executable
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Add include directories
include_directories(${INC_DIRS})

# If CONFIG is set to Release, add the -s flag to the linker
add_link_options("$<$<CONFIG:Release>:-s>")

# Create the executable
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

# Link any libraries
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})

# Post build commands (copying files, etc.)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	# Copy executable to the ${PROJECT_DIR}/bin/ directory
	COMMAND ${CMAKE_COMMAND} -E echo "Copying $<TARGET_FILE:${PROJECT_NAME}> to ${PROJECT_DIR}/bin/"
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_DIR}/bin/
)
