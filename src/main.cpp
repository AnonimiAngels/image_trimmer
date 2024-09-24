#include "main.hpp" // IWYU pragma: keep

#define VERSION "v1.0.0"

auto main(int32_t argc, char* argv[]) -> int32_t
{
	std::filesystem::path path_dir;
	std::string check_pattern;
	bool perform_compresion = false;

	// Bulk trim images based on their alpha channel
	CLI::App app{std::format("Image trimmer\n\tVersion: {}\n", VERSION)};
	argv = app.ensure_utf8(argv);

	app.add_option("-p,--path", path_dir, "Path to the image file")->required();
	app.add_option("-c,--check", check_pattern, "Check pattern for the image files");
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

	// Gather all images in the directory
	std::vector<std::string> image_file_paths;

	for (const auto& entry : std::filesystem::directory_iterator(path_dir))
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

			if (!check_pattern.empty() && fnmatch(check_pattern.c_str(), filename.c_str(), 0) != 0)
			{
				continue;
			}

			image_file_paths.push_back(path.string());
		}
	}

	if (image_file_paths.empty())
	{
		spdlog::error("No images found.");
		return ENOENT;
	}

	std::vector<image*> images;

	for (const auto& file_path : image_file_paths)
	{
		try
		{
			auto* img = new image(file_path);

			img->set_extension(std::filesystem::path(file_path).extension().string());

			images.push_back(img);
		}
		catch (const std::exception& e)
		{
			spdlog::error("Failed to load image: {}", e.what());
		}
	}

	spdlog::info("Found {} images", images.size());

	progress progress_bar;
	progress_bar.set_total(images.size());
	progress_bar.set_is_incremental(true);

	spdlog::info("Calculating biggest bounding box...");
	rect biggest_bounding_box;
	for (const auto& img : images)
	{
		const auto& rect = img->get_image_boundings();

		if (rect.get_area() > biggest_bounding_box.get_area())
		{
			biggest_bounding_box = rect;
		}

		progress_bar.print_progress();
	}

	spdlog::info("Biggest bounding box: x: {}, y: {}, width: {}, height: {}", biggest_bounding_box.get_x(), biggest_bounding_box.get_y(), biggest_bounding_box.get_width(),
				 biggest_bounding_box.get_height());

	progress_bar.set_progress(0);
	spdlog::info("Trimming images...");
	for (auto& img : images)
	{
		img->rewrite_with_new_rect(biggest_bounding_box);
		progress_bar.print_progress();
	}

	if (perform_compresion)
	{
		progress_bar.set_progress(0);
		spdlog::info("Compressing images...");
		for (auto& img : images)
		{
			img->perform_compresion();
			progress_bar.print_progress();
		}
	}

	spdlog::info("Cleaning up...");
	std::for_each(images.begin(), images.end(), [](auto* img) { delete img; });

	return 0;
}