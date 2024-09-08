cmake_minimum_required(VERSION 3.10)
project(EdgeResponseAnalyzer)

# Установка стандарта C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Пути к библиотекам
set(OpenCV_DIR "C:/opencv/build")
set(GLEW_DIR "C:/glew")
set(GLFW_DIR "C:/glfw")
set(IMGUI_DIR "C:/imgui")
set(TINYFILEDIALOGS_DIR "C:/tinyfiledialogs")

# Поиск необходимых пакетов
find_package(OpenGL REQUIRED)
find_package(OpenCV REQUIRED)
if(NOT OpenCV_FOUND)
    message(FATAL_ERROR "OpenCV not found!")
endif()

# Указание путей к заголовочным файлам и библиотекам GLEW и GLFW
include_directories(${GLEW_DIR}/include)
include_directories(${GLFW_DIR}/include)

set(GLEW_LIBRARY "${GLEW_DIR}/lib/Release/x64/glew32.lib")
set(GLFW_LIBRARY "${GLFW_DIR}/lib-vc2022/glfw3.lib")

# Добавление исполняемого файла
add_executable(EdgeResponseAnalyzer
    main.cpp
    functions.cpp
    functions.h
    ${TINYFILEDIALOGS_DIR}/tinyfiledialogs.c
)

# Подключение директорий
include_directories(
    ${OPENGL_INCLUDE_DIRS}
    ${GLEW_DIR}/include
    ${GLFW_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${TINYFILEDIALOGS_DIR}
)

# Линковка библиотек
target_link_libraries(EdgeResponseAnalyzer
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARY}
    ${GLFW_LIBRARY}
    ${OpenCV_LIBS}
)

# Добавление исходных файлов для ImGui
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# Линковка ImGui с нашим приложением
target_link_libraries(EdgeResponseAnalyzer imgui)

# Копирование шейдеров в директорию сборки
configure_file(${CMAKE_SOURCE_DIR}/VertexShader.glsl ${CMAKE_BINARY_DIR}/VertexShader.glsl COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/FragmentShader.glsl ${CMAKE_BINARY_DIR}/FragmentShader.glsl COPYONLY)

# Построечный шаг для копирования необходимых DLL-файлов
add_custom_command(TARGET EdgeResponseAnalyzer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${GLEW_DIR}/bin/Release/x64/glew32.dll"
        "${GLFW_DIR}/lib-vc2022/glfw3.dll"
        $<TARGET_FILE_DIR:EdgeResponseAnalyzer>
)

# Указание, что мы собираем 64-битное приложение
set(CMAKE_GENERATOR_PLATFORM x64)
