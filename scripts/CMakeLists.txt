cmake_minimum_required(VERSION 3.29)

include(./utilities.cmake)
include(./CPM.cmake)

project(${PROJECT_NAME})

collect_subdirectories(${PROJECT_DIR}/src SRC_DIRS)
collect_subdirectories(${PROJECT_DIR}/inc INC_DIRS)

file(GLOB_RECURSE PROJECT_SOURCES ${SRC_DIRS}/*.cpp ${SRC_DIRS}/*.c)
file(GLOB_RECURSE PROJECT_HEADERS ${INC_DIRS}/*.hpp ${INC_DIRS}/*.h)

CPMAddPackage(
NAME Ccache.cmake
GITHUB_REPOSITORY TheLartians/Ccache.cmake
VERSION 1.2.5
)

include(FetchContent)

FetchContent_Declare(imgui
	QUIET
	GIT_REPOSITORY https://github.com/ocornut/imgui.git
	GIT_TAG v1.91.2
	GIT_SHALLOW TRUE
	FETCHCONTENT_TRY_FIND_PACKAGE_MODE ALWAYS
)

FetchContent_MakeAvailable(imgui)

file(GLOB IMGUI_SOURCES ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
	${imgui_SOURCE_DIR}/imgui.cpp ${imgui_SOURCE_DIR}/imgui_draw.cpp ${imgui_SOURCE_DIR}/imgui_widgets.cpp ${imgui_SOURCE_DIR}/imgui_tables.cpp
	${imgui_SOURCE_DIR}/imgui_demo.cpp)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)


find_package(spdlog REQUIRED)
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

set(LIBRARIES spdlog::spdlog uuid imgui glfw OpenGL::GL GLEW::GLEW)

# Set the output directory for the built executable
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Create the executable
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

# Add include directories
target_include_directories(${PROJECT_NAME} PRIVATE ${INC_DIRS})

# Link any libraries
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBRARIES})

# Post build commands (copying files, etc.)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	# Copy executable to the ${PROJECT_DIR}/bin/ directory
	COMMAND ${CMAKE_COMMAND} -E echo "Copying $<TARGET_FILE:${PROJECT_NAME}> to ${PROJECT_DIR}/bin/"
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_DIR}/bin/
)
