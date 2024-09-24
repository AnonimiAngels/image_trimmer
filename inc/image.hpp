#ifndef AD314575_F224_47AC_B8F3_797A63D13BB7
#define AD314575_F224_47AC_B8F3_797A63D13BB7

#include <cstddef>
#include <cstdint>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.hpp"

#include "rect.hpp"
#include "compress.hpp"

class image
{
  private:
	rect m_rect;
	std::string m_file_path;
	std::string m_extension;

	size_t m_channels;
	std::vector<uint8_t> m_data;

  public:
	image(const std::string_view& file_path) : m_file_path(file_path)
	{
		int32_t width;
		int32_t height;
		int32_t channels;
		uint8_t* ptr_data = stbi_load(m_file_path.c_str(), &width, &height, &channels, 0);

		if (ptr_data == nullptr)
		{
			throw std::runtime_error("Failed to load image: " + m_file_path);
		}

		const size_t new_data_size = static_cast<size_t>(width * height) * channels;
		m_data.resize(new_data_size);
		m_data.reserve(new_data_size);

		std::memcpy(m_data.data(), ptr_data, new_data_size);

		m_channels = static_cast<size_t>(channels);
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
		size_t new_x	  = new_rect.get_x();
		size_t new_y	  = new_rect.get_y();
		size_t new_width  = new_rect.get_width() + new_x;
		size_t new_height = new_rect.get_height() + new_y;

		size_t old_width = m_rect.get_width();

		std::vector<uint8_t> new_data(new_width * new_height * m_channels);

		size_t new_index = 0;
		size_t old_index = 0;

		for (size_t pos_y = 0, pos_x; pos_y < new_height; ++pos_y)
		{
			for (pos_x = 0; pos_x < new_width; ++pos_x)
			{
				new_index = (pos_y * new_width + pos_x) * m_channels;
				old_index = ((new_y + pos_y) * old_width + (new_x + pos_x)) * m_channels;

				std::memcpy(new_data.data() + new_index, m_data.data() + old_index, m_channels);
			}
		}

		stbi_write_png(m_file_path.c_str(), static_cast<int32_t>(new_width), static_cast<int32_t>(new_height), static_cast<int32_t>(m_channels), new_data.data(),
					   static_cast<int32_t>(new_width * m_channels));
	}

	auto perform_compresion() -> void
	{
		if (m_extension == ".png")
		{
			compress_png(m_file_path);
		}
		else if (m_extension == ".jpg" || m_extension == ".jpeg")
		{
			compress_jpeg(m_file_path);
		}
		else
		{
			spdlog::error("Unsupported extension: {}", m_extension);
		}
	}

	auto get_image_boundings() -> rect
	{
		const auto& rect = m_rect;

		int32_t x_min = rect.get_width();
		int32_t x_max = 0;
		int32_t y_min = rect.get_height();
		int32_t y_max = 0;

		for (int32_t pos_y = 0; pos_y < rect.get_height(); ++pos_y)
		{
			for (int32_t pos_x = 0; pos_x < rect.get_width(); ++pos_x)
			{
				const auto alpha = m_data[(pos_y * rect.get_width() + pos_x) * m_channels + 3];

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

	[[nodiscard]] auto get_data() const -> const uint8_t* { return m_data.data(); }
	[[nodiscard]] auto get_rect() const -> const rect& { return m_rect; }
	[[nodiscard]] auto get_file_path() const -> const std::string& { return m_file_path; }
	[[nodiscard]] auto get_extension() const -> std::string { return m_extension; }

	auto set_extension(const std::string_view& extension) -> void { m_extension = extension; }
};

#endif /* AD314575_F224_47AC_B8F3_797A63D13BB7 */
