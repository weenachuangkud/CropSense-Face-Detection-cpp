#pragma once
#include <string>
#include <tuple>

struct CropOptions {
    double      topMargin;
    double      bottomMargin;
    std::string debugOutput;
    std::string outputFolder;
    int         cropType;   // 1=upperbody, 2=face, 3=fullbody
};

struct RunOptions {
    bool showPreview;
    bool parallel;
};

CropOptions selectOption();
RunOptions  previewWindow();
void        cleanOutput(const std::string& outputFolder,
                        const std::string& debugOutput,
                        const std::string& errorFolder);
