#include "main.hpp" // IWYU pragma: keep

#define VERSION "v1.0.0"

auto gather_file_paths(const std::filesystem::path& in_path, const std::string& in_check_pattern) -> std::vector<std::string>
{
	std::vector<std::string> image_file_paths;

	for (const auto& entry : std::filesystem::directory_iterator(in_path))
	{
		if (entry.is_regular_file())
		{
			const auto& path	 = entry.path();
			const auto extension = path.extension().string();
			const auto filename	 = path.filename().string();

			// If doenst match the regex, skip the file
			if (!std::regex_match(extension, std::regex(".*\\.(png|jpg|jpeg)")))
			{
				continue;
			}

			if (!in_check_pattern.empty() && fnmatch(in_check_pattern.c_str(), filename.c_str(), 0) != 0)
			{
				continue;
			}

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

auto calculate_new_rect(const std::vector<image*>& in_vector, progress& in_bar) -> rect
{
	in_bar.set_progress(0);
	in_bar.set_total(in_vector.size());

	rect new_trim_rect = {0, 0, 0, 0};

	spdlog::info("Calculating biggest bounding box...");
	for (const auto& img : in_vector)
	{
		const auto& rect = img->get_image_boundings();

		if (rect > new_trim_rect)
		{
			new_trim_rect = rect;
		}

		in_bar.print_progress();
	}

	return new_trim_rect;
}

auto trim_images(std::vector<image*>& in_vector, const rect& in_box, progress& in_bar) -> void
{
	in_bar.set_progress(0);
	in_bar.set_total(in_vector.size());

	spdlog::info("Trimming images...");
	for (auto& img : in_vector)
	{
		img->rewrite_with_new_rect(in_box);
		in_bar.print_progress();
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

	spdlog::info("Compressing images, using {} threads", num_threads);

	std::vector<std::future<void>> futures;
	const size_t chunk_size = size_vector / num_threads;

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

	// Bulk trim images based on their alpha channel
	CLI::App app{std::format("Image trimmer\n\tVersion: {}\n", VERSION)};
	argv = app.ensure_utf8(argv);

	app.add_option("-p,--path", path_dir, "Path to the image file")->required();
	app.add_option("-c,--check", check_pattern, "Check pattern for the image files");

	app.add_flag("-t,--trim", perform_trim, "Trim the images");
	app.add_flag("-z,--compress", perform_compresion, "Compress the images");

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

	std::vector<image*> images;
	std::vector<std::string> image_file_paths = gather_file_paths(path_dir, check_pattern);
	const double_t prev_size				  = get_file_size(image_file_paths);

	populate_images(images, image_file_paths);

	spdlog::info("Found {} images", images.size());

	progress progress_bar;
	progress_bar.set_total(images.size());
	progress_bar.set_is_incremental(true);

	if (perform_trim)
	{
		rect new_rect = calculate_new_rect(images, progress_bar);
		trim_images(images, new_rect, progress_bar);
	}

	if (perform_compresion)
	{
		compress_images(images, progress_bar);

		const double_t new_size = get_file_size(image_file_paths);
		const double_t ratio	= (prev_size - new_size) / prev_size;

		spdlog::info("Compression ratio: {:.2f}%", ratio * 100);
	}

	spdlog::info("Cleaning up...");
	std::for_each(images.begin(), images.end(), [](auto* img) { delete img; });

	return 0;
}