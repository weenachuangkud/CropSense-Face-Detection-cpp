#pragma once

#include "crop_config.h"

struct PreviewConfig {
    bool show_preview;
    bool parallel;
};

CropConfig select_option();
PreviewConfig preview_window();
void clean_output(
    const std::string& output_folder,
    const std::string& debug_output,
    const std::string& error_folder
);
