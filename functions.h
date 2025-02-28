#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

#include <imgui.h>

using json = nlohmann::json;

// Структура для результатов анализа
struct ImageAnalysisResult {
    std::vector<double> edgeProfile;
    std::vector<double> noiseProfile;
    double signalMean = 0.0;
    double noiseStd = 0.0;
    double cnr = 0.0;
    int centerX = 0;
    int centerY = 0;
    int radius = 0;
};

// Глобальные переменные
extern GLuint programID;
extern GLuint vao;

extern cv::Mat images[2];
extern cv::Mat* imagePtrs[2];

extern cv::Mat*& currentImage;
extern cv::Mat*& processedImage;

extern GLuint textureIDs[2];
extern GLuint* textureIDPtrs[2];

extern GLuint*& currentImageID;
extern GLuint*& processedImageID;

extern std::vector<float> responseFunction;
extern std::string outputMessage;
extern ImageAnalysisResult currentAnalysis;

extern ImVec2 resolution;

// Функции
void GenerateCustomCircle(int width, int height, int radius);
void LoadImage();
void CalculateResponseFunction();
void UpdateImageTexture(const cv::Mat& from, GLuint& textureID);
void RenderImage(const cv::Mat& from, const GLuint& textureID, const char* title);
double CalculateNoiseLevel(const cv::Mat& image);
double CalculateCNR(const cv::Mat& image, const cv::Rect& roi);
ImageAnalysisResult AnalyzeImage(const cv::Mat& image, const cv::Point& center, int radius);
void RenderAnalysisWindows();
void RenderEdgeProfile();
void RenderNoiseProfile();
void RenderStatistics();
json GetAnalysisData();

void SwapImages();

cv::Mat SharpenFilter(const cv::Mat& from);
cv::Mat GaussBlurFilter(const cv::Mat& from);
cv::Mat LaplaceOperator(const cv::Mat& from);
cv::Mat NoiseFilter(const cv::Mat& from);

void SaveImageToDisk(const cv::Mat& from, const char* path);
