#pragma once

#include<string>
#include "crop_config.h"

struct ImageProcessConfig {
    int output_res;
    int preview_output_res;
    int preview_debug_max_res;
    float confidence_level;
    int min_face_res;
    int min_upperbody_res;
    int min_fullbody_res;
};

void images_error(
    const std::string& image_path,
    const std::string& error_folder
);
int process_image(
    const std::string& image_path,
    const CropConfig& crop,
    const ImageProcessConfig& config
);
