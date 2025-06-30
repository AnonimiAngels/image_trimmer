#include "main.hpp" // IWYU pragma: keep

#define VERSION "v1.0.0"

auto gather_file_paths(const std::filesystem::path& in_path, const std::string& in_check_pattern) -> std::vector<std::string>
{
	std::vector<std::string> image_file_paths;

	// If in_path is a file, return it
	if (std::filesystem::is_regular_file(in_path))
	{
		image_file_paths.push_back(in_path.string());
		return image_file_paths;
	}

	for (const auto& entry : std::filesystem::directory_iterator(in_path))
	{
		spdlog::trace("Found: {}", entry.path().string());

		if (entry.is_regular_file())
		{
			const auto& path	 = entry.path();
			const auto extension = path.extension().string();
			const auto filename	 = path.filename().string();

			// If doenst match the regex, skip the file
			if (!std::regex_match(extension, std::regex(".*\\.(png|jpg|jpeg)")))
			{
				spdlog::trace("Skipping no regex match: {}", path.string());
				continue;
			}

			if (!in_check_pattern.empty() && fnmatch(in_check_pattern.c_str(), filename.c_str(), 0) != 0)
			{
				spdlog::trace("Skipping no fnmatch match: {}", path.string());
				continue;
			}

			spdlog::trace("Adding: {}", path.string());

			image_file_paths.push_back(path.string());
		}
	}

	return image_file_paths;
}

auto populate_images(std::vector<image*>& in_vector, const std::vector<std::string>& in_file_paths) -> void
{
	if (in_file_paths.empty())
	{
		spdlog::error("No images found.");
		exit(ENOENT);
	}

	for (const auto& file_path : in_file_paths)
	{
		try
		{
			auto* img = new image(file_path);

			img->set_extension(std::filesystem::path(file_path).extension().string());

			in_vector.push_back(img);
		}
		catch (const std::exception& e)
		{
			spdlog::error("Failed to load image: {}", e.what());
		}
	}
}

auto get_file_size(const std::vector<std::string>& in_vector) -> double_t
{
	double_t total_size = 0;

	for (const auto& path : in_vector)
	{
		total_size += static_cast<double_t>(std::filesystem::file_size(path));
	}

	return total_size;
}

auto calculate_new_rect(const std::vector<image*>& in_vector, progress& in_bar, uint8_t in_algorithm, bool in_apply_parity) -> rect
{
	in_bar.set_progress(0);
	in_bar.set_total(in_vector.size());

	// Initialize the bounding rect with extreme values
	spdlog::info("Calculating bounding box...");

	int32_t min_x = std::numeric_limits<int32_t>::max();
	int32_t min_y = std::numeric_limits<int32_t>::max();
	int32_t max_x = std::numeric_limits<int32_t>::min();
	int32_t max_y = std::numeric_limits<int32_t>::min();

	// Iterate over each image's bounding rectangle and find both the smallest and largest bounding box
	for (const auto& img : in_vector)
	{
		const auto& rect = img->get_image_boundings(in_algorithm);

		// Update min and max for x and y
		min_x = std::min(min_x, rect.get_x());
		min_y = std::min(min_y, rect.get_y());
		max_x = std::max(max_x, rect.get_x() + rect.get_width());
		max_y = std::max(max_y, rect.get_y() + rect.get_height());

		in_bar.print_progress();
	}

	// Set the bounding box to the calculated min/max values
	rect l_trim;
	l_trim.set_x(min_x);
	l_trim.set_y(min_y);
	l_trim.set_width(max_x - min_x);
	l_trim.set_height(max_y - min_y);

	if (in_apply_parity)
	{
		l_trim.set_x(l_trim.get_x() % 2 == 0 ? l_trim.get_x() : l_trim.get_x() - 1);
		l_trim.set_y(l_trim.get_y() % 2 == 0 ? l_trim.get_y() : l_trim.get_y() - 1);
		l_trim.set_width(l_trim.get_width() % 2 == 0 ? l_trim.get_width() : l_trim.get_width() + 1);
		l_trim.set_height(l_trim.get_height() % 2 == 0 ? l_trim.get_height() : l_trim.get_height() + 1);
	}

	spdlog::info("Trimming to: x: {}, y: {}, width: {}, height: {}", l_trim.get_x(), l_trim.get_y(), l_trim.get_width(), l_trim.get_height());

	return l_trim;
}

auto trim_images(std::vector<image*>& in_vector, progress& in_bar, const rect& in_rect) -> void
{
	std::mutex progress_mutex;

	auto trim_image = [&progress_mutex](image* ptr_img, progress& in_bar, const rect& in_rect)
	{
		ptr_img->rewrite_with_new_rect(in_rect);
		std::lock_guard<std::mutex> lock(progress_mutex);
		in_bar.print_progress();
	};

	in_bar.set_progress(0);
	in_bar.set_total(in_vector.size());

	const size_t size_vector = in_vector.size();

	// Get number of max threads
	const auto max_threads = std::thread::hardware_concurrency();
	const auto num_threads = std::min(max_threads, static_cast<uint32_t>(size_vector));

	spdlog::info("Trimming images...");

	std::vector<std::future<void>> futures;
	const size_t chunk_size = size_vector / num_threads;

	for (size_t i = 0; i < num_threads; ++i)
	{
		const size_t start = i * chunk_size;
		const size_t end   = (i + 1 == num_threads) ? size_vector : (i + 1) * chunk_size;

		futures.push_back(std::async(std::launch::async,
									 [&in_vector, start, end, &in_bar, &trim_image, &in_rect]()
									 {
										 for (size_t j = start; j < end; ++j)
										 {
											 trim_image(in_vector[j], in_bar, in_rect);
										 }
									 }));
	}

	for (auto& future : futures)
	{
		future.get();
	}
}

auto compress_images(std::vector<image*>& in_vector, progress& in_bar) -> void
{
	std::mutex progress_mutex;

	auto compress_image = [&progress_mutex](image* ptr_img, progress& in_bar)
	{
		ptr_img->perform_compresion();
		std::lock_guard<std::mutex> lock(progress_mutex);
		in_bar.print_progress();
	};

	in_bar.set_progress(0);
	in_bar.set_total(in_vector.size());

	const size_t size_vector = in_vector.size();

	// Get number of max threads
	const auto max_threads = std::thread::hardware_concurrency();
	const auto num_threads = std::min(max_threads, static_cast<uint32_t>(size_vector));

	std::vector<std::future<void>> futures;
	const size_t chunk_size = size_vector / num_threads;

	spdlog::info("Compressing images...");

	for (size_t i = 0; i < num_threads; ++i)
	{
		const size_t start = i * chunk_size;
		const size_t end   = (i + 1 == num_threads) ? size_vector : (i + 1) * chunk_size;

		futures.push_back(std::async(std::launch::async,
									 [&in_vector, start, end, &in_bar, &compress_image]()
									 {
										 for (size_t j = start; j < end; ++j)
										 {
											 compress_image(in_vector[j], in_bar);
										 }
									 }));
	}

	for (auto& future : futures)
	{
		future.get();
	}
}

auto main(int32_t argc, char* argv[]) -> int32_t
{
	std::filesystem::path path_dir;

	std::string check_pattern;

	bool perform_trim		= false;
	bool perform_compresion = false;
	bool perform_dry_run	= false;
	bool apply_parity		= false;

	uint8_t log_level = spdlog::level::err;
	uint8_t algorithm = 1;

	// Bulk trim images based on their alpha channel
	CLI::App app{std::format("Image trimmer\n\tVersion: {}\n", VERSION)};
	argv = app.ensure_utf8(argv);

	app.add_option("-p,--path", path_dir, "Path to the image file")->required();
	app.add_option("-c,--check", check_pattern, "Check pattern for the image files");

	app.add_flag("-t,--trim", perform_trim, "Flag: Trim the images");
	app.add_flag("-z,--compress", perform_compresion, "Flag:Compress the images");
	app.add_flag("-d,--dry-run", perform_dry_run, "Flag: Dry run, do not modify the images");
	app.add_flag("-e,--parity", apply_parity, "Flag: Adds to new image offsets to make it even.");

	app.add_option("-v,--verbose", log_level, "Set the log level, 0 for trace, 1 for debug, 2 for info, 3 for warn, 4 for error, 5 for critical, 6 for off");
	app.add_option("-a,--algorithm", algorithm, "Algorithm to use for bounding box calculation, 0 for flood fill, 1 for std algo");

	CLI11_PARSE(app, argc, argv);

	std::string path_str = path_dir.string();
	spdlog::set_level(static_cast<spdlog::level::level_enum>(log_level));

	// Remove spaces from the path
	path_str.erase(std::remove_if(path_str.begin(), path_str.end(), ::isblank), path_str.end());

	// Normalize the path
	path_dir = std::filesystem::path(path_str);
	path_dir = std::filesystem::absolute(path_dir);
	path_dir = path_dir.lexically_normal();

	spdlog::trace("Path: {}", path_dir.string());
	spdlog::trace("Check pattern: {}", check_pattern);
	spdlog::trace("Trim: {}", perform_trim);
	spdlog::trace("Compress: {}", perform_compresion);
	spdlog::trace("Log level: {}", log_level);
	spdlog::trace("Algorithm: {}", algorithm);

	if (!std::filesystem::exists(path_dir))
	{
		spdlog::error("Path does not exist: {}", path_dir.string());
		return ENOENT;
	}

	std::vector<image*> images;
	std::vector<std::string> image_file_paths = gather_file_paths(path_dir, check_pattern);
	const double_t prev_size				  = get_file_size(image_file_paths);

	populate_images(images, image_file_paths);

	spdlog::info("Found {} images", images.size());

	progress progress_bar;
	progress_bar.set_total(images.size());
	progress_bar.set_is_incremental(true);
	progress_bar.set_is_verbose(log_level <= spdlog::level::info);
	rect new_rect;

	if (perform_trim || perform_dry_run)
	{
		new_rect = calculate_new_rect(images, progress_bar, algorithm, apply_parity);
	}

	if (perform_trim)
	{
		trim_images(images, progress_bar, new_rect);
	}

	if (perform_compresion)
	{
		compress_images(images, progress_bar);

		const double_t new_size = get_file_size(image_file_paths);
		const double_t ratio	= (prev_size - new_size) / prev_size;

		spdlog::info("Compression ratio: {:.2f}% ({:.0f} -> {:.0f})", ratio * 100, prev_size, new_size);
	}

	spdlog::info("Cleaning up...");
	std::for_each(images.begin(), images.end(), [](auto* img) { delete img; });

	return 0;
}