#pragma once

#include<string>

struct CropConfig
{
    float top_margin;
    float bottom_margin;
    std::string error_folder;
    std::string debug_folder;
    std::string output_folder;
    // TODO: Use Enum type Instead of hard-coding crop_type
    int crop_type;
};
