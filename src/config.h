#pragma once
#include <string>

// Detection
constexpr float CONFIDENCE_LEVEL = 0.5f;

// Output resolutions
constexpr int OUTPUT_RES          = 1080;
constexpr int PREVIEW_OUTPUT_RES  = 256;
constexpr int PREVIEW_DEBUG_MAX_RES = 640;

// Minimum face bounding-box sizes (px)
constexpr int MIN_FACE_RES      = 96;
constexpr int MIN_UPPERBODY_RES = 64;
constexpr int MIN_FULLBODY_RES  = 32;

// Folders
const std::string INPUT_FOLDER              = "input";
const std::string OUTPUT_UPPERBODY_FOLDER   = "output/upperbody_cropped";
const std::string DEBUG_UPPERBODY_FOLDER    = "output/upperbody_debug";
const std::string OUTPUT_FACE_FOLDER        = "output/face_cropped";
const std::string DEBUG_FACE_FOLDER         = "output/face_debug";
const std::string OUTPUT_FULLBODY_FOLDER    = "output/fullbody_cropped";
const std::string DEBUG_FULLBODY_FOLDER     = "output/fullbody_debug";
const std::string ERROR_FOLDER              = "output/error_images";
