#ifndef B2B19C29_3CE9_4DBE_8B07_82C4B6EB5634
#define B2B19C29_3CE9_4DBE_8B07_82C4B6EB5634

#include <cstdint>
#include <cstdlib>

class rect
{
  public:
	rect() : m_start_x(0), m_start_y(0), m_width(0), m_height(0) {}
	rect(int32_t in_start_x, int32_t in_start_y, int32_t in_end_x, int32_t in_end_y)
		: m_start_x(in_start_x), m_start_y(in_start_y), m_width(in_end_x - in_start_x), m_height(in_end_y - in_start_y)
	{
	}

	[[nodiscard]] auto get_area() const -> int32_t { return m_width * m_height; }

	[[nodiscard]] auto get_perimeter() const -> int32_t { return (m_width + m_height) << 1; }

	[[nodiscard]] auto get_x() const -> int32_t { return m_start_x; }
	[[nodiscard]] auto get_y() const -> int32_t { return m_start_y; }
	[[nodiscard]] auto get_width() const -> int32_t { return m_width; }
	[[nodiscard]] auto get_height() const -> int32_t { return m_height; }

	auto set_x(int32_t in_x) -> void { m_start_x = in_x; }
	auto set_y(int32_t in_y) -> void { m_start_y = in_y; }
	auto set_width(int32_t in_width) -> void { m_width = in_width; }
	auto set_height(int32_t in_height) -> void { m_height = in_height; }

	auto operator>(const rect& in_rect) const -> bool { return get_perimeter() > in_rect.get_perimeter(); }

  private:
	int32_t m_start_x;
	int32_t m_start_y;

	int32_t m_width;
	int32_t m_height;
};

#endif /* B2B19C29_3CE9_4DBE_8B07_82C4B6EB5634 */
