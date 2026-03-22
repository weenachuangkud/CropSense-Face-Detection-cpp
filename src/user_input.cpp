#include "user_input.h"
#include "config.h"

#include <iostream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <string>
#include <algorithm>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Helper: delete every file inside a directory tree (non-recursive removal)
// ---------------------------------------------------------------------------
static void deleteFilesIn(const std::string& dir)
{
    if (!fs::exists(dir)) return;
    for (auto& entry : fs::recursive_directory_iterator(dir)) {
        if (fs::is_regular_file(entry.path())) {
            fs::remove(entry.path());
        }
    }
}

// ---------------------------------------------------------------------------
CropOptions selectOption()
{
    std::string option;
    while (true) {
        std::cout << "Select a crop type:\n"
        << "  1. Upper Body\n"
        << "  2. Face\n"
        << "  3. Full body (try to crop as much as possible)\n"
        << "Select: ";
        std::cin >> option;
        if (option == "1" || option == "2" || option == "3") break;
        std::cout << "Invalid option selected. Please try again.\n\n";
    }

    CropOptions opts{};
    if (option == "1") {
        opts.topMargin    = 1.0;
        opts.bottomMargin = 3.0;
        opts.debugOutput  = DEBUG_UPPERBODY_FOLDER;
        opts.outputFolder = OUTPUT_UPPERBODY_FOLDER;
        opts.cropType     = 1;
    } else if (option == "2") {
        opts.topMargin    = 0.25;
        opts.bottomMargin = 0.25;
        opts.debugOutput  = DEBUG_FACE_FOLDER;
        opts.outputFolder = OUTPUT_FACE_FOLDER;
        opts.cropType     = 2;
    } else {
        opts.topMargin    = 1.0;
        opts.bottomMargin = 12.0;
        opts.debugOutput  = DEBUG_FULLBODY_FOLDER;
        opts.outputFolder = OUTPUT_FULLBODY_FOLDER;
        opts.cropType     = 3;
    }
    return opts;
}

// ---------------------------------------------------------------------------
RunOptions previewWindow()
{
    RunOptions ro{};
    std::string answer;
    while (true) {
        std::cout << "Show preview window? [Y]es/[N]o: ";
        std::cin >> answer;
        // lowercase
        std::transform(answer.begin(), answer.end(), answer.begin(), ::tolower);
        if (answer == "y") {
            ro.showPreview = true;
            ro.parallel    = false;
            break;
        } else if (answer == "n") {
            ro.showPreview = false;
            ro.parallel    = true;
            break;
        } else {
            std::cout << "Invalid option. Please enter 'Y' or 'N'.\n\n";
        }
    }
    return ro;
}

// ---------------------------------------------------------------------------
void cleanOutput(const std::string& outputFolder,
                 const std::string& debugOutput,
                 const std::string& errorFolder)
{
    // Only prompt when the output folder is non-empty
    while (fs::exists(outputFolder) && !fs::is_empty(outputFolder)) {
        std::string answer;
        std::cout << "Output folders are not empty, clean it? [Y]es/[N]o: ";
        std::cin >> answer;
        std::transform(answer.begin(), answer.end(), answer.begin(), ::tolower);

        if (answer == "y") {
            for (int i = 5; i > 0; --i) {
                std::cout << "\rDeleting files in " << i << "... [PRESS CTRL+C TO CANCEL]" << std::flush;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            std::cout << '\n';

            deleteFilesIn(outputFolder);
            deleteFilesIn(debugOutput);
            deleteFilesIn(errorFolder);

            std::cout << "Done cleaning the output folder.\n\n";
            break;
        } else if (answer == "n") {
            std::cout << '\n';
            break;
        } else {
            std::cout << "Invalid option. Please enter 'Y' or 'N'.\n\n";
        }
    }
}
