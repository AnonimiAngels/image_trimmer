#include "CLI11.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>

class rect final
{
  public:
	rect() : m_start_x(0), m_start_y(0), m_end_x(0), m_end_y(0) {}
	rect(int32_t in_start_x, int32_t in_start_y, int32_t in_end_x, int32_t in_end_y) : m_start_x(in_start_x), m_start_y(in_start_y), m_end_x(in_end_x), m_end_y(in_end_y) {}

	[[nodiscard]] auto get_area() const -> int32_t
	{
		const int32_t width	 = std::abs(m_end_x - m_start_x);
		const int32_t height = std::abs(m_end_y - m_start_y);
		return width * height;
	}

	[[nodiscard]] auto get_x() const -> int32_t { return m_start_x; }
	[[nodiscard]] auto get_y() const -> int32_t { return m_start_y; }
	[[nodiscard]] auto get_width() const -> int32_t { return m_end_x - m_start_x; }
	[[nodiscard]] auto get_height() const -> int32_t { return m_end_y - m_start_y; }

	auto set_x(int32_t in_x) -> void { m_start_x = in_x; }
	auto set_y(int32_t in_y) -> void { m_start_y = in_y; }
	auto set_width(int32_t in_width) -> void { m_end_x = m_start_x + in_width; }
	auto set_height(int32_t in_height) -> void { m_end_y = m_start_y + in_height; }

  private:
	int32_t m_start_x;
	int32_t m_start_y;
	int32_t m_end_x;
	int32_t m_end_y;
};

class image
{
  public:
	image(const std::string_view& file_path) : m_file_path(file_path)
	{
		int32_t width;
		int32_t height;
		int32_t channels;
		m_data = stbi_load(m_file_path.c_str(), &width, &height, &channels, 0);

		if (m_data == nullptr)
		{
			throw std::runtime_error("Failed to load image: " + m_file_path);
		}

		m_channels = channels;
		m_rect.set_x(0);
		m_rect.set_y(0);
		m_rect.set_width(width);
		m_rect.set_height(height);
	}

	image(const image&)					   = delete;
	image(image&&)						   = delete;
	auto operator=(const image&) -> image& = delete;

	auto rewrite_with_new_rect(const rect& new_rect) -> void
	{
		int32_t new_width  = new_rect.get_width() + new_rect.get_x();
		int32_t new_height = new_rect.get_height() + new_rect.get_y();

		std::vector<uint8_t> new_data(static_cast<size_t>(new_width * new_height * m_channels));

		// We need to copy the data from the old image to the new image
		// We start from new_rect.get_x() and new_rect.get_y() on old image and copy to 0, 0 on new image
		for (int32_t pos_y = 0; pos_y < new_height; ++pos_y)
		{
			for (int32_t pos_x = 0; pos_x < new_width; ++pos_x)
			{
				const int32_t old_x = new_rect.get_x() + pos_x;
				const int32_t old_y = new_rect.get_y() + pos_y;

				const int32_t new_index = (pos_y * new_width + pos_x) * m_channels;
				const int32_t old_index = (old_y * m_rect.get_width() + old_x) * m_channels;

				for (int32_t channel = 0; channel < m_channels; ++channel)
				{
					new_data[new_index + channel] = m_data[old_index + channel];
				}
			}
		}

		stbi_write_png(m_file_path.c_str(), new_width, new_height, m_channels, new_data.data(), new_width * m_channels);
	}

	~image() { stbi_image_free(m_data); }

	[[nodiscard]] auto get_data() const -> const uint8_t* { return m_data; }
	[[nodiscard]] auto get_rect() const -> const rect& { return m_rect; }
	[[nodiscard]] auto get_file_path() const -> const std::string& { return m_file_path; }

  private:
	rect m_rect;
	std::string m_file_path;
	int32_t m_channels;
	uint8_t* m_data{};
};

static auto get_image_boundings(const image& img) -> rect
{
	const auto* data = img.get_data();
	const auto& rect = img.get_rect();

	int32_t x_min = rect.get_width();
	int32_t x_max = 0;
	int32_t y_min = rect.get_height();
	int32_t y_max = 0;

	for (int32_t pos_y = 0; pos_y < rect.get_height(); ++pos_y)
	{
		for (int32_t pos_x = 0; pos_x < rect.get_width(); ++pos_x)
		{
			const auto alpha = data[(pos_y * rect.get_width() + pos_x) * 4 + 3];

			if (alpha > 0)
			{
				x_min = std::min(x_min, pos_x);
				x_max = std::max(x_max, pos_x);
				y_min = std::min(y_min, pos_y);
				y_max = std::max(y_max, pos_y);
			}
		}
	}

	return {x_min, y_min, x_max - x_min + 1, y_max - y_min + 1};
}

auto main(int32_t argc, char* argv[]) -> int32_t
{
	std::filesystem::path path_dir;

	// Bulk trim images based on their alpha channel
	CLI::App app{"Bulk trim images based on their alpha channel"};
	argv = app.ensure_utf8(argv);

	app.add_option("-p,--path", path_dir, "Path to the image file")->required();

	CLI11_PARSE(app, argc, argv);

	std::string path_str = path_dir.string();

	// Remove spaces from the path
	path_str.erase(std::remove_if(path_str.begin(), path_str.end(), ::isblank), path_str.end());

	// Normalize the path
	path_dir = std::filesystem::path(path_str);
	path_dir = std::filesystem::absolute(path_dir);
	path_dir = path_dir.lexically_normal();

	if (!std::filesystem::exists(path_dir))
	{
		spdlog::error("Path does not exist: {}", path_dir.string());
		return ENOENT;
	}

	if (!std::filesystem::is_directory(path_dir))
	{
		spdlog::error("Path is not a directory: {}", path_dir.string());
		return ENOTDIR;
	}

	std::vector<image*> images;
	// Gather all images in the directory

	for (const auto& entry : std::filesystem::directory_iterator(path_dir))
	{
		if (entry.is_regular_file())
		{
			const auto& path	 = entry.path();
			const auto extension = path.extension().string();

			if (extension == ".png" || extension == ".jpg" || extension == ".jpeg")
			{
				try
				{
					auto* img = new image(path.string());
					images.push_back(img);
				}
				catch (const std::exception& e)
				{
					spdlog::error("{}", e.what());
				}
			}
		}
	}

	spdlog::info("Found {} images", images.size());

	rect biggest_bounding_box;
	for (const auto& img : images)
	{
		const auto& rect = get_image_boundings(*img);

		if (rect.get_area() > biggest_bounding_box.get_area())
		{
			biggest_bounding_box = rect;
		}
	}

	spdlog::info("Found biggest bounding box: pos_x: {}, pos_y: {}, width: {}, height: {}", biggest_bounding_box.get_x(), biggest_bounding_box.get_y(),
				 biggest_bounding_box.get_width(), biggest_bounding_box.get_height());

	for (auto& img : images)
	{
		img->rewrite_with_new_rect(biggest_bounding_box);

		delete img;
	}

	spdlog::info("Done!");

	return 0;
}