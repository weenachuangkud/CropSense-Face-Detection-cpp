# CropSense - Face Detection in C++
(C++ port of [CropSense-Face-Detection](https://github.com/senhan07/CropSense-Face-Detection))

CropSense is a tool to crop images based on various bounding box sizes, such as upper body, face, or full body. It utilizes a pre-trained SSD face detection model to accurately identify faces within images and performs precise cropping into a 1:1 ratio based on the detected regions. Useful for creating datasets for model training.

## Requirements
- CMake 3.10+
- OpenCV
- Ninja (optional)

## How to Build

### Windows
1. Open **Native Tools Command Prompt for VS 2022**
2. Navigate to your project folder:
```bat
   cd your_project_path
```
3. Clear existing build folder (if any):
```bat
   rmdir /s /q build
```
4. Configure and build with CMake:
```bat
   cmake -B build -DOpenCV_DIR="C:/your_path/opencv/build"
   cmake --build build
```
5. Or with Ninja (faster):
```bat
   cmake -B build -G Ninja -DOpenCV_DIR="C:/your_path/opencv/build"
   cmake --build build
```

### Linux
1. Install OpenCV:
```bash
   sudo apt-get install -y libopencv-dev ninja-build
```
2. Configure and build:
```bash
   cmake -B build -G Ninja
   cmake --build build
```

## Usage
1. Place your images in the `input` folder
2. Copy `config.json`, `deploy.prototxt.txt`, and `res10_300x300_ssd_iter_140000.caffemodel` next to the executable
3. Run `main.exe` (Windows) or `./main` (Linux)
4. Select crop type and preview option

## License
This project is licensed under the [MIT License](LICENSE).
