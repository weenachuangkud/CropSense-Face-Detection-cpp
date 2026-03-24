#include <opencv2/opencv.hpp>

#include <filesystem>
#include <string>

#include "image_processing.h"
#include "crop_config.h"

namespace fs = std::filesystem;

void link_image_error(const std::string& image_path,
                      const std::string& error_folder)
{
    fs::create_directories(error_folder);
    fs::path linkPath = fs::path(error_folder) / fs::path(image_path).filename();
    std::error_code ec;

    fs::create_hard_link(image_path, linkPath, ec);

    if (ec)
        fs::copy_file(image_path, linkPath, fs::copy_options::overwrite_existing);
}

int process_image(
    const std::string& image_path,
    const CropConfig& crop,
    const ImageProcessConfig& process_config,
    const PreviewConfig& preview_config)
{
    int error_count = 0;

    // Load DNN model
    cv::dnn::Net net = cv::dnn::readNetFromCaffe("deploy.prototxt.txt", "res10_300x300_ssd_iter_140000.caffemodel");

    // Get filename and extension
    fs::path filepath(image_path);
    std::string extension = filepath.extension().string();
    std::string filename  = filepath.stem().string();

    // Read image
    cv::Mat image = cv::imread(image_path);
    if (image.empty())
    {
        std::cout << "\rFailed to read image, skipping " << filename << extension << "\n";
        link_image_error(image_path, crop.error_folder);
        return ++error_count;
    }

    // Validate image format
    std::vector<std::string> supported_formats = { ".jpg", ".jpeg", ".png", ".webp" };
    bool is_supported = std::find(supported_formats.begin(), supported_formats.end(), extension) != supported_formats.end();
    if (!is_supported)
    {
        std::cout << "\rInvalid or unsupported image format, skipping " << filename << extension << "\n";
        return ++error_count;
    }

    // Check minimum resolution for face detection
    if (image.rows <= 300 && image.cols <= 300)
    {
        std::cout << "\rResolution too low for face detection, skipping " << filename << extension << "\n";
        link_image_error(image_path, crop.error_folder);
        return ++error_count;
    }

    // Create blob and run inference
    cv::Mat blob = cv::dnn::blobFromImage(
                                          image,
                                          1.0,
                                          cv::Size(300, 300),
                                          cv::Scalar(104.0, 177.0, 123.0));
    net.setInput(blob);
    cv::Mat detections = net.forward();

    // detections shape: [1, 1, N, 7]
    cv::Mat detections_mat(detections.size[2], detections.size[3], CV_32F, detections.ptr<float>());

    bool is_error = false;
    std::string error_msg;

    // First pass: confidence and resolution checks
    for (int face_index = 0; face_index < detections_mat.rows; face_index++)
    {
        float confidence = detections_mat.at<float>(face_index, 2);

        cv::Rect box;
        box.x      = static_cast<int>(detections_mat.at<float>(face_index, 3) * image.cols);
        box.y      = static_cast<int>(detections_mat.at<float>(face_index, 4) * image.rows);
        box.width  = static_cast<int>(detections_mat.at<float>(face_index, 5) * image.cols) - box.x;
        box.height = static_cast<int>(detections_mat.at<float>(face_index, 6) * image.rows) - box.y;

        int startX = box.x;
        int startY = box.y;
        int endX   = box.x + box.width;
        int endY   = box.y + box.height;

        if (confidence < process_config.confidence_level)
        {
            std::cout << "\rConfidence too low (" << static_cast<int>(confidence * 100) << "%), skipping face_"
            << face_index << " on " << filename << extension << "\n";
            error_msg = "CONFIDENCE LEVEL TOO LOW";
            link_image_error(image_path, crop.error_folder);
            is_error = true;
            error_count++;
            draw_rectangle(endX, startX, endY, startY, image, image_path, filename,
                           crop, process_config, preview_config, is_error, face_index, confidence, error_msg);
            break;
        }

        if (confidence > process_config.confidence_level)
        {
            bool res_too_small = false;
            switch (crop.crop_type)
            {
                case CropType::FullBody:
                    res_too_small = (box.width < process_config.min_fullbody_res || box.height < process_config.min_fullbody_res);
                    break;
                case CropType::Face:
                    res_too_small = (box.width < process_config.min_face_res || box.height < process_config.min_face_res);
                    break;
                case CropType::UpperBody:
                    res_too_small = (box.width < process_config.min_upperbody_res || box.height < process_config.min_upperbody_res);
                    break;
            }

            if (res_too_small)
            {
                std::string crop_name;
                switch (crop.crop_type)
                {
                    case CropType::FullBody:  crop_name = "fullbody";  break;
                    case CropType::Face:      crop_name = "face";      break;
                    case CropType::UpperBody: crop_name = "upperbody"; break;
                }
                std::cout << "\rFace resolution too small for " << crop_name << " crop, skipping face_"
                << face_index << " on " << filename << extension << "\n";
                error_msg = "FACE RESOLUTION IS TOO SMALL";
                link_image_error(image_path, crop.error_folder);
                is_error = true;
                error_count++;
                draw_rectangle(endX, startX, endY, startY, image, image_path, filename,
                               crop, process_config, preview_config, is_error, face_index, confidence, error_msg);
                break;
            }
        }
        break;
    }

    // Second pass: process valid detections
    for (int face_index = 0; face_index < detections_mat.rows; face_index++)
    {
        if (is_error) break;

        float confidence = detections_mat.at<float>(face_index, 2);
        if (confidence < process_config.confidence_level) break;

        int startX = static_cast<int>(detections_mat.at<float>(face_index, 3) * image.cols);
        int startY = static_cast<int>(detections_mat.at<float>(face_index, 4) * image.rows);
        int endX   = static_cast<int>(detections_mat.at<float>(face_index, 5) * image.cols);
        int endY   = static_cast<int>(detections_mat.at<float>(face_index, 6) * image.rows);

        is_error = draw_rectangle(endX, startX, endY, startY, image, image_path, filename,
                                  crop, process_config, preview_config, is_error, face_index, confidence, error_msg);
    }

    return error_count;
}

void preview(
    const cv::Mat& debug_image,
    const cv::Mat& resized_image,
    const int& preview_output_res,
    const int& preview_debug_max_res,
    bool is_error)
{
    double scale_factor;
    int window_width  = debug_image.cols;
    int window_height = debug_image.rows;

    if (window_width > preview_debug_max_res || window_height > preview_debug_max_res)
    {
        double width_scale  = static_cast<double>(preview_debug_max_res) / window_width;
        double height_scale = static_cast<double>(preview_debug_max_res) / window_height;
        scale_factor = std::min(width_scale, height_scale);
    }
    else
    {
        double width_scale  = static_cast<double>(preview_output_res) / window_width;
        double height_scale = static_cast<double>(preview_output_res) / window_height;
        scale_factor = std::max(width_scale, height_scale);
    }

    int new_width  = static_cast<int>(window_width  * scale_factor);
    int new_height = static_cast<int>(window_height * scale_factor);

    cv::Mat debug_preview_image;
    cv::resize(debug_image, debug_preview_image, cv::Size(new_width, new_height));

    cv::namedWindow("Debug Image", cv::WINDOW_AUTOSIZE);
    cv::imshow("Debug Image", debug_preview_image);
    cv::setWindowProperty("Debug Image", cv::WND_PROP_TOPMOST, 1);
    cv::setWindowProperty("Debug Image", cv::WND_PROP_ASPECT_RATIO, cv::WINDOW_KEEPRATIO);

    if (!is_error && !resized_image.empty())
    {
        cv::Mat output_preview_image;
        cv::resize(resized_image, output_preview_image, cv::Size(preview_output_res, preview_output_res));
        cv::namedWindow("Output Image", cv::WINDOW_AUTOSIZE);
        cv::imshow("Output Image", output_preview_image);
        cv::setWindowProperty("Output Image", cv::WND_PROP_TOPMOST, 1);
        cv::setWindowProperty("Output Image", cv::WND_PROP_ASPECT_RATIO, cv::WINDOW_KEEPRATIO);
    }

    cv::waitKey(250);
}

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
    const std::string& error_msg)
{
    // Calculate square region
    int square_size = std::min(endX - startX, endY - startY);
    int square_center_x = (endX + startX) / 2;
    int square_center_y = (endY + startY) / 2;
    int square_upper_left_x = square_center_x - square_size / 2;
    int square_upper_left_y = square_center_y - square_size / 2;
    int square_lower_right_x = square_upper_left_x + square_size;
    int square_lower_right_y = square_upper_left_y + square_size;
    int width_square = square_lower_right_x - square_upper_left_x;

    // Calculate margins
    int top_margin    = static_cast<int>(square_size * crop.top_margin);
    int bottom_margin = static_cast<int>(square_size * crop.bottom_margin);
    int left_margin   = static_cast<int>(square_size * crop.bottom_margin);
    int right_margin  = static_cast<int>(square_size * crop.bottom_margin);

    // Calculate upper body region
    int upper_left_x  = std::max(square_upper_left_x - left_margin, 0);
    int upper_left_y  = std::max(square_upper_left_y - top_margin, 0);
    int lower_right_x = std::min(square_lower_right_x + right_margin, image.cols);
    int lower_right_y = std::min(square_lower_right_y + bottom_margin, image.rows);

    // Adjust width
    int width_square_margin = lower_right_x - upper_left_x;
    if (width_square_margin > image.cols)
    {
        int shift = width_square_margin - image.cols;
        upper_left_x  = std::max(upper_left_x - shift, 0);
        lower_right_x = image.cols;
    }

    // Adjust height
    int height_square_margin = lower_right_y - upper_left_y;
    if (height_square_margin > image.rows)
    {
        int shift = height_square_margin - image.rows;
        upper_left_y  = std::max(upper_left_y - shift, 0);
        lower_right_y = image.rows;
    }

    // Calculate square with margin
    int square_margin_size        = std::min(lower_right_x - upper_left_x, lower_right_y - upper_left_y);
    int square_margin_center_x    = (lower_right_x + upper_left_x) / 2;
    int square_margin_upper_left_x = square_margin_center_x - square_margin_size / 2;
    int square_margin_upper_left_y = upper_left_y;
    int square_margin_lower_right_x = square_margin_upper_left_x + square_margin_size;

    // Align horizontally to face center
    int second_box_center_x = (startX + endX) / 2;
    int shift_amount = second_box_center_x - ((square_margin_upper_left_x + square_margin_lower_right_x) / 2);
    square_margin_upper_left_x  += shift_amount;
    square_margin_lower_right_x += shift_amount;

    // Clamp
    square_margin_upper_left_x  = std::max(square_margin_upper_left_x, 0);
    square_margin_lower_right_x = std::min(square_margin_lower_right_x, image.cols);
    square_margin_size = square_margin_lower_right_x - square_margin_upper_left_x;

    // Crop square region
    cv::Mat square_region = image(cv::Rect(
        square_margin_upper_left_x,
        square_margin_upper_left_y,
        square_margin_size,
        square_margin_size));

    // Save output
    cv::Mat resized_image;
    std::string output_image_path = crop.output_folder + "/" + filename + "_face_" + std::to_string(face_index) + ".png";

    if (square_region.empty())
    {
        is_error = true;
        const_cast<std::string&>(error_msg) = "NO FACE DETECTED";
    }
    else if (!is_error)
    {
        resized_image = cv::Mat();
        cv::resize(square_region, resized_image, cv::Size(process_config.output_res, process_config.output_res));
        cv::imwrite(output_image_path, resized_image);
    }

    // Debug image
    int thickness = std::max(image.cols / 128, 5);
    cv::Mat debug_image = image.clone();
    cv::rectangle(debug_image, cv::Point(startX, startY), cv::Point(endX, endY), cv::Scalar(0, 0, 255), thickness);
    cv::rectangle(debug_image, cv::Point(square_upper_left_x, square_upper_left_y), cv::Point(square_lower_right_x, square_lower_right_y), cv::Scalar(0, 255, 0), thickness);
    cv::rectangle(debug_image,
                  cv::Point(square_margin_upper_left_x, square_margin_upper_left_y),
                  cv::Point(square_margin_upper_left_x + square_margin_size, square_margin_upper_left_y + square_margin_size),
                  cv::Scalar(255, 165, 0), thickness);

    // Text labels
    double font_scale     = std::min(image.cols, image.rows) / 1000.0;
    int font_thickness    = std::max(1, std::min(image.cols, image.rows) / 500);

    // First label
    std::string resolution_text = std::to_string(image.cols) + "x" + std::to_string(image.rows) +
    " face_" + std::to_string(face_index) + "_" + std::to_string(width_square) + "px (" +
    std::to_string(static_cast<int>(confidence * 100)) + "%)";

    int baseline = 0;
    cv::Size text_size = cv::getTextSize(resolution_text, cv::FONT_HERSHEY_SIMPLEX, font_scale, font_thickness, &baseline);
    int bg_w = text_size.width + 10;
    int bg_h = text_size.height + 10;
    if (bg_w > debug_image.cols) bg_w = debug_image.cols;
    cv::Mat background(bg_h, bg_w, CV_8UC3, cv::Scalar(255, 255, 0));
    cv::putText(background, resolution_text, cv::Point(10, text_size.height + 5), cv::FONT_HERSHEY_SIMPLEX, font_scale, cv::Scalar(0, 0, 0), font_thickness);
    background.copyTo(debug_image(cv::Rect(0, 0, bg_w, bg_h)));

    // Second label
    std::string second_text    = is_error ? "ERROR " + error_msg : "OK PASSED";
    cv::Scalar second_bg_color = is_error ? cv::Scalar(0, 0, 255) : cv::Scalar(0, 255, 0);
    cv::Size second_text_size  = cv::getTextSize(second_text, cv::FONT_HERSHEY_SIMPLEX, font_scale, font_thickness, &baseline);
    int second_bg_w = second_text_size.width + 10;
    int second_bg_h = second_text_size.height + 10;
    if (second_bg_w > debug_image.cols) second_bg_w = debug_image.cols;

    int combined_height = bg_h + second_bg_h;
    if (combined_height > debug_image.rows)
        cv::resize(debug_image, debug_image, cv::Size(debug_image.cols, combined_height));

    cv::Mat second_background(second_bg_h, second_bg_w, CV_8UC3, second_bg_color);
    cv::putText(second_background, second_text, cv::Point(10, second_text_size.height + 5), cv::FONT_HERSHEY_SIMPLEX, font_scale, cv::Scalar(0, 0, 0), font_thickness);
    second_background.copyTo(debug_image(cv::Rect(0, bg_h, second_bg_w, second_bg_h)));

    // Save debug image
    std::string debug_image_path = crop.debug_folder + "/" + filename + "_face_" + std::to_string(face_index) + ".jpg";
    cv::imwrite(debug_image_path, debug_image);

    // Preview
    if (preview_config.show_preview)
        preview(debug_image, resized_image, process_config.preview_output_res, process_config.preview_debug_max_res, is_error);

    return is_error;
}
