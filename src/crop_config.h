#pragma once

#include<string>

struct CropConfig
{
    float top_margin;
    float bottom_margin;
    std::string debug_folder;
    std::string output_folder;
    int crop_type;
}
