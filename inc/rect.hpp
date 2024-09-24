#ifndef B2B19C29_3CE9_4DBE_8B07_82C4B6EB5634
#define B2B19C29_3CE9_4DBE_8B07_82C4B6EB5634

#include <cstdint>
#include <cstdlib>

class rect final
{
  public:
	rect() : m_start_x(0), m_start_y(0), m_end_x(0), m_end_y(0) {}
	rect(int32_t in_start_x, int32_t in_start_y, int32_t in_end_x, int32_t in_end_y) : m_start_x(in_start_x), m_start_y(in_start_y), m_end_x(in_end_x), m_end_y(in_end_y) {}

	[[nodiscard]] auto get_area() const -> int32_t
	{
		const int32_t width	 = m_end_x - m_start_x;
		const int32_t height = m_end_y - m_start_y;
		return width * height;
	}

	[[nodiscard]] auto get_perimeter() const -> int32_t
	{
		const int32_t width	 = m_end_x - m_start_x;
		const int32_t height = m_end_y - m_start_y;
		return 2 * (width + height);
	}

	[[nodiscard]] auto get_x() const -> int32_t { return m_start_x; }
	[[nodiscard]] auto get_y() const -> int32_t { return m_start_y; }
	[[nodiscard]] auto get_width() const -> int32_t { return m_end_x > m_start_x ? m_end_x - m_start_x : m_start_x - m_end_x; }
	[[nodiscard]] auto get_height() const -> int32_t { return m_end_y > m_start_y ? m_end_y - m_start_y : m_start_y - m_end_y; }

	auto set_x(int32_t in_x) -> void { m_start_x = in_x; }
	auto set_y(int32_t in_y) -> void { m_start_y = in_y; }
	auto set_width(int32_t in_width) -> void { m_end_x = m_start_x + in_width; }
	auto set_height(int32_t in_height) -> void { m_end_y = m_start_y + in_height; }

	auto operator>(const rect& in_rect) const -> bool { return get_perimeter() > in_rect.get_perimeter(); }

  private:
	int32_t m_start_x;
	int32_t m_start_y;
	int32_t m_end_x;
	int32_t m_end_y;
};

#endif /* B2B19C29_3CE9_4DBE_8B07_82C4B6EB5634 */
