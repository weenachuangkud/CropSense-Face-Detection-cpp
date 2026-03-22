#include "image_processing.h"
#include "config.h"

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>

#include <filesystem>
#include <iostream>
#include <string>
#include <algorithm>
#include <set>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Windows-only: create a .lnk shortcut pointing at imagePath inside errorFolder.
// On non-Windows platforms we just copy the file instead.
// ---------------------------------------------------------------------------
static void imagesError(const std::string& imagePath, const std::string& errorFolder)
{
    #ifdef _WIN32
    // Use the Windows Script Host COM object via system() call as a fallback —
    // a proper COM implementation would require <windows.h> + IShellLink.
    // For portability we write a small PowerShell one-liner.
    std::string filename = fs::path(imagePath).filename().string();
    std::string lnkPath  = errorFolder + "/" + filename + ".lnk";
    std::string absTarget = fs::absolute(imagePath).string();

    std::string cmd =
    "powershell -NoProfile -Command \""
    "$s=(New-Object -COM WScript.Shell).CreateShortcut('" + lnkPath + "');"
    "$s.TargetPath='" + absTarget + "';"
    "$s.Save()\"";
    std::system(cmd.c_str());
    #else
    // On Linux/macOS, create a symbolic link (or just copy).
    std::string filename  = fs::path(imagePath).filename().string();
    std::string linkPath  = errorFolder + "/" + filename;
    std::error_code ec;
    fs::create_symlink(fs::absolute(imagePath), linkPath, ec);
    // ignore error if link already exists
    #endif
}

// ---------------------------------------------------------------------------
// Supported image extensions (imghdr equivalent)
// ---------------------------------------------------------------------------
static bool isSupportedImage(const std::string& path)
{
    static const std::set<std::string> supported = {".jpg",".jpeg",".png",".webp"};
    std::string ext = fs::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return supported.count(ext) > 0;
}

// ---------------------------------------------------------------------------
// Forward declaration
// ---------------------------------------------------------------------------
static bool drawRectangle(int endX, int startX, int endY, int startY,
                          double topMarginValue, double bottomMarginValue,
                          const cv::Mat& image,
                          int outputRes,
                          const std::string& outputFolder,
                          const std::string& outputImagePath,
                          const std::string& filename,
                          const std::string& imagePath,
                          const std::string& debugOutput,
                          bool showPreview,
                          int previewOutputRes,
                          int previewDebugMaxRes,
                          bool isError,
                          int i,
                          float confidence,
                          const std::string& errorMsg);

static void preview(const cv::Mat& debugImage,
                    const cv::Mat& resizedImage,
                    int previewOutputRes,
                    int previewDebugMaxRes,
                    bool isError);

// ---------------------------------------------------------------------------
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
                 double bottomMarginValue)
{
    int errorCount = 0;

    static cv::dnn::Net net = cv::dnn::readNetFromCaffe(
        "deploy.prototxt.txt", "res10_300x300_ssd_iter_140000.caffemodel");

    std::string stem = fs::path(imagePath).stem().string();
    std::string ext  = fs::path(imagePath).extension().string();

    if (!isSupportedImage(imagePath)) {
        std::cout << "\rInvalid or unsupported image format, skipping " << stem << ext << "\n";
        ++errorCount;
        return errorCount;
    }

    cv::Mat image = cv::imread(imagePath);
    if (image.empty()) {
        std::cout << "\rCould not read image, skipping " << stem << ext << "\n";
        ++errorCount;
        return errorCount;
    }

    if (image.rows <= 300 || image.cols <= 300) {
        std::cout << "\rThe resolution is too low for face detection, skipping " << stem << ext << "\n";
        imagesError(imagePath, errorFolder);
        ++errorCount;
        return errorCount;
    }

    // Prepare blob and run forward pass
    cv::Mat resized300;
    cv::resize(image, resized300, {300, 300});
    cv::Mat blob = cv::dnn::blobFromImage(resized300, 1.0, {300, 300},
                                          {104.0, 177.0, 123.0});
    net.setInput(blob);
    cv::Mat detections = net.forward();   // shape: 1×1×N×7

    // detections.ptr<float>(0,0) is a flat array; use reshape for convenience
    // Shape after reshape: (N, 7)
    cv::Mat det = detections.reshape(1, detections.size[2]);  // N×7

    int N = det.rows;
    bool isError = false;
    std::string outputImagePath;
    std::string errorMsg;

    // ---- First pass: validate the first high-confidence detection ----------
    for (int i = 0; i < N; ++i) {
        float conf = det.at<float>(i, 2);
        if (conf < CONFIDENCE_LEVEL) {
            std::cout << "\rConfidence level too low (" << static_cast<int>(conf * 100)
            << "%), skipping face_" << i << " on " << stem << ext << "\n";
            errorMsg = "CONFIDENCE LEVEL TOO LOW";
            imagesError(imagePath, errorFolder);
            isError = true;
            ++errorCount;

            float x0 = det.at<float>(i, 3), y0 = det.at<float>(i, 4);
            float x1 = det.at<float>(i, 5), y1 = det.at<float>(i, 6);
            int sX = static_cast<int>(x0 * image.cols);
            int sY = static_cast<int>(y0 * image.rows);
            int eX = static_cast<int>(x1 * image.cols);
            int eY = static_cast<int>(y1 * image.rows);

            drawRectangle(eX, sX, eY, sY,
                          topMarginValue, bottomMarginValue,
                          image, outputRes, outputFolder, outputImagePath,
                          stem, imagePath, debugOutput,
                          showPreview, previewOutputRes, previewDebugMaxRes,
                          isError, i, conf, errorMsg);
            break;
        }

        if (conf >= CONFIDENCE_LEVEL) {
            float x0 = det.at<float>(i, 3), y0 = det.at<float>(i, 4);
            float x1 = det.at<float>(i, 5), y1 = det.at<float>(i, 6);
            int sX = static_cast<int>(x0 * image.cols);
            int sY = static_cast<int>(y0 * image.rows);
            int eX = static_cast<int>(x1 * image.cols);
            int eY = static_cast<int>(y1 * image.rows);
            int w = eX - sX, h = eY - sY;

            int minRes = 0;
            if      (cropType == 3) minRes = MIN_FULLBODY_RES;
            else if (cropType == 2) minRes = MIN_FACE_RES;
            else if (cropType == 1) minRes = MIN_UPPERBODY_RES;

            if (w < minRes || h < minRes) {
                std::string typeLabel = (cropType == 3) ? "fullbody" :
                (cropType == 2) ? "face" : "upperbody";
                std::cout << "\rFace resolution is too small for " << typeLabel
                << " crop, skipping face_" << i << " on " << stem << ext << "\n";
                errorMsg = "FACE RESOLUTION IS TOO SMALL";
                imagesError(imagePath, errorFolder);
                isError = true;
                ++errorCount;
                drawRectangle(eX, sX, eY, sY,
                              topMarginValue, bottomMarginValue,
                              image, outputRes, outputFolder, outputImagePath,
                              stem, imagePath, debugOutput,
                              showPreview, previewOutputRes, previewDebugMaxRes,
                              isError, i, conf, errorMsg);
                break;
            }
            break;
        }
    }

    // ---- Second pass: process each high-confidence detection ---------------
    for (int i = 0; i < N && !isError; ++i) {
        float conf = det.at<float>(i, 2);
        if (conf <= CONFIDENCE_LEVEL) break;

        float x0 = det.at<float>(i, 3), y0 = det.at<float>(i, 4);
        float x1 = det.at<float>(i, 5), y1 = det.at<float>(i, 6);
        int sX = static_cast<int>(x0 * image.cols);
        int sY = static_cast<int>(y0 * image.rows);
        int eX = static_cast<int>(x1 * image.cols);
        int eY = static_cast<int>(y1 * image.rows);

        isError = drawRectangle(eX, sX, eY, sY,
                                topMarginValue, bottomMarginValue,
                                image, outputRes, outputFolder, outputImagePath,
                                stem, imagePath, debugOutput,
                                showPreview, previewOutputRes, previewDebugMaxRes,
                                isError, i, conf, errorMsg);
    }

    return errorCount;
}

// ---------------------------------------------------------------------------
static bool drawRectangle(int endX, int startX, int endY, int startY,
                          double topMarginValue, double bottomMarginValue,
                          const cv::Mat& image,
                          int outputRes,
                          const std::string& outputFolder,
                          const std::string& /*outputImagePath*/,
                          const std::string& filename,
                          const std::string& imagePath,
                          const std::string& debugOutput,
                          bool showPreview,
                          int previewOutputRes,
                          int previewDebugMaxRes,
                          bool isError,
                          int i,
                          float confidence,
                          const std::string& errorMsg)
{
    int squareSize    = std::min(endX - startX, endY - startY);
    int sqCx          = (endX + startX) / 2;
    int sqCy          = (endY + startY) / 2;
    int sqULx         = sqCx - squareSize / 2;
    int sqULy         = sqCy - squareSize / 2;
    int sqLRx         = sqULx + squareSize;
    int sqLRy         = sqULy + squareSize;
    int widthSquare   = sqLRx - sqULx;

    int topMargin    = static_cast<int>(squareSize * topMarginValue);
    int bottomMargin = static_cast<int>(squareSize * bottomMarginValue);
    int leftMargin   = static_cast<int>(squareSize * bottomMarginValue);
    int rightMargin  = static_cast<int>(squareSize * bottomMarginValue);

    int ulX = std::max(sqULx - leftMargin,   0);
    int ulY = std::max(sqULy - topMargin,    0);
    int lrX = std::min(sqLRx + rightMargin,  image.cols);
    int lrY = std::min(sqLRy + bottomMargin, image.rows);

    // Clamp width
    if ((lrX - ulX) > image.cols) {
        int shift = (lrX - ulX) - image.cols;
        ulX = std::max(ulX - shift, 0);
        lrX = image.cols;
    }
    // Clamp height
    if ((lrY - ulY) > image.rows) {
        int shift = (lrY - ulY) - image.rows;
        ulY = std::max(ulY - shift, 0);
        lrY = image.rows;
    }

    int sqMarginSize  = std::min(lrX - ulX, lrY - ulY);
    int sqMCx         = (lrX + ulX) / 2;
    int sqMULx        = sqMCx - sqMarginSize / 2;
    int sqMULy        = ulY;
    int sqMLRx        = sqMULx + sqMarginSize;
    int sqMLRy        = sqMULy + sqMarginSize;

    // Centre the crop horizontally on the detected face
    int faceCx  = (startX + endX) / 2;
    int shift   = faceCx - ((sqMULx + sqMLRx) / 2);
    sqMULx     += shift;
    sqMLRx     += shift;

    sqMULx = std::max(sqMULx, 0);
    sqMLRx = std::min(sqMLRx, image.cols);
    sqMarginSize = sqMLRx - sqMULx;

    cv::Mat squareRegion = image(cv::Rect(sqMULx, sqMULy, sqMarginSize, sqMarginSize));

    std::string outPath = outputFolder + "/" + filename + "_face_" + std::to_string(i) + ".png";
    cv::Mat resizedImage;

    if (squareRegion.empty()) {
        isError = true;
        const_cast<std::string&>(errorMsg) = "NO FACE DETECTED";
    } else if (!isError) {
        cv::resize(squareRegion, resizedImage, {outputRes, outputRes});
        cv::imwrite(outPath, resizedImage);
    }

    // ---- Debug overlay ----
    int thickness = std::max(image.cols / 128, 5);
    cv::Mat debugImage = image.clone();

    cv::rectangle(debugImage, {startX, startY}, {endX, endY},      {0, 0, 255},   thickness);
    cv::rectangle(debugImage, {sqULx,  sqULy},  {sqLRx, sqLRy},    {0, 255, 0},   thickness);
    cv::rectangle(debugImage,
                  {sqMULx, sqMULy},
                  {sqMULx + sqMarginSize, sqMULy + sqMarginSize},
                  {255, 165, 0}, thickness);

    double fontScale = std::min(image.cols, image.rows) / 1000.0;
    int    fontThick = std::max(1, std::min(image.cols, image.rows) / 500);

    // First label
    std::string resText = std::to_string(image.cols) + "x" + std::to_string(image.rows)
    + " face_" + std::to_string(i)
    + "_" + std::to_string(widthSquare) + "px"
    + " (" + std::to_string(static_cast<int>(confidence * 100)) + "%)";

    int baseline = 0;
    cv::Size ts = cv::getTextSize(resText, cv::FONT_HERSHEY_SIMPLEX, fontScale, fontThick, &baseline);
    int bgW = ts.width + 10, bgH = ts.height + 10;
    bgW = std::min(bgW, debugImage.cols);

    cv::Mat bg1(bgH, bgW, CV_8UC3, cv::Scalar(255, 255, 0));
    cv::putText(bg1, resText, {10, ts.height + 5},
                cv::FONT_HERSHEY_SIMPLEX, fontScale, {0, 0, 0}, fontThick);
    bg1.copyTo(debugImage(cv::Rect(0, 0, bgW, bgH)));

    // Second label
    std::string statusText = isError ? ("ERROR " + errorMsg) : "OK PASSED";
    cv::Scalar  statusColor = isError ? cv::Scalar(0, 0, 255) : cv::Scalar(0, 255, 0);

    cv::Size ts2 = cv::getTextSize(statusText, cv::FONT_HERSHEY_SIMPLEX, fontScale, fontThick, &baseline);
    int bg2W = std::min(ts2.width + 10, debugImage.cols);
    int bg2H = ts2.height + 10;

    int combinedH = bgH + bg2H;
    if (combinedH > debugImage.rows) {
        cv::resize(debugImage, debugImage, {debugImage.cols, combinedH});
    }

    cv::Mat bg2(bg2H, bg2W, CV_8UC3, statusColor);
    cv::putText(bg2, statusText, {10, ts2.height + 5},
                cv::FONT_HERSHEY_SIMPLEX, fontScale, {0, 0, 0}, fontThick);
    bg2.copyTo(debugImage(cv::Rect(0, bgH, bg2W, bg2H)));

    std::string debugPath = debugOutput + "/" + filename + "_face_" + std::to_string(i) + ".jpg";
    cv::imwrite(debugPath, debugImage);

    if (showPreview) {
        preview(debugImage, resizedImage, previewOutputRes, previewDebugMaxRes, isError);
    }

    return isError;
}

// ---------------------------------------------------------------------------
static void preview(const cv::Mat& debugImage,
                    const cv::Mat& resizedImage,
                    int previewOutputRes,
                    int previewDebugMaxRes,
                    bool isError)
{
    double ww = debugImage.cols, wh = debugImage.rows;
    double scale;
    if (ww > previewDebugMaxRes || wh > previewDebugMaxRes) {
        scale = std::min(previewDebugMaxRes / ww, previewDebugMaxRes / wh);
    } else {
        scale = std::max(previewOutputRes / ww, previewOutputRes / wh);
    }

    cv::Mat dbPrev;
    cv::resize(debugImage, dbPrev,
               {static_cast<int>(ww * scale), static_cast<int>(wh * scale)});

    cv::namedWindow("Debug Image", cv::WINDOW_AUTOSIZE);
    cv::imshow("Debug Image", dbPrev);
    cv::setWindowProperty("Debug Image", cv::WND_PROP_TOPMOST, 1);
    cv::setWindowProperty("Debug Image", cv::WND_PROP_ASPECT_RATIO, cv::WINDOW_KEEPRATIO);

    if (!isError && !resizedImage.empty()) {
        cv::Mat outPrev;
        cv::resize(resizedImage, outPrev, {previewOutputRes, previewOutputRes});
        cv::namedWindow("Output Image", cv::WINDOW_AUTOSIZE);
        cv::imshow("Output Image", outPrev);
        cv::setWindowProperty("Output Image", cv::WND_PROP_TOPMOST, 1);
        cv::setWindowProperty("Output Image", cv::WND_PROP_ASPECT_RATIO, cv::WINDOW_KEEPRATIO);
    }

    cv::waitKey(250);
}
