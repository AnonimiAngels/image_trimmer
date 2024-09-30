#ifndef B5333D0E_48CE_4219_B181_64B8A587473B
#define B5333D0E_48CE_4219_B181_64B8A587473B

#include <cstdint>
#include <print>
#include <string>
#include <format>

class progress
{
  public:
	progress() = default;

	auto set_progress(int32_t in_progress) -> void { m_progress = in_progress; }
	auto set_total(size_t in_total) -> void { m_total = in_total; }
	auto set_fill_char(char in_fill_char) -> void { m_fill_char = in_fill_char; }
	auto set_is_incremental(bool in_is_incremental) -> void { m_is_incremental = in_is_incremental; }
	auto set_is_verbose(bool in_is_verbose) -> void { m_is_verbose = in_is_verbose; }

	[[nodiscard]] auto get_progress() const -> size_t { return m_progress; }
	[[nodiscard]] auto get_total() const -> size_t { return m_total; }
	[[nodiscard]] auto get_fill_char() const -> char { return m_fill_char; }
	[[nodiscard]] auto get_is_incremental() const -> bool { return m_is_incremental; }
	[[nodiscard]] auto get_is_verbose() const -> bool { return m_is_verbose; }

	// print total as a progress bar
	auto print_progress() -> void
	{
		if (m_is_incremental)
		{
			++m_progress;
		}

		if (!m_is_verbose)
		{
			return;
		}

		const auto percentage = static_cast<double>(m_progress) / static_cast<double>(m_total) * 100.0;
		const auto num_hashes = static_cast<int32_t>(percentage / 2.0);
		const auto num_spaces = 50 - num_hashes;

		std::string progress_bar = std::format("[{}{}] {:.2f}%", std::string(num_hashes, m_fill_char), std::string(num_spaces, ' '), percentage);
		std::print("\r{}", progress_bar);
		std::fflush(stdout);

		// If the progress is 100%, print a newline
		if (m_progress == m_total)
		{
			std::print("\n");
		}
	}

  private:
	size_t m_progress{};
	size_t m_total{};

	char m_fill_char{'#'};

	bool m_is_incremental{false};
	bool m_is_verbose{false};
};

#endif /* B5333D0E_48CE_4219_B181_64B8A587473B */
