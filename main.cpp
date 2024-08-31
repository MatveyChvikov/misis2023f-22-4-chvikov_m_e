/**
 * @file main.cpp
 * @brief Основной файл программы для анализа функции отклика границы с использованием OpenGL и ImGui.
 *
 * Этот файл содержит функцию main, которая инициализирует контекст OpenGL, запускает цикл обработки событий и отображает интерфейс пользователя с помощью ImGui.
 */

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

/**
 * @brief Загрузка шейдеров и создание программы.
 *
 * @param vertex_file_path Путь к файлу с вершинным шейдером.
 * @param fragment_file_path Путь к файлу с фрагментным шейдером.
 * @return Идентификатор OpenGL программы.
 */
GLuint LoadShaders(const char* vertex_file_path, const char* fragment_file_path) {
    // Создаем шейдеры
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    // Чтение кода вершинного шейдера из файла
    std::string VertexShaderCode;
    std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
    if(VertexShaderStream.is_open()){
        std::stringstream sstr;
        sstr << VertexShaderStream.rdbuf();
        VertexShaderCode = sstr.str();
        VertexShaderStream.close();
    }else{
        std::cerr << "Не удалось открыть " << vertex_file_path << std::endl;
        return 0;
    }

    // Чтение кода фрагментного шейдера из файла
    std::string FragmentShaderCode;
    std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
    if(FragmentShaderStream.is_open()){
        std::stringstream sstr;
        sstr << FragmentShaderStream.rdbuf();
        FragmentShaderCode = sstr.str();
        FragmentShaderStream.close();
    }else{
        std::cerr << "Не удалось открыть " << fragment_file_path << std::endl;
        return 0;
    }

    GLint Result = GL_FALSE;
    int InfoLogLength;

    // Компиляция вершинного шейдера
    char const * VertexSourcePointer = VertexShaderCode.c_str();
    glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
    glCompileShader(VertexShaderID);

    // Проверка вершинного шейдера
    glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if ( InfoLogLength > 0 ){
        std::vector<char> VertexShaderErrorMessage(InfoLogLength+1);
        glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
        std::cerr << &VertexShaderErrorMessage[0] << std::endl;
    }

    // Компиляция фрагментного шейдера
    char const * FragmentSourcePointer = FragmentShaderCode.c_str();
    glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
    glCompileShader(FragmentShaderID);

    // Проверка фрагментного шейдера
    glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if ( InfoLogLength > 0 ){
        std::vector<char> FragmentShaderErrorMessage(InfoLogLength+1);
        glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
        std::cerr << &FragmentShaderErrorMessage[0] << std::endl;
    }

    // Линковка программы
    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, VertexShaderID);
    glAttachShader(ProgramID, FragmentShaderID);
    glLinkProgram(ProgramID);

    // Проверка программы
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if ( InfoLogLength > 0 ){
        std::vector<char> ProgramErrorMessage(InfoLogLength+1);
        glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
        std::cerr << &ProgramErrorMessage[0] << std::endl;
    }
    
    glDetachShader(ProgramID, VertexShaderID);
    glDetachShader(ProgramID, FragmentShaderID);
    
    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);

    return ProgramID;
}

/**
 * @brief Главная функция программы.
 *
 * Инициализирует GLFW, GLEW и ImGui, загружает шейдеры и запускает главный цикл обработки событий и рендеринга.
 *
 * @return Код завершения программы (0 - успешное завершение, -1 - ошибка).
 */
int main() {
    // Инициализация GLFW
    if (!glfwInit()) {
        std::cerr << "Не удалось инициализировать GLFW\n";
        return -1;
    }

    // Создание окна
    window = glfwCreateWindow(1280, 720, "Edge Response Function Analyzer", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Не удалось открыть окно GLFW\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Включение вертикальной синхронизации

    // Инициализация GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Не удалось инициализировать GLEW\n";
        return -1;
    }

    // Вывод информации об OpenGL
    std::cout << "Версия OpenGL: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "Версия GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "Производитель: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "Рендерер: " << glGetString(GL_RENDERER) << std::endl;

    // Настройка ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Загрузка шейдеров
    programID = LoadShaders("VertexShader.glsl", "FragmentShader.glsl");

    // Создание VAO и VBO для отображения изображения
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
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    int synthWidth = 500;
    int synthHeight = 500;
    int synthRadius = 200;

    // Основной цикл
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Начало нового кадра ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Создание интерфейса
        ImGui::Begin("Панель управления");
        
        if (ImGui::Button("Сгенерировать тестовое изображение")) {
            GenerateTestImage();
        }
        
        if (ImGui::Button("Загрузить изображение")) {
            LoadImage();
        }
        
        if (ImGui::Button("Рассчитать функцию отклика")) {
            CalculateResponseFunction();
        }
        
        if (ImGui::Button("Применить улучшение границ")) {
            ApplyEdgeEnhancement();
        }

        if (ImGui::Button("Рассчитать уровень шума")) {
            double noiseLevel = CalculateNoiseLevel(currentImage);
            outputMessage = "Уровень шума: " + std::to_string(noiseLevel);
        }

        if (ImGui::Button("Рассчитать CNR")) {
            cv::Rect roi(100, 100, 100, 100);
            double cnr = CalculateCNR(currentImage, roi);
            outputMessage = "CNR: " + std::to_string(cnr);
        }
        
        ImGui::Checkbox("Показать оригинальное изображение", &showOriginalImage);

        ImGui::Separator();
        ImGui::Text("Синтезировать тестовое изображение");
        ImGui::InputInt("Ширина", &synthWidth);
        ImGui::InputInt("Высота", &synthHeight);
        ImGui::InputInt("Радиус окружности", &synthRadius);
        if (ImGui::Button("Синтезировать")) {
            SynthesizeTestImage(synthWidth, synthHeight, synthRadius);
        }

        ImGui::Separator();
        ImGui::TextWrapped("%s", outputMessage.c_str());

        ImGui::End();

        // Рендеринг
        glClear(GL_COLOR_BUFFER_BIT);

        if (showOriginalImage && !currentImage.empty()) {
            RenderImage();
        } else {
            std::cout << "Изображение не отображается: " 
                      << (showOriginalImage ? "отображение включено" : "отображение выключено")
                      << ", " 
                      << (currentImage.empty() ? "изображение отсутствует" : "изображение загружено") 
                      << std::endl;
        }

        RenderResponseFunction();

        // Рендеринг ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Очистка
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
