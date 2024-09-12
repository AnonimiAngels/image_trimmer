# image_trimmer

This project provides a command-line tool to bulk trim images based on their alpha channels, using C++ libraries such as `stb_image`, `stb_image_write`, `spdlog`, and `CLI11`. It resizes images by trimming transparent pixels and applies the largest bounding box found among a group of images.

## Features

- Load images in `.png`, `.jpg`, and `.jpeg` formats.
- Automatically find the smallest rectangle that contains non-transparent pixels (bounding box) for each image.
- Normalize all images to the largest found bounding box.
- Save the modified images back to disk.
- Extensive logging with `spdlog`.

## Dependencies

The project uses the following libraries:

- [CLI11](https://github.com/CLIUtils/CLI11) for command-line argument parsing.
- [stb_image](https://github.com/nothings/stb) for loading image files.
- [stb_image_write](https://github.com/nothings/stb) for writing modified images.
- [spdlog](https://github.com/gabime/spdlog) for logging.

Ensure these libraries are included and correctly configured in your build environment.

## Build Instructions

### Prerequisites

- C++23 or later.
- [CMake](https://cmake.org/) for build configuration.
- [Ninja](https://ninja-build.org/) or any other build system compatible with CMake.
- The required libraries mentioned above.

### Steps

1. Clone the repository:

	```bash
	git clone <repository_url>
	cd <repository_directory>
	```

2. Use the project manager script to generate build files, build the project, and more:

	```bash
	project_manager.py --clear --generate --build --type Release
	```

4. Run the tool:

	```bash
	./bin/image_trimmer -p /path/to/image_directory
	```

## Note
- When the build is successful, the executable will be located in the `bin` directory.

## Usage

To run the tool, provide a path to a directory containing images:

```bash
./image_trimmer -p /path/to/image_directory
```

- The tool will look for .png, .jpg, and .jpeg files in the specified directory and its subdirectories.
- It will then trim the images by removing transparent pixels and normalize them to the largest bounding box found among the images.

## Command-Line Options

The tool supports the following command-line options:

- `-p, --path` : Path to the directory containing images.

## Example

```bash
./image_trimmer -p ./images
```

This command will process all images in the `images` directory `and` its `subdirectories`.

## Logging
- The tool uses spdlog for logging.
- Log messages include:
	- Number of images found.
	- The biggest bounding box dimensions.
	- Any errors encountered during image loading or processing.

## License
The project is licensed under the MIT License. See the [LICENSE](./LICENSE) file for more information.