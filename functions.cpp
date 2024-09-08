#include "functions.h"
#include <imgui.h>
#include "tinyfiledialogs.h"
#include <iostream>

void GenerateTestImage() {
    std::cout << "Generating test image" << std::endl;
    currentImage = cv::Mat(500, 500, CV_8UC1, cv::Scalar(0));
    cv::circle(currentImage, cv::Point(250, 250), 200, cv::Scalar(255), -1);
    cv::GaussianBlur(currentImage, currentImage, cv::Size(5, 5), 2.0);
    
    UpdateImageTexture();
    outputMessage = "Test image generated";
    std::cout << "Image size: " << currentImage.size() << std::endl;
}

void LoadImage() {
    std::cout << "Loading image" << std::endl;
    const char* filepath = tinyfd_openFileDialog(
        "Choose an image",
        "",
        0,
        NULL,
        NULL,
        0);
    if (filepath) {
        currentImage = cv::imread(filepath, cv::IMREAD_GRAYSCALE);
        if (currentImage.empty()) {
            outputMessage = "Failed to load image";
        } else {
            UpdateImageTexture();
            outputMessage = "Image loaded";
        }
    }
    std::cout << "Image loaded: " << (currentImage.empty() ? "no" : "yes") << std::endl;
    if (!currentImage.empty()) {
        std::cout << "Image size: " << currentImage.size() << std::endl;
    }
}

void CalculateResponseFunction() {
    std::cout << "Calculating response function" << std::endl;
    if (currentImage.empty()) {
        outputMessage = "No image loaded";
        return;
    }

    cv::Mat edges;
    cv::Canny(currentImage, edges, 100, 200);

    std::vector<cv::Vec3f> circles;
    cv::HoughCircles(edges, circles, cv::HOUGH_GRADIENT, 1, edges.rows/8, 200, 100, 0, 0);

    if (circles.empty()) {
        outputMessage = "No circles detected";
        std::cout << "No circles detected in the image" << std::endl;
        return;
    }

    cv::Vec3f circle = circles[0];
    cv::Point center(cvRound(circle[0]), cvRound(circle[1]));
    int radius = cvRound(circle[2]);

    std::cout << "Circle detected: center (" << center.x << ", " << center.y << "), radius " << radius << std::endl;

    std::vector<double> radialProfile;
    for (int r = 0; r <= radius; ++r) {
        double sum = 0;
        int count = 0;
        for (int theta = 0; theta < 360; ++theta) {
            int x = center.x + r * cos(theta * CV_PI / 180);
            int y = center.y + r * sin(theta * CV_PI / 180);
            if (x >= 0 && x < currentImage.cols && y >= 0 && y < currentImage.rows) {
                sum += currentImage.at<uchar>(y, x);
                count++;
            }
        }
        if (count > 0) {
            radialProfile.push_back(sum / count);
        }
    }

    responseFunction.clear();
    for (size_t i = 1; i < radialProfile.size(); ++i) {
        responseFunction.push_back(static_cast<float>(radialProfile[i] - radialProfile[i-1]));
    }

    outputMessage = "Response function calculated";
    std::cout << "Response function size: " << responseFunction.size() << std::endl;
}

void ApplyEdgeEnhancement() {
    std::cout << "Applying edge enhancement" << std::endl;
    if (currentImage.empty()) {
        outputMessage = "No image loaded";
        return;
    }

    cv::Mat enhanced;
    cv::Laplacian(currentImage, enhanced, CV_8U, 3);
    cv::add(currentImage, enhanced, currentImage);

    UpdateImageTexture();
    outputMessage = "Edge enhancement applied";
}

void UpdateImageTexture() {
    std::cout << "Updating image texture" << std::endl;
    if (currentImage.empty()) {
        std::cout << "Image is empty, texture not updated" << std::endl;
        return;
    }

    glDeleteTextures(1, &imageTexture);
    glGenTextures(1, &imageTexture);
    glBindTexture(GL_TEXTURE_2D, imageTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, currentImage.cols, currentImage.rows, 0, GL_RED, GL_UNSIGNED_BYTE, currentImage.data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    std::cout << "Texture updated, size: " << currentImage.cols << "x" << currentImage.rows << std::endl;

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL error when updating texture: " << err << std::endl;
    }
}

void RenderImage() {
    std::cout << "Rendering image" << std::endl;
    if (currentImage.empty()) {
        std::cout << "Image is empty, rendering skipped" << std::endl;
        return;
    }

    glUseProgram(programID);
    glBindVertexArray(vao);
    glBindTexture(GL_TEXTURE_2D, imageTexture);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL error when rendering image: " << err << std::endl;
    }
}

void RenderResponseFunction() {
    std::cout << "Rendering response function graph" << std::endl;
    if (responseFunction.empty()) {
        std::cout << "Response function is empty, rendering skipped" << std::endl;
        return;
    }

    ImGui::Begin("Response Function Graph");
    ImGui::PlotLines("Response Function", responseFunction.data(), static_cast<int>(responseFunction.size()), 0, nullptr, FLT_MAX, FLT_MAX, ImVec2(0, 200));
    ImGui::End();
}

double CalculateNoiseLevel(const cv::Mat& image) {
    cv::Scalar mean, stddev;
    cv::meanStdDev(image, mean, stddev);
    return stddev[0];
}

double CalculateCNR(const cv::Mat& image, const cv::Rect& roi) {
    cv::Mat roiImage = image(roi);
    cv::Scalar mean, stddev;
    cv::meanStdDev(roiImage, mean, stddev);
    return mean[0] / stddev[0];
}

void SynthesizeTestImage(int width, int height, int radius) {
    std::cout << "Synthesizing test image" << std::endl;
    currentImage = cv::Mat(height, width, CV_8UC1, cv::Scalar(0));
    cv::circle(currentImage, cv::Point(width/2, height/2), radius, cv::Scalar(255), -1);
    cv::GaussianBlur(currentImage, currentImage, cv::Size(5, 5), 2.0);
    UpdateImageTexture();
    outputMessage = "Test image synthesized";
    std::cout << "Synthesized image size: " << currentImage.size() << std::endl;
}