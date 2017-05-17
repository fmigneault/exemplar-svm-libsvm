/* --------------------
Image operations
-------------------- */
#ifndef IMG_UTILS_H
#define IMG_UTILS_H

#include "opencv2/opencv.hpp"

// Reads an image from file and displays it in a small window at the same time
cv::Mat imReadAndDisplay(std::string imagePath, std::string windowName = "", cv::ImreadModes readMode = cv::IMREAD_GRAYSCALE);

// Converts the specified image to desired image extension format (if `path` is an image file)
// Converts all found images in a directory with specified extension from one format to another (if `path` is a directory, `fromExtension` required)
// Optionally outputs the converted image(s) to the `toDirectory`
bool imConvert(std::string path, std::string toExtension, std::string fromExtension = "", std::string toDirectory = "");

// Translation of an image with XY pixel offset
cv::Mat imTranslate(const cv::Mat& image, cv::Point offset);

// Flips an image in horizontal/vertital/both directions
enum FlipMode { VERTICAL = 0, HORIZONTAL = 1, BOTH = -1, NONE = -2 };
cv::Mat imFlip(cv::Mat image, FlipMode flipMode);

// Resizes an image to the specified dimensions
cv::Mat imResize(cv::Mat image, cv::Size size, cv::InterpolationFlags interpolMethod = cv::INTER_AREA);

// Crops an image with specified inputs
cv::Mat imCrop(cv::Mat image, int x, int y, int w, int h);
cv::Mat imCrop(cv::Mat image, cv::Point p1, cv::Point p2);
cv::Mat imCrop(cv::Mat image, cv::Rect r);

// Crops an image so that the resulting image corresponds to the specified ratio and method
enum CropMode { TOP_LEFT, TOP_MIDDLE, TOP_RIGHT, CENTER_LEFT, CENTER_MIDDLE, CENTER_RIGHT, BOTTOM_LEFT, BOTTOM_MIDDLE, BOTTOM_RIGHT };
cv::Mat imCropByRatio(cv::Mat image, double ratio, CropMode cropMode = CENTER_MIDDLE);

// Returns a vector containing the original image followed by multiple synthetic images generated from the original
std::vector<cv::Mat> imSyntheticGeneration(cv::Mat image);

// Returns a vector containing the original image followed by multiple synthetic images generated from the original
std::vector<cv::Mat> imSyntheticGenerationScaleAndTranslation(const cv::Mat image, int nScales, int translationSize, double minScale);

// Returns a vector containing the patches generated by splitting the image with specified paramters
std::vector<cv::Mat> imSplitPatches(cv::Mat image, cv::Size patchCounts = cv::Size(0, 0));

// Returns a vector of images combining patches splitting and other preprocessing steps (resizing, grayscale, hist.equal., etc.) 
std::vector<cv::Mat> imPreprocess(std::string imagePath, cv::Size imSize, cv::Size patchCounts, bool useHistEqual = false,
                                  std::string windowName = "", cv::ImreadModes readMode = cv::IMREAD_GRAYSCALE, 
                                  cv::InterpolationFlags resizeMode = cv::INTER_AREA);

std::vector<cv::Mat> imPreprocess(cv::Mat roi, cv::Size imSize, cv::Size patchCounts, bool useHistEqual = false,
                                  std::string windowName = "", cv::ImreadModes readMode = cv::IMREAD_GRAYSCALE, 
                                  cv::InterpolationFlags resizeMode = cv::INTER_AREA);

#endif/*IMG_UTILS_H*/
