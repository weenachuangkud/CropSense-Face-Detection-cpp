#include "config.h"
#include "user_input.h"
#include "image_processing.h"

#include <opencv2/opencv.hpp>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <future>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Collect all regular files from a directory
// ---------------------------------------------------------------------------
static std::vector<std::string> listFiles(const std::string& dir)
{
    std::vector<std::string> files;
    if (!fs::exists(dir)) return files;
    for (auto& entry : fs::directory_iterator(dir)) {
        if (fs::is_regular_file(entry.path())) {
            files.push_back(entry.path().string());
        }
    }
    return files;
}

// ---------------------------------------------------------------------------
// Print summary
// ---------------------------------------------------------------------------
static void printSummary(int total, int errors, const std::string& outputFolder)
{
    int outputCount = 0;
    if (fs::exists(outputFolder)) {
        for (auto& e : fs::directory_iterator(outputFolder))
            if (fs::is_regular_file(e)) ++outputCount;
    }
    std::cout << "\nTotal images:      " << total         << "\n";
    std::cout << "Processed images:  " << total - errors  << "\n";
    std::cout << "Total faces found: " << outputCount     << "\n";
    std::cout << "Error images:      " << errors          << "\n";
}

// ---------------------------------------------------------------------------
// Single-thread processing
// ---------------------------------------------------------------------------
static void runSingleThread(const CropOptions& crop,
                            bool showPreview)
{
    auto files = listFiles(INPUT_FOLDER);
    int errorCount = 0;
    int done = 0;

    for (auto& path : files) {
        int err = processImage(path,
                               ERROR_FOLDER,
                               crop.outputFolder,
                               crop.debugOutput,
                               OUTPUT_RES,
                               PREVIEW_OUTPUT_RES,
                               PREVIEW_DEBUG_MAX_RES,
                               showPreview,
                               crop.cropType,
                               crop.topMargin,
                               crop.bottomMargin);
        errorCount += err;
        ++done;
        std::cout << "\r[SINGLETHREAD] Processed " << done << "/" << files.size() << std::flush;
    }
    std::cout << "\n";
    cv::destroyAllWindows();
    printSummary(static_cast<int>(files.size()), errorCount, crop.outputFolder);
}

// ---------------------------------------------------------------------------
// Multi-thread processing  (uses std::async / thread-pool via hardware_concurrency)
// ---------------------------------------------------------------------------
static void runMultiThread(const CropOptions& crop,
                           bool showPreview)
{
    auto files = listFiles(INPUT_FOLDER);
    std::atomic<int> totalErrors{0};
    std::atomic<int> done{0};
    int total = static_cast<int>(files.size());

    // Lambda for one image
    auto worker = [&](const std::string& path) {
        int err = processImage(path,
                               ERROR_FOLDER,
                               crop.outputFolder,
                               crop.debugOutput,
                               OUTPUT_RES,
                               PREVIEW_OUTPUT_RES,
                               PREVIEW_DEBUG_MAX_RES,
                               showPreview,
                               crop.cropType,
                               crop.topMargin,
                               crop.bottomMargin);
        totalErrors += err;
        int d = ++done;
        std::cout << "\r[MULTITHREAD] Processed " << d << "/" << total << std::flush;
    };

    unsigned int nThreads = std::max(1u, std::thread::hardware_concurrency());
    std::vector<std::future<void>> futures;
    futures.reserve(files.size());

    // Simple thread-pool using async with a semaphore-like approach
    std::mutex queueMtx;
    size_t nextIdx = 0;

    auto threadBody = [&]() {
        while (true) {
            std::string path;
            {
                std::lock_guard<std::mutex> lk(queueMtx);
                if (nextIdx >= files.size()) return;
                path = files[nextIdx++];
            }
            worker(path);
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(nThreads);
    for (unsigned i = 0; i < nThreads; ++i)
        threads.emplace_back(threadBody);
    for (auto& t : threads) t.join();

    std::cout << "\n";
    cv::destroyAllWindows();
    printSummary(total, totalErrors.load(), crop.outputFolder);
}

// ---------------------------------------------------------------------------
int main()
{
    CropOptions crop = selectOption();
    RunOptions  run  = previewWindow();

    // Ensure folders exist
    for (auto& dir : { INPUT_FOLDER,
        crop.outputFolder,
         crop.debugOutput,
         ERROR_FOLDER })
    {
        fs::create_directories(dir);
    }

    cleanOutput(crop.outputFolder, crop.debugOutput, ERROR_FOLDER);

    if (run.parallel) {
        runMultiThread(crop, run.showPreview);
    } else {
        runSingleThread(crop, run.showPreview);
    }

    return 0;
}
