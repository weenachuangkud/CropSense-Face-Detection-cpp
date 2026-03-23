#pragma once

#include<string>

enum class CropType {
    UpperBody = 1,
    Face = 2,
    FullBody = 3
};

struct CropConfig
{
    float top_margin;
    float bottom_margin;
    std::string error_folder;
    std::string debug_folder;
    std::string output_folder;
    CropType crop_type;
};
