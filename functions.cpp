#include "functions.h"
#include <imgui.h>
#include "tinyfiledialogs.h"
#include <iostream>
#include <cmath>

ImageAnalysisResult currentAnalysis;

void GenerateCustomCircle(int width, int height, int radius) {
    *currentImage = cv::Mat(height, width, CV_8UC1, cv::Scalar(0));
    
    // Рисуем круг в центре изображения
    cv::Point center(width/2, height/2);
    cv::circle(*currentImage, center, radius, cv::Scalar(255), -1);
    
    // Применяем размытие
    cv::GaussianBlur(*currentImage, *currentImage, cv::Size(5, 5), 2.0);
    
    UpdateImageTexture(*currentImage, *currentImageID);
    outputMessage = "Custom circle generated";
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
        *currentImage = cv::imread(filepath, cv::IMREAD_GRAYSCALE);
        if (!currentImage->empty()) {
            UpdateImageTexture(*currentImage, *currentImageID);
            outputMessage = "Image loaded successfully";
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
    
    // Анализ профиля края
    for (int r = -radius; r <= radius; ++r) {
        double sum = 0;
        int count = 0;
        
        for (int theta = 0; theta < 360; theta += 1) {
            double rad = theta * CV_PI / 180.0;
            int x = static_cast<int>(center.x + r * std::cos(rad));
            int y = static_cast<int>(center.y + r * std::sin(rad));
            
            if (x >= 0 && x < image.cols && y >= 0 && y < image.rows) {
                sum += static_cast<double>(image.at<uchar>(y, x));
                count++;
            }
        }
        
        if (count > 0) {
            result.edgeProfile.push_back(sum / static_cast<double>(count));
        }
    }
    
    // Анализ шума
    cv::Rect roiRect(
        std::max(0, center.x - radius/2),
        std::max(0, center.y - radius/2),
        std::min(radius, image.cols - center.x + radius/2),
        std::min(radius, image.rows - center.y + radius/2)
    );
    cv::Mat roi = image(roiRect);
    
    cv::Mat meanFiltered;
    cv::blur(roi, meanFiltered, cv::Size(3, 3));
    
    for (int y = 0; y < roi.rows; ++y) {
        double rowNoise = 0;
        for (int x = 0; x < roi.cols; ++x) {
            double diff = static_cast<double>(roi.at<uchar>(y, x)) - static_cast<double>(meanFiltered.at<uchar>(y, x));
            rowNoise += diff * diff;
        }
        result.noiseProfile.push_back(std::sqrt(rowNoise / static_cast<double>(roi.cols)));
    }
    
    // Расчет статистик
    cv::Scalar mean, stddev;
    cv::meanStdDev(roi, mean, stddev);
    
    result.signalMean = mean[0];
    result.noiseStd = stddev[0];
    result.cnr = (stddev[0] > 0) ? (mean[0] / stddev[0]) : 0;
    
    return result;
}

void CalculateResponseFunction() {
    if (currentImage->empty()) {
        outputMessage = "No image loaded";
        return;
    }
    
    cv::Mat edges;
    cv::Canny(*currentImage, edges, 100, 200);    

    int dp = 1;
    int minDist = 20;
    int param1 = 10;
    int param2 = 10;
    int minRadius = 0;
    int maxRadius = 0;

    std::vector<cv::Vec3f> circles;
    cv::HoughCircles(edges, circles, cv::HOUGH_GRADIENT, dp, minDist, param1, param2, minRadius, maxRadius);
    
    if (circles.empty()) {
        outputMessage = "No circles detected";
        return;
    }
    
    cv::Vec3f circle = circles[0];
    cv::Point center(cvRound(circle[0]), cvRound(circle[1]));
    int radius = cvRound(circle[2]);
    
    currentAnalysis = AnalyzeImage(*currentImage, center, radius);
    
    responseFunction.clear();
    for (size_t i = 1; i < currentAnalysis.edgeProfile.size(); ++i) {
        responseFunction.push_back(static_cast<float>(
            currentAnalysis.edgeProfile[i] - currentAnalysis.edgeProfile[i-1]
        ));
    }

    // создаём найденный круг на обработанном изображении для наглядности
    // поскольку изображение ч/б выделяем белым тонким кругом обведённым для контраста 2 чёрными
    *processedImage = currentImage->clone();
    cv::circle(*processedImage, center, radius - 1, cv::Scalar(0), 1);
    cv::circle(*processedImage, center, radius + 0, cv::Scalar(255), 1);
    cv::circle(*processedImage, center, radius + 1, cv::Scalar(0), 1);
    UpdateImageTexture(*processedImage, *processedImageID);
    
    outputMessage = "Analysis completed successfully";
}

void UpdateImageTexture(const cv::Mat& from, GLuint& textureID) {
    if (from.empty()) return;
    
    if (textureID != 0) {
        glDeleteTextures(1, &textureID);
    }
    
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // чтобы изображение не "ехало" по горизонтали
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, from.step/from.elemSize());

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, from.cols, from.rows,
                 0, GL_RED, GL_UNSIGNED_BYTE, from.ptr());

    glPixelStorei(GL_UNPACK_ALIGNMENT, 0);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

static struct CallbackData {
    ImVec2 pos;
    ImVec2 size;
    GLuint textureID;
} callbackData[2];

static CallbackData* callbackDataPtrs[2] = {&callbackData[0], &callbackData[1]};

static void draw_callback(const ImDrawList* parent_list, const ImDrawCmd* cmd)
{
    glBindVertexArray(vao);
    glUseProgram(programID);

    const auto& [pos, size, textureID] = *(CallbackData*)cmd->UserCallbackData;

    // отправка инфы про окно в вертексный шейдер
    glUniform1i(glGetUniformLocation(programID, "windowMode"), 1);
    glUniform2f(glGetUniformLocation(programID, "windowSize"), size.x, size.y);
    glUniform2f(glGetUniformLocation(programID, "screenPos"), pos.x, pos.y);
    glUniform2f(glGetUniformLocation(programID, "resolution"), resolution.x, resolution.y);

    glBindTexture(GL_TEXTURE_2D, textureID);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void RenderImage(const cv::Mat& from, const GLuint& textureID, const char* title) {
    if (from.empty()) return;

    // Проверяем что окно не свёрнуто чтобы не было артефактов от рендерхука
    if(ImGui::Begin(title)) {
        ImGui::Separator();

        auto& [pos, size, texID] = *callbackDataPtrs[0];

        pos  = ImGui::GetWindowPos();
        size = ImGui::GetWindowSize();
        texID = textureID;

        // ImGui::Image умеет рисовать только красным, без задания своего шейдера
        // Так что создаём колбек на отрисовку внутри окна через средства OpenGL
        ImGui::GetWindowDrawList()->AddCallback(draw_callback, callbackDataPtrs[0]);
        // затем добавляем сброс чтобы остальной рендер не сломался
        ImGui::GetWindowDrawList()->AddCallback(ImDrawCallback_ResetRenderState, nullptr);

        // Меняем слоты после передачи текстурки в колбек
        std::swap(callbackDataPtrs[0], callbackDataPtrs[1]);
    }
    ImGui::End();

}

static ImVec2 responseWindowSize = ImVec2(620, 450);
static ImVec2 responseGraphSize = ImVec2(600, 350);

void RenderAnalysisWindows() {
    ImGui::SetNextWindowSize(responseWindowSize, ImGuiCond_Once);
    ImGui::Begin("Analysis Results");
    
    if (ImGui::BeginTabBar("AnalysisTabs")) {
        if (ImGui::BeginTabItem("Edge Response")) {
            RenderEdgeProfile();
            ImGui::EndTabItem();
        }
        //if (ImGui::BeginTabItem("Noise Analysis")) {
        //    RenderNoiseProfile();
        //    ImGui::EndTabItem();
        //}
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
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.0f, 0.5f, 1.0f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
        
        ImGui::PlotLines("##EdgeResponse",
                         responseFunction.data(),
                         static_cast<int>(responseFunction.size()),
                         0,
                         "Edge Response Function",
                         FLT_MAX,
                         FLT_MAX,
                         responseGraphSize);
        
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        
        ImGui::Text("Distance from Edge (pixels)");
        ImGui::SameLine(responseGraphSize.x - 100);
        ImGui::Text("Response");
    }
}

void RenderNoiseProfile() {
    if (!currentAnalysis.noiseProfile.empty()) {
        std::vector<float> noiseData;
        noiseData.reserve(currentAnalysis.noiseProfile.size());
        for (const auto& val : currentAnalysis.noiseProfile) {
            noiseData.push_back(static_cast<float>(val));
        }
        
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
        
        ImGui::PlotLines("##NoiseProfile",
                         noiseData.data(),
                         static_cast<int>(noiseData.size()),
                         0,
                         "Noise Profile",
                         FLT_MAX,
                         FLT_MAX,
                         responseGraphSize);
        
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        
        ImGui::Text("Position (pixels)");
        ImGui::SameLine(responseGraphSize.x - 100);
        ImGui::Text("Noise Level");
    }
}

void RenderStatistics() {
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 20));
    
    ImGui::BeginChild("Statistics", ImVec2(0, 0), true);
    
    ImGui::Text("Signal Statistics:");
    ImGui::Separator();
    
    ImGui::Text("Mean Signal: %.2f", currentAnalysis.signalMean);
    ImGui::Text("Noise StdDev: %.2f", currentAnalysis.noiseStd);
    ImGui::Text("Contrast-to-Noise Ratio: %.2f", currentAnalysis.cnr);
    
    ImGui::Spacing();
    ImGui::Text("Region Information:");
    ImGui::Separator();
    
    ImGui::Text("Center Position: (%d, %d)", currentAnalysis.centerX, currentAnalysis.centerY);
    ImGui::Text("Circle Radius: %d pixels", currentAnalysis.radius);
    
    ImGui::EndChild();
    
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

void SwapImages()
{
    std::swap(imagePtrs[0], imagePtrs[1]);
    std::swap(textureIDPtrs[0], textureIDPtrs[1]);
}

cv::Mat SharpenFilter(const cv::Mat& from)
{
    // sharpen image using "unsharp mask" algorithm
    cv::Mat blurred;
    double sigma = 1, threshold = 5, amount = 1;
    cv::GaussianBlur(from, blurred, cv::Size(), sigma, sigma);

    cv::Mat lowContrastMask = cv::abs(from - blurred) < threshold;
    cv::Mat sharpened = from * (1+amount) + blurred * (-amount);
    from.copyTo(sharpened, lowContrastMask);

    return sharpened;
}
cv::Mat GaussBlurFilter(const cv::Mat& from)
{
    cv::Mat blurred;
    double sigma = 1;
    cv::GaussianBlur(from, blurred, cv::Size(), sigma, sigma);

    return blurred;
}

cv::Mat LaplaceOperator(const cv::Mat& from)
{
    int kernel_size = 3,
        scale = 1,
        delta = 0,
        ddepth = CV_16S;

    cv::Mat abs_dst, dst;
    cv::GaussianBlur(from, dst, cv::Size(3, 3), 0, 0, cv::BORDER_DEFAULT);
    cv::Laplacian(from, dst, ddepth, kernel_size, scale, delta, cv::BORDER_DEFAULT);

    // converting back to CV_8U
    cv::convertScaleAbs(dst, abs_dst);

    return abs_dst;
}

void SaveImageToDisk(const cv::Mat& from, const char* path)
{
    cv::imwrite(path, from);
}

cv::Mat NoiseFilter(const cv::Mat& from)
{
    cv::Mat noise(from.size(), from.type());
    cv::randn(noise, cv::Scalar::all(0), cv::Scalar::all(10));

    return from + noise;
}
