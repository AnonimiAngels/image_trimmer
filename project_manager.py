#!/usr/bin/env python3

import subprocess
import sys
import argparse
import platform
import os
import json

start_dir = subprocess.run(["pwd"], capture_output=True).stdout.decode().strip()

vscode_dir = start_dir + "/.vscode"
cache_dir = vscode_dir + "/.cache"
build_dir = start_dir + "/build"
script_dir = start_dir + "/scripts"
bin_dir = start_dir + "/bin"
docs_dir = start_dir + "/docs"

cache_file = cache_dir + "/cmake_sha1sum.txt"
cmake_file = script_dir + "/CMakeLists.txt"
database_file = build_dir + "/compile_commands.json"
clangd_database_file = vscode_dir + "/compile_commands.json"
launch_file = vscode_dir + "/launch.json"
tasks_file = vscode_dir + "/tasks.json"

project_name = "image_trimmer"

def print_debug(*args, **kwargs):
	"""Prints debug messages."""
	print(*args, **kwargs)

def cleanup():
	"""Removes build and cache folders."""
	# Remove directories
	subprocess.run(["rm", "-rf", build_dir, cache_dir, f"{bin_dir}/{project_name}", docs_dir])

	# Remove files
	subprocess.run(["rm", "-f", cache_file, clangd_database_file])

def create_dir():
	"""Creates build and cache folders and an empty checksum file."""
	# No need to create vscode folder it is created from cache_dir
	subprocess.run(["mkdir", "-p", build_dir, cache_dir, bin_dir, docs_dir])
	subprocess.run(["touch", cache_file])

def generate_cmake_flags(build_type):
	"""Generates flags for cmake command."""
	flags = ["-G", "Ninja"]

	flags.extend(["-B", build_dir]) # Set build dir to build folder
	flags.extend(["-S", script_dir]) # Set cmake file dir to scripts folder

	flags.extend(["-DPROJECT_DIR=" + start_dir])
	flags.extend(["-DPROJECT_NAME=" + project_name])

	flags.extend(["-DCMAKE_BUILD_TYPE=" + build_type])

	flags.extend(["-DCMAKE_CXX_COMPILER=clang++"])
	flags.extend(["-DCMAKE_C_COMPILER=clang"])

	flags.extend(["-DCMAKE_CXX_STANDARD=23"])


	flags.extend(["-DCMAKE_CXX_FLAGS=-march=native"])
	flags.extend(["-DCMAKE_C_FLAGS=-march=native"])

	flags.extend(["-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON"])
	flags.extend(["-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"])

	flags.extend(["-DCMAKE_CXX_COMPILER_CLANG_SCAN_DEPS=/usr/bin/clang-scan-deps-17"])

	if build_type == "release":
		flags.extend(["-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} -O3"])
		flags.extend(["-DCMAKE_C_FLAGS=${CMAKE_C_FLAGS} -O3"])
	else:
		flags.extend(["-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} -g"])
		flags.extend(["-DCMAKE_C_FLAGS=${CMAKE_C_FLAGS} -g"])

	return flags

def generate_vscode_files():
	"""Generates VSCode configuration files for debugging a C++ project."""

	if os.path.exists(launch_file):
		os.remove(launch_file)

	if os.path.exists(tasks_file):
		os.remove(tasks_file)

	create_dir()

	# Launch configuration for debugging
	launch_config = {
		"version": "0.2.0",
		"configurations": [
			{
				"name": "Debug",
				"type": "cppdbg",
				"request": "launch",
				"program": f"{bin_dir}/{project_name}",
				"args": ["-p", "./test_replica", "-c", "'*.png'", "-z", "-t"],
				"stopAtEntry": False,
				"cwd": f"{start_dir}",
				"environment": [],
				"externalConsole": False,
				"MIMode": "gdb",
				"setupCommands": [
					{
						"description": "Enable pretty-printing for gdb",
						"text": "-enable-pretty-printing",
						"ignoreFailures": True
					}
				],
				"preLaunchTask": "build",
				"miDebuggerPath": "/usr/bin/gdb",
				"miDebuggerArgs": "-q -ex quit; wait() { fg >/dev/null; }; /bin/gdb -q --interpreter=mi"
			}
		]
	}

	# Write launch.json
	with open(launch_file, "w") as f:
		f.write(json.dumps(launch_config, indent=4))

	# Task configuration for building call this script with arguments
	task_config = {
		"version": "2.0.0",
		"tasks": [
			{
				"label": "build",
				"type": "shell",
				"command": "python3",
				"args": ["project_manager.py", "-bt", "debug"],
				"group": {
					"kind": "build",
					"isDefault": True
				}
			}
		]
	}

	# Write tasks.json
	with open(tasks_file, "w") as f:
		f.write(json.dumps(task_config, indent=4))

def check_cmakelists_change():
	"""Checks if CMakeLists.txt has changed based on SHA1 sum. or build folder does not exist."""

	# If compile_commands.json does not exist, force compile
	force_compile = subprocess.run(["test", "-f", database_file]).returncode != 0

	if force_compile:
		return True

	current_sum = subprocess.run(["sha1sum", cmake_file], capture_output=True).stdout.decode().strip().split()[0]
	try:
		with open(cache_file, "r") as f:
			stored_sum = f.read().strip()
	except FileNotFoundError:
		return True
	return current_sum != stored_sum


def post_build():
	rsync_return_code = subprocess.run(["rsync", "-acz", "-delete", f"{start_dir}/test_1/", f"{start_dir}/test_replica/"]).returncode
	if rsync_return_code != 0:
		print("Rsync failed.")
		sys.exit(1)


def compile(build_type="Debug"):
	"""Runs cmake and make based on build type."""

	flag_regen = check_cmakelists_change()
	if flag_regen:
		print("CMakeLists.txt has been modified, regenerating build environment.")
		create_dir()
		flags = generate_cmake_flags(build_type)
		subprocess.run(["cmake"] + flags, cwd=script_dir)

		with open(cache_file, "w") as f:
			f.write(subprocess.run(["sha1sum", cmake_file], capture_output=True).stdout.decode().strip().split()[0])

	ninja_return_code = subprocess.run(["ninja"], cwd=build_dir).returncode
	if ninja_return_code != 0:
		print("Build failed.")
		sys.exit(1)

	post_build()

def generate_docs():
	"""Generates documentation using Doxygen."""

	file_exists = subprocess.run(["test", "-f", "Doxyfile"]).returncode == 0
	if not file_exists:
		print("Doxyfile does not exist, skipping documentation generation.")
		return

	create_dir()

	subprocess.run(["doxygen", "-q Doxyfile ./"], cwd=start_dir)
	subprocess.run(["xdg-open", docs_dir + "/html/index.html"])


def copy_compile_commands():
	"""Copies compile_commands.json to VSCode folder."""
	file_exists = subprocess.run(["test", "-f", database_file]).returncode == 0
	if not file_exists:
		print("compile_commands.json does not exist, skipping copy.")
		return

	subprocess.run(["rsync", "-acz", database_file, clangd_database_file])

def generate_project(build_type):
	# Capitalize first letter for cmake build type
	build_type = build_type.capitalize()
	compile(build_type)
	copy_compile_commands()

def run_project():
	"""Runs the project."""
	file_exists = subprocess.run(["test", "-f", f"{bin_dir}/{project_name}"]).returncode == 0
	if not file_exists:
		print("Executable does not exist, skipping run.")
		return

	subprocess.run(f"{bin_dir}/{project_name}", shell=True, cwd=start_dir)
	sys.exit(0)

def check_executable(executable):
	"""Check if the executable is installed."""
	return subprocess.run(["which", executable], stdout=subprocess.DEVNULL).returncode == 0

def test_required():
	"""Check if required executables are installed."""
	required_executables = ["cmake", "ninja", "clang++", "clang", "doxygen", "rsync", "sha1sum", "dot", "xdg-open", "gdb"]
	missing_executables = [exe for exe in required_executables if not check_executable(exe)]

	if missing_executables:
		print("Missing executables:")
		for exe in missing_executables:
			print(f"  - {exe}")

		print("sudo apt update -y && sudo apt install -y rsync ssh desktop-file-utils cmake ninja-build clang clangd clang-tools-17 clang-tidy clang-format llvm doxygen graphviz gdb libspdlog-dev build-essential")

		sys.exit(1)

def install_project():
	"""Installs the project."""
	file_exists = subprocess.run(["test", "-f", f"{bin_dir}/{project_name}"]).returncode == 0
	if not file_exists:
		print("Executable does not exist, skipping install.")
		return

	subprocess.run(["sudo", "cp", "-f", f"{bin_dir}/{project_name}", "/usr/local/bin/"])

def print_example():
	print("\nExample:")
	print("\t\tRebuild and run the project in debug mode:")
	print("\t\t\t./project_manager.py -cbrt debug")

def print_config():
	print("Configuration:")
	print("\t\tStart directory:\t\t", start_dir)
	print("\t\tVSCode directory:\t\t", vscode_dir)
	print("\t\tCache directory:\t\t", cache_dir)
	print("\t\tBuild directory:\t\t", build_dir)
	print("\t\tScript directory:\t\t", script_dir)
	print("\t\tDocs directory:\t\t", docs_dir)
	print("\t\tBin directory:\t\t", bin_dir)
	print("\t\tCache file:\t\t", cache_file)
	print("\t\tCMake file:\t\t", cmake_file)
	print("\t\tDatabase file:\t\t", database_file)
	print("\t\tClangd database file:\t\t", clangd_database_file)
	print("\t\tLaunch file:\t\t", launch_file)
	print("\t\tTasks file:\t\t", tasks_file)

	print("\t\tProject name:\t\t", project_name)

if __name__ == "__main__":

	parser = argparse.ArgumentParser()
	parser.add_argument("-c", "--clean", help="Remove build and cache folders.", action="store_true")
	parser.add_argument("-r", "--run", help="Run the project.", action="store_true")
	parser.add_argument("-t", "--type", help="Build type (release, debug).", type=str)
	parser.add_argument("-b", "--build", help="Build the project.", action="store_true")
	parser.add_argument("-d", "--docs", help="Generate documentation.", action="store_true")
	parser.add_argument("-i", "--info", help="Print configuration information.", action="store_true")
	parser.add_argument("-e", "--example", help="Print example usage.", action="store_true")
	parser.add_argument("-g", "--generate", help="Generate configuration files.", action="store_true")
	parser.add_argument("--install", help="Install the project.", action="store_true")
	args = parser.parse_args()

	test_required()

	if args.example:
		print_example()
		sys.exit(0)

	if args.info:
		print_config()
		sys.exit(0)

	if args.generate:
		generate_vscode_files()

	if args.clean:
		cleanup()

	if args.build:
		if not args.type:
			print("No build type specified, defaulting to debug.")
			args.type = "debug"

		generate_project(args.type)

	if args.docs:
		generate_docs()

	if args.run:
		run_project()

	if args.install:
		install_project()
