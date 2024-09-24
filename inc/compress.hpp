#ifndef B15BF9B5_6F86_48C7_AB2D_35B8FD21054B
#define B15BF9B5_6F86_48C7_AB2D_35B8FD21054B

#include <cassert>
#include <format>
#include <string_view>
#include <stdexcept>

static auto compress_png(const std::string_view& file_path) -> void
{
	std::string cmd = std::format("optipng -o7 -zm1-9 -strip all -nb -clobber -quiet {}", file_path);
	if (std::system(cmd.c_str()) != 0)
	{
		throw std::runtime_error("Failed to compress PNG file.");
	}
}

static auto compress_jpeg(const std::string_view& file_path) -> void
{
	assert(false && "JPEG compression is not implemented yet.");
}

#endif /* B15BF9B5_6F86_48C7_AB2D_35B8FD21054B */
