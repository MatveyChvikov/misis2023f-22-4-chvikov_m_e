#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include "functions.h"
#include "tinyfiledialogs.h"

GLFWwindow* window;
cv::Mat currentImage;
std::vector<float> responseFunction;
GLuint imageTexture;
GLuint programID;
GLuint vao, vbo;
bool showOriginalImage = true;
std::string outputMessage;

// Загрузка и компиляция шейдеров
GLuint LoadShaders(const char* vertex_file_path, const char* fragment_file_path) {
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    // Читаем вершинный шейдер из файла
    std::string VertexShaderCode;
    std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
    if (VertexShaderStream.is_open()) {
        std::stringstream sstr;
        sstr << VertexShaderStream.rdbuf();
        VertexShaderCode = sstr.str();
        VertexShaderStream.close();
    }
    else {
        std::cerr << "Could not open " << vertex_file_path << std::endl;
        return 0;
    }

    // Читаем фрагментный шейдер из файла
    std::string FragmentShaderCode;
    std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
    if (FragmentShaderStream.is_open()) {
        std::stringstream sstr;
        sstr << FragmentShaderStream.rdbuf();
        FragmentShaderCode = sstr.str();
        FragmentShaderStream.close();
    }
    else {
        std::cerr << "Could not open " << fragment_file_path << std::endl;
        return 0;
    }

    GLint Result = GL_FALSE;
    int InfoLogLength;

    // Компилируем вершинный шейдер
    const char* VertexSourcePointer = VertexShaderCode.c_str();
    glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
    glCompileShader(VertexShaderID);

    // Проверяем вершинный шейдер
    glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
        glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
        std::cerr << &VertexShaderErrorMessage[0] << std::endl;
    }

    // Компилируем фрагментный шейдер
    const char* FragmentSourcePointer = FragmentShaderCode.c_str();
    glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
    glCompileShader(FragmentShaderID);

    // Проверяем фрагментный шейдер
    glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
        glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
        std::cerr << &FragmentShaderErrorMessage[0] << std::endl;
    }

    // Создаем программу и прикрепляем шейдеры
    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, VertexShaderID);
    glAttachShader(ProgramID, FragmentShaderID);
    glLinkProgram(ProgramID);

    // Проверяем программу
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
        glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
        std::cerr << &ProgramErrorMessage[0] << std::endl;
    }

    glDetachShader(ProgramID, VertexShaderID);
    glDetachShader(ProgramID, FragmentShaderID);
    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);

    return ProgramID;
}

int main() {
    // Инициализация GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    // Настройка GLFW
    glfwWindowHint(GLFW_SAMPLES, 4); // 4x антиалиасинг
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Создание окна и контекста OpenGL
    window = glfwCreateWindow(1280, 720, "Edge Response Function Analyzer", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to open GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Включаем вертикальную синхронизацию

    // Инициализация GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW\n";
        return -1;
    }

    // Настройка ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    // Настройка стиля ImGui
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Загружаем шейдеры
    programID = LoadShaders("VertexShader.glsl", "FragmentShader.glsl");

    // Настраиваем шрифты
    io.Fonts->Clear();
    ImFont* font = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/arial.ttf", 18.0f);
    ImFont* fontLarge = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/arial.ttf", 24.0f);
    io.Fonts->Build();

    // Создаем вершинный буфер для отображения изображения
    float vertices[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f,  1.0f, 1.0f, 1.0f
    };

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Настраиваем атрибуты вершин
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Параметры синтеза тестового изображения
    int synthWidth = 500;
    int synthHeight = 500;
    int synthRadius = 200;

    // Главный цикл приложения
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Начало нового кадра ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Основное окно управления
        ImGui::Begin("Control Panel", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        
        ImGui::PushFont(fontLarge);
        if (ImGui::Button("Generate Test Image", ImVec2(300, 50))) {
            GenerateTestImage();
        }

        if (ImGui::Button("Load Image", ImVec2(300, 50))) {
            LoadImage();
        }

        if (ImGui::Button("Calculate Response Function", ImVec2(300, 50))) {
            CalculateResponseFunction();
        }

        if (ImGui::Button("Apply Edge Enhancement", ImVec2(300, 50))) {
            ApplyEdgeEnhancement();
        }
        ImGui::PopFont();

        ImGui::Separator();

        // Параметры синтеза изображения
        ImGui::Text("Synthesize Test Image Parameters:");
        ImGui::InputInt("Width", &synthWidth);
        ImGui::InputInt("Height", &synthHeight);
        ImGui::InputInt("Circle Radius", &synthRadius);
        if (ImGui::Button("Synthesize", ImVec2(150, 30))) {
            SynthesizeTestImage(synthWidth, synthHeight, synthRadius);
        }

        ImGui::Separator();
        ImGui::TextWrapped("%s", outputMessage.c_str());
        ImGui::End();

        // Окно анализа
        RenderAnalysisWindows();

        // Рендеринг
        glClear(GL_COLOR_BUFFER_BIT);
        
        if (showOriginalImage && !currentImage.empty()) {
            RenderImage();
        }

        // Рендеринг ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Очистка ресурсов
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(programID);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}