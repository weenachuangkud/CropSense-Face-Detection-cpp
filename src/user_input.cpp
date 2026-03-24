// user_input.cpp
#include "user_input.h"
#include <iostream>
#include<filesystem>
#include<thread>
#include<chrono>

namespace fs = std::filesystem;

CropConfig select_option() {
    int option;

    while (true) {
        std::cout << "Select a crop type:\n";
        std::cout << "1. Upper Body\n";
        std::cout << "2. Face\n";
        std::cout << "3. Full Body\n";
        std::cout << "Select: ";
        std::cin >> option;

        if (option < 1 || option > 3) {
            std::cout << "Invalid option. Please try again.\n\n";
        } else {
            break;
        }
    }

    CropConfig config;
    config.crop_type = (CropType)option;

    if (option == 1) {
        config.top_margin = 1.0f;
        config.bottom_margin = 3.0f;
        config.output_folder = "output/upperbody_cropped";
        config.debug_folder = "output/upperbody_debug";
    } else if (option == 2) {
        config.top_margin = 0.25f;
        config.bottom_margin = 0.25f;
        config.output_folder = "output/face_cropped";
        config.debug_folder = "output/face_debug";
    } else if (option == 3) {
        config.top_margin = 1.0f;
        config.bottom_margin = 12.0f;
        config.output_folder = "output/fullbody_cropped";
        config.debug_folder = "output/fullbody_debug";
    }

    return config;
}


// Are you sure this is not some bs
PreviewConfig preview_window() {
    std::string input;

    while (true) {
        std::cout << "Show preview window? [Y]es/[N]o: ";
        std::cin >> input;

        if (input == "y" || input == "Y") {
            PreviewConfig config;
            config.show_preview = true;
            config.parallel = false;
            return config;
        } else if (input == "n" || input == "N") {
            PreviewConfig config;
            config.show_preview = false;
            config.parallel = true;
            return config;
        } else {
            std::cout << "Invalid option. Please enter 'Y' or 'N'.\n\n";
        }
    }
}

int count_files(const std::string& folder) {
    int count = 0;
    for (const auto& entry : fs::recursive_directory_iterator(folder)) {
        if (fs::is_regular_file(entry)) count++;
    }
    return count;
}

void delete_files(const std::string& folder) {
    for (const auto& entry : fs::recursive_directory_iterator(folder)) {
        if (fs::is_regular_file(entry)) {
            fs::remove(entry.path());
        }
    }
}

void clean_output(const std::string& output_folder,
                  const std::string& debug_output,
                  const std::string& error_folder) {

    while (!fs::is_empty(output_folder)) {
        char input;
        std::cout << "Output folders are not empty, clean it? [Y]es/[N]o: ";
        std::cin >> input;

        if (input == 'y' || input == 'Y') {
            for (int i = 5; i > 0; i--) {
                std::cout << "\rDeleting files in " << i << "... [PRESS CTRL+C TO CANCEL]" << std::flush;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            std::cout << "\n";

            delete_files(output_folder);
            delete_files(debug_output);
            delete_files(error_folder);

            std::cout << "Done cleaning the output folder.\n\n";

        } else if (input == 'n' || input == 'N') {
            std::cout << "\n";
            break;
        } else {
            std::cout << "Invalid option. Please enter 'Y' or 'N'.\n\n";
        }
    }
}
