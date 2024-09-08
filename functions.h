#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

extern GLuint programID;
extern GLuint vao;
extern GLuint imageTexture;
extern cv::Mat currentImage;
extern std::vector<float> responseFunction;
extern std::string outputMessage;

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