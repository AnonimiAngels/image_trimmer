#ifndef AD314575_F224_47AC_B8F3_797A63D13BB7
#define AD314575_F224_47AC_B8F3_797A63D13BB7

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "rect.hpp"

class image
{
  private:
	rect m_rect;
	rect m_trim_rect;

	std::string m_file_path;
	std::string m_extension;

	size_t m_channels;
	std::vector<uint8_t> m_data;

	auto write_data(std::vector<uint8_t>& out_data, const rect& in_rect) -> void;
	auto flood_fill() -> rect;
	auto std_algo() -> rect;

  public:
	image(const std::string_view& file_path);

	image(const image&)					   = delete;
	image(image&&)						   = delete;
	auto operator=(const image&) -> image& = delete;

	auto rewrite_with_new_rect(const rect& new_rect) -> void;

	auto perform_compresion() -> void;

	auto get_image_boundings(uint8_t idx_algorithm) -> rect;

	[[nodiscard]] auto get_data() const -> const uint8_t* { return m_data.data(); }
	[[nodiscard]] auto get_rect() const -> const rect& { return m_rect; }
	[[nodiscard]] auto get_file_path() const -> const std::string& { return m_file_path; }
	[[nodiscard]] auto get_extension() const -> std::string { return m_extension; }

	auto set_extension(const std::string_view& extension) -> void { m_extension = extension; }
};

#endif /* AD314575_F224_47AC_B8F3_797A63D13BB7 */
