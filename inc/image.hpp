#ifndef AD314575_F224_47AC_B8F3_797A63D13BB7
#define AD314575_F224_47AC_B8F3_797A63D13BB7

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
		const auto* data = m_data;
		const auto& rect = m_rect;

		int32_t x_min = rect.get_width();
		int32_t x_max = 0;
		int32_t y_min = rect.get_height();
		int32_t y_max = 0;

		for (int32_t pos_y = 0; pos_y < rect.get_height(); ++pos_y)
		{
			for (int32_t pos_x = 0; pos_x < rect.get_width(); ++pos_x)
			{
				const auto alpha = data[(pos_y * rect.get_width() + pos_x) * m_channels + 3];

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

	~image() { stbi_image_free(m_data); }

	[[nodiscard]] auto get_data() const -> const uint8_t* { return m_data; }
	[[nodiscard]] auto get_rect() const -> const rect& { return m_rect; }
	[[nodiscard]] auto get_file_path() const -> const std::string& { return m_file_path; }
	[[nodiscard]] auto get_extension() const -> std::string { return m_extension; }

	auto set_extension(const std::string_view& extension) -> void { m_extension = extension; }

  private:
	rect m_rect;
	std::string m_file_path;
	std::string m_extension;

	int32_t m_channels;
	uint8_t* m_data{};
};

#endif /* AD314575_F224_47AC_B8F3_797A63D13BB7 */
