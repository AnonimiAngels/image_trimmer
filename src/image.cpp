#include "image.hpp"

#include "compress.hpp"

#include <spdlog/spdlog.h>
#include <stdexcept>
#include <queue>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.hpp"
#include "stb_image_write.hpp"

auto image::flood_fill() -> rect
{
	if (m_trim_rect.get_area() > 0)
	{
		return m_trim_rect;
	}

	// Define boundary limits
	int32_t x_min = std::numeric_limits<int32_t>::max();
	int32_t x_max = std::numeric_limits<int32_t>::min();
	int32_t y_min = std::numeric_limits<int32_t>::max();
	int32_t y_max = std::numeric_limits<int32_t>::min();

	struct point
	{
		int32_t x;
		int32_t y;
	};

	// Array for tracking visited pixels
	std::vector<std::vector<bool>> visited(m_rect.get_height(), std::vector<bool>(m_rect.get_width(), false));

	// 4-directional deltas (up, down, left, right)
	const std::array<point, 4> directions = {point{-1, 0}, point{1, 0}, point{0, -1}, point{0, 1}};

	// Find the first non-transparent pixel to start flood fill
	bool found_seed = false;
	point seed;

	for (int32_t pos_y = 0; pos_y < m_rect.get_height() && !found_seed; ++pos_y)
	{
		for (int32_t pos_x = 0; pos_x < m_rect.get_width() && !found_seed; ++pos_x)
		{
			const auto alpha = m_data[(pos_y * m_rect.get_width() + pos_x) * m_channels + 3];
			if (alpha > 0)
			{
				seed	   = {pos_x, pos_y};
				found_seed = true;
			}
		}
	}

	if (!found_seed)
	{
		// If no non-transparent pixel is found, return an empty rectangle
		return {0, 0, 0, 0};
	}

	// BFS queue for flood fill
	std::queue<point> queue;
	queue.push(seed);
	visited[seed.y][seed.x] = true;

	// Start flood fill
	while (!queue.empty())
	{
		point current = queue.front();
		queue.pop();

		// Update bounding box
		x_min = std::min(x_min, current.x);
		x_max = std::max(x_max, current.x);
		y_min = std::min(y_min, current.y);
		y_max = std::max(y_max, current.y);

		// Explore neighbors
		for (const auto& dir : directions)
		{
			int32_t new_x = current.x + dir.x;
			int32_t new_y = current.y + dir.y;

			if (new_x >= 0 && new_x < m_rect.get_width() && new_y >= 0 && new_y < m_rect.get_height() && !visited[new_y][new_x])
			{

				const auto alpha = m_data[(new_y * m_rect.get_width() + new_x) * m_channels + 3];

				if (alpha > 0)
				{
					queue.push({new_x, new_y});
					visited[new_y][new_x] = true;
				}
			}
		}
	}

	m_trim_rect.set_x(x_min);
	m_trim_rect.set_y(y_min);
	m_trim_rect.set_width(x_max - x_min + 1);
	m_trim_rect.set_height(y_max - y_min + 1);

	return m_trim_rect;
}

auto image::std_algo() -> rect
{
	int32_t x_min = std::numeric_limits<int32_t>::max();
	int32_t x_max = std::numeric_limits<int32_t>::min();
	int32_t y_min = std::numeric_limits<int32_t>::max();
	int32_t y_max = std::numeric_limits<int32_t>::min();

	for (int32_t pos_y = 0; pos_y < m_rect.get_height(); ++pos_y)
	{
		for (int32_t pos_x = 0; pos_x < m_rect.get_width(); ++pos_x)
		{
			const auto alpha = m_data[(pos_y * m_rect.get_width() + pos_x) * m_channels + 3];

			if (alpha > 0)
			{
				x_min = std::min(x_min, pos_x);
				x_max = std::max(x_max, pos_x);
				y_min = std::min(y_min, pos_y);
				y_max = std::max(y_max, pos_y);
			}
		}
	}

	rect l_rect;
	l_rect.set_x(x_min);
	l_rect.set_y(y_min);
	l_rect.set_width(x_max - x_min + 1);
	l_rect.set_height(y_max - y_min + 1);

	return l_rect;
}

auto image::write_data(std::vector<uint8_t>& out_data, const rect& in_rect) -> void
{
	const auto new_x	  = static_cast<size_t>(in_rect.get_x());
	const auto new_y	  = static_cast<size_t>(in_rect.get_y());
	const auto new_width  = static_cast<size_t>(in_rect.get_width());
	const auto new_height = static_cast<size_t>(in_rect.get_height());

	const auto old_width = m_rect.get_width();

	size_t old_index = 0;
	size_t new_index = 0;

	for (size_t pos_y = 0; pos_y < new_height; ++pos_y)
	{
		old_index = ((new_y + pos_y) * old_width + new_x) * m_channels;
		new_index = pos_y * new_width * m_channels;

		std::copy(m_data.begin() + static_cast<ptrdiff_t>(old_index), m_data.begin() + static_cast<ptrdiff_t>(old_index + new_width * m_channels),
				  out_data.begin() + static_cast<ptrdiff_t>(new_index));
	}
}
image::image(const std::string_view& file_path) : m_file_path(file_path)
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

auto image::rewrite_with_new_rect(const rect& new_rect) -> void
{
	std::vector<uint8_t> new_data;
	new_data.resize(static_cast<size_t>(new_rect.get_width() * new_rect.get_height()) * m_channels);

	write_data(new_data, new_rect);

	stbi_write_png(m_file_path.c_str(), new_rect.get_width(), new_rect.get_height(), static_cast<int32_t>(m_channels), new_data.data(),
				   static_cast<int32_t>(new_rect.get_width() * m_channels));
}

auto image::perform_compresion() -> void
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

auto image::get_image_boundings(uint8_t idx_algorithm) -> rect
{
	switch (idx_algorithm)
	{
	case 0:
		return flood_fill();
	default:
		return std_algo();
	}
}
