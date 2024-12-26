#include "functions.h"
#include <imgui.h>
#include "tinyfiledialogs.h"
#include <iostream>

ImageAnalysisResult currentAnalysis;

void GenerateTestImage() {
    std::cout << "Generating test image" << std::endl;
    currentImage = cv::Mat(500, 500, CV_8UC1, cv::Scalar(0));
    cv::circle(currentImage, cv::Point(250, 250), 200, cv::Scalar(255), -1);
    cv::GaussianBlur(currentImage, currentImage, cv::Size(5, 5), 2.0);
    
    UpdateImageTexture();
    outputMessage = "Test image generated";
}

void LoadImage() {
    const char* filepath = tinyfd_openFileDialog(
        "Choose an image",
        "",
        0,
        nullptr,
        nullptr,
        0
    );
    
    if (filepath) {
        currentImage = cv::imread(filepath, cv::IMREAD_GRAYSCALE);
        if (!currentImage.empty()) {
            UpdateImageTexture();
            outputMessage = "Image loaded successfully";
            
            // Автоматически запускаем анализ после загрузки
            CalculateResponseFunction();
        } else {
            outputMessage = "Failed to load image";
        }
    }
}

ImageAnalysisResult AnalyzeImage(const cv::Mat& image, const cv::Point& center, int radius) {
    ImageAnalysisResult result;
    result.centerX = center.x;
    result.centerY = center.y;
    result.radius = radius;
    
    // Расчет профиля края
    for (int r = -radius; r <= radius; ++r) {
        double sum = 0;
        int count = 0;
        
        for (int theta = 0; theta < 360; theta += 1) {
            double rad = theta * CV_PI / 180.0;
            int x = center.x + r * cos(rad);
            int y = center.y + r * sin(rad);
            
            if (x >= 0 && x < image.cols && y >= 0 && y < image.rows) {
                sum += image.at<uchar>(y, x);
                count++;
            }
        }
        
        if (count > 0) {
            result.edgeProfile.push_back(sum / count);
        }
    }
    
    // Расчет профиля шума
    cv::Mat roi = image(cv::Rect(
        std::max(0, center.x - radius/2),
        std::max(0, center.y - radius/2),
        std::min(radius, image.cols - center.x + radius/2),
        std::min(radius, image.rows - center.y + radius/2)
    ));
    
    cv::Mat meanFiltered;
    cv::blur(roi, meanFiltered, cv::Size(3, 3));
    
    for (int y = 0; y < roi.rows; ++y) {
        double rowNoise = 0;
        for (int x = 0; x < roi.cols; ++x) {
            double diff = roi.at<uchar>(y, x) - meanFiltered.at<uchar>(y, x);
            rowNoise += diff * diff;
        }
        result.noiseProfile.push_back(sqrt(rowNoise / roi.cols));
    }
    
    // Расчет статистических параметров
    cv::Mat innerROI = image(cv::Rect(
        std::max(0, center.x - radius/4),
        std::max(0, center.y - radius/4),
        std::min(radius/2, image.cols - center.x + radius/4),
        std::min(radius/2, image.rows - center.y + radius/4)
    ));
    
    cv::Scalar mean, stddev;
    cv::meanStdDev(innerROI, mean, stddev);
    
    result.signalMean = mean[0];
    result.noiseStd = stddev[0];
    result.cnr = mean[0] / stddev[0];
    
    return result;
}

void CalculateResponseFunction() {
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
        return;
    }
    
    cv::Vec3f circle = circles[0];
    cv::Point center(cvRound(circle[0]), cvRound(circle[1]));
    int radius = cvRound(circle[2]);
    
    currentAnalysis = AnalyzeImage(currentImage, center, radius);
    
    // Обновляем график функции отклика
    responseFunction.clear();
    for (size_t i = 1; i < currentAnalysis.edgeProfile.size(); ++i) {
        responseFunction.push_back(static_cast<float>(
            currentAnalysis.edgeProfile[i] - currentAnalysis.edgeProfile[i-1]
        ));
    }
    
    outputMessage = "Analysis complete";
}

void RenderAnalysisWindows() {
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    ImGui::Begin("Analysis Results", nullptr, ImGuiWindowFlags_NoCollapse);
    
    if (ImGui::BeginTabBar("AnalysisTabs")) {
        if (ImGui::BeginTabItem("Edge Response")) {
            RenderEdgeProfile();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Noise Analysis")) {
            RenderNoiseProfile();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Statistics")) {
            RenderStatistics();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    
    ImGui::End();
}

void RenderEdgeProfile() {
    if (!responseFunction.empty()) {
        // Увеличенный размер графика
        ImVec2 plotSize = ImVec2(700, 400);
        
        // Настройка стиля графика
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.0f, 0.5f, 1.0f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
        
        ImGui::PlotLines("##EdgeResponse",
            responseFunction.data(),
            responseFunction.size(),
            0, "Edge Response Function",
            FLT_MAX, FLT_MAX,
            plotSize
        );
        
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        
        // Добавляем подписи осей
        ImGui::Text("Distance from Edge (pixels)");
        ImGui::SameLine(plotSize.x - 100);
        ImGui::Text("Response");
    }
}

void RenderNoiseProfile() {
    if (!currentAnalysis.noiseProfile.empty()) {
        std::vector<float> noiseData(currentAnalysis.noiseProfile.begin(), 
                                   currentAnalysis.noiseProfile.end());
        
        ImVec2 plotSize = ImVec2(700, 400);
        
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
        
        ImGui::PlotLines("##NoiseProfile",
            noiseData.data(),
            noiseData.size(),
            0, "Noise Profile",
            FLT_MAX, FLT_MAX,
            plotSize
        );
        
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        
        ImGui::Text("Position (pixels)");
        ImGui::SameLine(plotSize.x - 100);
        ImGui::Text("Noise Level");
    }
}

void RenderStatistics() {
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 20));
    
    // Используем увеличенный шрифт для значений
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
    
    ImGui::BeginChild("Statistics", ImVec2(0, 0), true);
    
    ImGui::Text("Signal Statistics");
    ImGui::Separator();
    
    ImGui::Text("Mean Signal: %.2f HU", currentAnalysis.signalMean);
    ImGui::Text("Noise StdDev: %.2f HU", currentAnalysis.noiseStd);
    ImGui::Text("CNR: %.2f", currentAnalysis.cnr);
    
    ImGui::Spacing();
    ImGui::Text("Region Information");
    ImGui::Separator();
    
    ImGui::Text("Center: (%d, %d)", currentAnalysis.centerX, currentAnalysis.centerY);
    ImGui::Text("Radius: %d pixels", currentAnalysis.radius);
    
    ImGui::EndChild();
    
    ImGui::PopFont();
    ImGui::PopStyleVar(2);
}

json GetAnalysisData() {
    json data;
    
    if (!currentAnalysis.edgeProfile.empty()) {
        data["edgeProfile"] = currentAnalysis.edgeProfile;
        data["noiseProfile"] = currentAnalysis.noiseProfile;
        data["signalMean"] = currentAnalysis.signalMean;
        data["noiseStd"] = currentAnalysis.noiseStd;
        data["cnr"] = currentAnalysis.cnr;
        data["centerX"] = currentAnalysis.centerX;
        data["centerY"] = currentAnalysis.centerY;
        data["radius"] = currentAnalysis.radius;
    }
    
    return data;
}

// Остальные существующие функции остаются без изменений
void UpdateImageTexture() {
    if (currentImage.empty()) return;
    
    glDeleteTextures(1, &imageTexture);
    glGenTextures(1, &imageTexture);
    glBindTexture(GL_TEXTURE_2D, imageTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, currentImage.cols, currentImage.rows,
                 0, GL_RED, GL_UNSIGNED_BYTE, currentImage.data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void RenderImage() {
    if (currentImage.empty()) return;
    
    glUseProgram(programID);
    glBindVertexArray(vao);
    glBindTexture(GL_TEXTURE_2D, imageTexture);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void ApplyEdgeEnhancement() {
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
    currentImage = cv::Mat(height, width, CV_8UC1, cv::Scalar(0));
    cv::circle(currentImage, cv::Point(width/2, height/2), radius, cv::Scalar(255), -1);
    cv::GaussianBlur(currentImage, currentImage, cv::Size(5, 5), 2.0);
    UpdateImageTexture();
    outputMessage = "Test image synthesized";
}