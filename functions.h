#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Структура для хранения результатов анализа изображения
struct ImageAnalysisResult {
    std::vector<double> edgeProfile;
    std::vector<double> noiseProfile;
    double signalMean;
    double noiseStd;
    double cnr;
    int centerX;
    int centerY;
    int radius;
};

// Глобальные переменные
extern GLuint programID;
extern GLuint vao;
extern GLuint imageTexture;
extern cv::Mat currentImage;
extern std::vector<float> responseFunction;
extern std::string outputMessage;
extern ImageAnalysisResult currentAnalysis;

// Функции обработки изображения
void GenerateTestImage();
void LoadImage();
void CalculateResponseFunction();
void ApplyEdgeEnhancement();
void UpdateImageTexture();
void RenderImage();
void RenderResponseFunction();
double CalculateNoiseLevel(const cv::Mat& image);
double CalculateCNR(const cv::Mat& image, const cv::Rect& roi);
void SynthesizeTestImage(int width, int height, int radius);
ImageAnalysisResult AnalyzeImage(const cv::Mat& image, const cv::Point& center, int radius);
json GetAnalysisData();

// Функции визуализации
void RenderAnalysisWindows();
void RenderEdgeProfile();
void RenderNoiseProfile();
void RenderStatistics();