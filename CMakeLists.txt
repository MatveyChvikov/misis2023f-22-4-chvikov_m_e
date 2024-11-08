cmake_minimum_required(VERSION 3.10)
project(EdgeResponseAnalyzer)

# Установка стандарта C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Пути к библиотекам (измените на свои пути)
set(OpenCV_DIR "C:/opencv/build")
set(GLEW_DIR "C:/glew")
set(GLFW_DIR "C:/glfw")
set(IMGUI_DIR "C:/imgui")
set(TINYFILEDIALOGS_DIR "C:/tinyfiledialogs")
set(JSON_DIR "C:/json") # nlohmann json

# Поиск необходимых пакетов
find_package(OpenGL REQUIRED)
find_package(OpenCV REQUIRED)

if(NOT OpenCV_FOUND)
    message(FATAL_ERROR "OpenCV not found!")
endif()

# Включение директорий заголовочных файлов
include_directories(
    ${OPENGL_INCLUDE_DIRS}
    ${GLEW_DIR}/include
    ${GLFW_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${TINYFILEDIALOGS_DIR}
    ${JSON_DIR}/include
    ${CMAKE_SOURCE_DIR}
)

# Сборка ImGui как отдельной библиотеки
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${GLFW_DIR}/include
)

# Добавление исполняемого файла
add_executable(EdgeResponseAnalyzer
    main.cpp
    functions.cpp
    functions.h
    ${TINYFILEDIALOGS_DIR}/tinyfiledialogs.c
)

# Установка путей к библиотекам
if(WIN32)
    set(GLEW_LIBRARY "${GLEW_DIR}/lib/Release/x64/glew32.lib")
    set(GLFW_LIBRARY "${GLFW_DIR}/lib-vc2022/glfw3.lib")
else()
    set(GLEW_LIBRARY "${GLEW_DIR}/lib/libGLEW.a")
    set(GLFW_LIBRARY "${GLFW_DIR}/lib/libglfw3.a")
endif()

# Линковка библиотек
target_link_libraries(EdgeResponseAnalyzer
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARY}
    ${GLFW_LIBRARY}
    ${OpenCV_LIBS}
    imgui
)

if(WIN32)
    target_link_libraries(EdgeResponseAnalyzer
        opengl32
        gdi32
        user32
        shell32
        comdlg32
    )
endif()

# Копирование шейдеров в директорию сборки
configure_file(${CMAKE_SOURCE_DIR}/VertexShader.glsl
    ${CMAKE_BINARY_DIR}/VertexShader.glsl COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/FragmentShader.glsl
    ${CMAKE_BINARY_DIR}/FragmentShader.glsl COPYONLY)

# Копирование необходимых DLL в директорию сборки (только для Windows)
if(WIN32)
    add_custom_command(TARGET EdgeResponseAnalyzer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${GLEW_DIR}/bin/Release/x64/glew32.dll"
        $<TARGET_FILE_DIR:EdgeResponseAnalyzer>
    )
    
    add_custom_command(TARGET EdgeResponseAnalyzer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${GLFW_DIR}/lib-vc2022/glfw3.dll"
        $<TARGET_FILE_DIR:EdgeResponseAnalyzer>
    )

    # Копирование DLL OpenCV
    foreach(OCVFile ${OpenCV_LIBS})
        add_custom_command(TARGET EdgeResponseAnalyzer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OpenCV_DIR}/bin/Release/${OCVFile}.dll"
            $<TARGET_FILE_DIR:EdgeResponseAnalyzer>
        )
    endforeach()
endif()

# Настройки компиляции
if(MSVC)
    target_compile_options(EdgeResponseAnalyzer PRIVATE /W4)
    target_compile_options(imgui PRIVATE /W4)
else()
    target_compile_options(EdgeResponseAnalyzer PRIVATE -Wall -Wextra)
    target_compile_options(imgui PRIVATE -Wall -Wextra)
endif()

# Установка выходной директории
set_target_properties(EdgeResponseAnalyzer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Включение отладочной информации
set(CMAKE_BUILD_TYPE Debug)
if(MSVC)
    target_compile_options(EdgeResponseAnalyzer PRIVATE /Zi)
    set_target_properties(EdgeResponseAnalyzer PROPERTIES 
        LINK_FLAGS "/DEBUG /INCREMENTAL:NO"
    )
else()
    target_compile_options(EdgeResponseAnalyzer PRIVATE -g)
endif()

# Установка архитектуры для Windows
if(WIN32)
    set(CMAKE_GENERATOR_PLATFORM x64)
endif()

# Опции сборки
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(USE_STATIC_RUNTIME "Use static C++ runtime" ON)

if(USE_STATIC_RUNTIME)
    if(MSVC)
        foreach(flag CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
                     CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
            if(${flag} MATCHES "/MD")
                string(REGEX REPLACE "/MD" "/MT" ${flag} "${${flag}}")
            endif()
        endforeach()
    endif()
endif()