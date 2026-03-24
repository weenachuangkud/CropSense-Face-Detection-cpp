#pragma once
#include <opencv2/opencv.hpp>
#include <string>
#include "crop_config.h"
#include "user_input.h"

struct ImageProcessConfig {
    int output_res;
    int preview_output_res;
    int preview_debug_max_res;
    float confidence_level;
    int min_face_res;
    int min_upperbody_res;
    int min_fullbody_res;
};

void link_image_error(
    const std::string& image_path,
    const std::string& error_folder
);

int process_image(
    const std::string& image_path,
    const CropConfig& crop,
    const ImageProcessConfig& process_config,
    const PreviewConfig& preview_config
);

bool draw_rectangle(
    const int& endX,
    const int& startX,
    const int& endY,
    const int& startY,
    cv::Mat& image,
    const std::string& image_path,
    const std::string& filename,
    const CropConfig& crop,
    const ImageProcessConfig& process_config,
    const PreviewConfig& preview_config,
    bool is_error,
    int face_index,
    float confidence,
    const std::string& error_msg
);

void preview(
    const cv::Mat& debug_image,
    const cv::Mat& resized_image,
    const int& preview_output_res,
    const int& preview_debug_max_res,
    bool is_error
);
