#include <opencv2/opencv.hpp>
#include <nlohmann/json.hpp>

#include "crop_config.h"
#include "image_processing.h"
#include "user_input.h"

#include <fstream>
#include <string>
#include <atomic>
#include <future>
#include <thread>
#include <vector>
#include <filesystem>

#define DEFAULT_OUTPUT_FOLDER "output_folder"
#define DEFAULT_DEBUG_FOLDER "debug_folder"
#define DEFAULT_ERROR_FOLDER "error_folder"

// I have no ideas, what I'm doing

using json = nlohmann::json;
namespace fs = std::filesystem;

void processMultithread(const CropConfig& crop,
                        const ImageProcessConfig& process_config,
                        const PreviewConfig& preview_config)
{
    clean_output(crop.output_folder, crop.debug_folder, crop.error_folder);

    // Collect input files
    std::vector<fs::path> inputFiles;
    for (const auto& entry : fs::directory_iterator(crop.input_folder))
        if (entry.is_regular_file())
            inputFiles.push_back(entry.path());

    std::atomic<int> errorCount{0};
    std::vector<std::future<int>> futures;

    // Launch async tasks
    for (const auto& file : inputFiles)
    {
        futures.push_back(std::async(std::launch::async, [file, &crop, &process_config, &preview_config]()
        {
            return process_image(file.string(), crop, process_config, preview_config);
        }));
    }

    // Wait and collect results
    int processed = 0;
    for (auto& f : futures)
    {
        errorCount += f.get();
        processed++;
        std::cout << "\rProcessing: " << processed << "/" << inputFiles.size() << std::flush;
    }

    cv::destroyAllWindows();

    int totalOutputImages = 0;
    for (const auto& entry : fs::directory_iterator(crop.output_folder))
        if (entry.is_regular_file())
            totalOutputImages++;

    std::cout << "\nTotal images: "        << inputFiles.size() << "\n"
    << "Processed images: "      << inputFiles.size() - errorCount << "\n"
    << "Total faces detected: "  << totalOutputImages << "\n"
    << "Error images: "          << errorCount << "\n";
}

int main()
{
    std::ifstream f("config.json");
    json JSON_config = json::parse(f);

    // Load config
    ImageProcessConfig process_config;
    process_config.output_res           = JSON_config.value("output_res", 1080);
    process_config.preview_output_res   = JSON_config.value("preview_output_res", 256);
    process_config.preview_debug_max_res = JSON_config.value("preview_debug_max_res", 640);
    process_config.confidence_level     = JSON_config.value("confidence_level", 0.5f);
    process_config.min_face_res         = JSON_config.value("min_face_res", 96);
    process_config.min_upperbody_res    = JSON_config.value("min_upperbody_res", 64);
    process_config.min_fullbody_res     = JSON_config.value("min_fullbody_res", 32);

    CropConfig crop;
    crop.input_folder  = JSON_config.value("input_folder", "input");
    crop.output_folder = JSON_config.value("output_upperbody_folder", DEFAULT_OUTPUT_FOLDER);
    crop.debug_folder  = JSON_config.value("debug_upperbody_folder", DEFAULT_DEBUG_FOLDER);
    crop.error_folder  = JSON_config.value("error_folder", DEFAULT_ERROR_FOLDER);

    // Get user input
    CropConfig crop_selection = select_option();
    crop.crop_type    = crop_selection.crop_type;
    crop.top_margin   = crop_selection.top_margin;
    crop.bottom_margin = crop_selection.bottom_margin;

    // Set output folders based on crop type
    switch (crop.crop_type)
    {
        case CropType::UpperBody:
            crop.output_folder = JSON_config.value("output_upperbody_folder", DEFAULT_OUTPUT_FOLDER);
            crop.debug_folder  = JSON_config.value("debug_upperbody_folder", DEFAULT_DEBUG_FOLDER);
            break;
        case CropType::Face:
            crop.output_folder = JSON_config.value("output_face_folder", DEFAULT_OUTPUT_FOLDER);
            crop.debug_folder  = JSON_config.value("debug_face_folder", DEFAULT_DEBUG_FOLDER);
            break;
        case CropType::FullBody:
            crop.output_folder = JSON_config.value("output_fullbody_folder", DEFAULT_OUTPUT_FOLDER);
            crop.debug_folder  = JSON_config.value("debug_fullbody_folder", DEFAULT_DEBUG_FOLDER);
            break;
    }

    fs::create_directories(crop.input_folder);

    PreviewConfig preview_config = preview_window();

    if (preview_config.parallel)
    {
        processMultithread(crop, process_config, preview_config);
    }
    else
    {
        // Singlethread mode
        clean_output(crop.output_folder, crop.debug_folder, crop.error_folder);

        std::vector<fs::path> inputFiles;
        for (const auto& entry : fs::directory_iterator(crop.input_folder))
            if (entry.is_regular_file())
                inputFiles.push_back(entry.path());

        int errorCount = 0;
        int processed  = 0;
        for (const auto& file : inputFiles)
        {
            errorCount += process_image(file.string(), crop, process_config, preview_config);
            processed++;
            std::cout << "\rProcessing: " << processed << "/" << inputFiles.size() << std::flush;
        }

        cv::destroyAllWindows();

        int totalOutputImages = 0;
        for (const auto& entry : fs::directory_iterator(crop.output_folder))
            if (entry.is_regular_file())
                totalOutputImages++;

        std::cout << "\nTotal images: "       << inputFiles.size() << "\n"
        << "Processed images: "     << inputFiles.size() - errorCount << "\n"
        << "Total faces detected: " << totalOutputImages << "\n"
        << "Error images: "         << errorCount << "\n";
    }

    return 0;
}
