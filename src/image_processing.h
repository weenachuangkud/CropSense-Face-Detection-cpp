#pragma once
#include <string>

// Returns number of errors encountered while processing a single image file.
int processImage(const std::string& imagePath,
                 const std::string& errorFolder,
                 const std::string& outputFolder,
                 const std::string& debugOutput,
                 int    outputRes,
                 int    previewOutputRes,
                 int    previewDebugMaxRes,
                 bool   showPreview,
                 int    cropType,
                 double topMarginValue,
                 double bottomMarginValue);
