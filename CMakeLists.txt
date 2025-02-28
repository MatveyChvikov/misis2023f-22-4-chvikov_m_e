cmake_minimum_required(VERSION 3.24)
project(EdgeResponseAnalyzer)

# C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(LIBS_PATH "${PROJECT_SOURCE_DIR}/libs")
set(GLEW_DIR "${LIBS_PATH}/glew")
set(GLFW_DIR "${LIBS_PATH}/glfw")
set(IMGUI_DIR "${LIBS_PATH}/imgui")
set(TINYFILEDIALOGS_DIR "${LIBS_PATH}/tinyfiledialogs")
set(JSON_DIR "${LIBS_PATH}/json")
set(MINGW_RUNTIME_DIR "${LIBS_PATH}/mingw-runtime")

# This function is used to force a build on a dependant project at cmake configuration phase.
function (build_external_project target prefix url branch) #FOLLOWING ARGUMENTS are the CMAKE_ARGS of ExternalProject_Add

    set(trigger_build_dir ${CMAKE_BINARY_DIR}/force_${target})

    #mktemp dir in build tree
    file(MAKE_DIRECTORY ${trigger_build_dir} ${trigger_build_dir}/build)

    #generate false dependency project
    set(CMAKE_LIST_CONTENT "
        cmake_minimum_required(VERSION 3.0)
        project(ExternalProjectCustom)
        include(ExternalProject)
        ExternalProject_add(${target}
            PREFIX ${prefix}/${target}
            GIT_REPOSITORY  ${url}
            GIT_TAG ${branch}
            CMAKE_ARGS ${ARGN})

        add_custom_target(trigger_${target})
        add_dependencies(trigger_${target} ${target})
    ")

    file(WRITE ${trigger_build_dir}/CMakeLists.txt "${CMAKE_LIST_CONTENT}")

    execute_process(COMMAND ${CMAKE_COMMAND} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} -G ${CMAKE_GENERATOR} ..
        WORKING_DIRECTORY ${trigger_build_dir}/build
    )
    execute_process(COMMAND ${CMAKE_COMMAND} --build .
        WORKING_DIRECTORY ${trigger_build_dir}/build
    )
endfunction()

if(WIN32)
    if(MSVC)
        message(FATAL_ERROR "MSVC build is not implemented. Try your luck with mingw")
    endif()

    set(GLEW_DIR "${LIBS_PATH}/glew-cmake")
    add_definitions(-DGLEW_STATIC)
    add_subdirectory(libs/glew-cmake)

    set(BUILD_PERF_TESTS OFF)
    set(BUILD_TESTS OFF)


    build_external_project(
	opencv
	${CMAKE_CURRENT_BINARY_DIR}/external
	https://github.com/opencv/opencv.git
    4.5.4
        -DBUILD_opencv_core=ON
        -DBUILD_opencv_highgui=ON
        -DBUILD_opencv_imgproc=ON
        -DBUILD_opencv_contrib=ON
        -DBUILD_DOCS:BOOL=FALSE
        -DBUILD_EXAMPLES:BOOL=FALSE
        -DBUILD_TESTS:BOOL=FALSE
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DBUILD_SHARED_LIBS:BOOL=FALSE
        -DBUILD_NEW_PYTHON_SUPPORT:BOOL=OFF
        -DBUILD_WITH_DEBUG_INFO=OFF
        -DWITH_CUDA:BOOL=FALSE
        -DWITH_FFMPEG:BOOL=FALSE
        -DWITH_MSMF:BOOL=FALSE
        -DWITH_IPP:BOOL=FALSE
        -DBUILD_PERF_TESTS:BOOL=FALSE
        -DBUILD_PNG:BOOL=ON
        -DBUILD_JPEG:BOOL=ON
        -DBUILD_TIFF:BOOL=ON
        -DBUILD_WITH_STATIC_CRT:BOOL=OFF
        -DBUILD_FAT_JAVA_LIB=OFF
        -DCMAKE_TOOLCHAIN_FILE:FILEPATH=${CMAKE_TOOLCHAIN_FILE}
	    -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR}/opencv/src/opencv
	)

	set(OpenCV_DIR "${CMAKE_CURRENT_BINARY_DIR}/external/opencv/src/opencv-build")

    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

    add_subdirectory(${GLFW_DIR})
endif()

set(BUILD_SHARED_LIBS OFF)
# Найти OpenGL и OpenCV
find_package(OpenCV REQUIRED)
find_package(OpenGL REQUIRED)

IF (NOT WIN32)
	find_package(GLEW REQUIRED)
ENDIF()


# Директории включений
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

# ImGui библиотека
file(GLOB IMGUI_SOURCES
       ${IMGUI_DIR}/*.cpp
       ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
	   ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp)


# Основной исполняемый файл
add_executable(EdgeResponseAnalyzer
    main.cpp
    functions.cpp
    ${TINYFILEDIALOGS_DIR}/tinyfiledialogs.c
    ${IMGUI_SOURCES}
)

# Библиотеки для Windows
if(WIN32 AND MSVC)
    set(GLEW_LIBRARY "${GLEW_DIR}/lib/Release/x64/glew32.lib")
    set(GLFW_LIBRARY "${GLFW_DIR}/lib-vc2022/glfw3.lib")
elseif(WIN32 AND MINGW)
    #set(GLEW_LIBRARY libglew_static)
    set(GLEW_LIBRARY "${CMAKE_BINARY_DIR}/libs/glew-cmake/lib/libglew.a")
    set(GLFW_LIBRARY glfw)
endif()

if(WIN32)
    target_link_libraries(EdgeResponseAnalyzer
        ${OPENGL_LIBRARY}
        ${GLEW_LIBRARY}
        ${GLFW_LIBRARY}
        ${OpenCV_LIBS}
        opengl32
        gdi32
        user32
        shell32
        comdlg32
    )
else()
    target_link_libraries(EdgeResponseAnalyzer
        ${OPENGL_LIBRARY} 
	    glfw
	    ${GLEW_LIBRARIES}
        ${OpenCV_LIBS}
    )
endif()

# Дополнительные включения для исполняемого файла
target_include_directories(EdgeResponseAnalyzer PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${OPENGL_INCLUDE_DIRS}
    ${GLEW_DIR}/include
    ${GLFW_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
    ${TINYFILEDIALOGS_DIR}
    ${JSON_DIR}/include
    ${CMAKE_SOURCE_DIR}
)

if(WIN32)
    set(BINARY_DIR ${CMAKE_BINARY_DIR}/bin)
    file(MAKE_DIRECTORY ${BINARY_DIR})
else()
    set(BINARY_DIR ${CMAKE_BINARY_DIR})
endif()

# Копирование шейдеров
file(COPY 
    ${CMAKE_SOURCE_DIR}/VertexShader.glsl
    ${CMAKE_SOURCE_DIR}/FragmentShader.glsl
    DESTINATION ${BINARY_DIR}
)

# Копирование DLL для Windows
if(WIN32 AND MSVC)
    # GLEW
    add_custom_command(TARGET EdgeResponseAnalyzer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${GLEW_DIR}/bin/Release/x64/glew32.dll"
        $<TARGET_FILE_DIR:EdgeResponseAnalyzer>
    )

    # GLFW
    add_custom_command(TARGET EdgeResponseAnalyzer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${GLFW_DIR}/lib-vc2022/glfw3.dll"
        $<TARGET_FILE_DIR:EdgeResponseAnalyzer>
    )

    # OpenCV
    foreach(OCVFile ${OpenCV_LIBS})
        add_custom_command(TARGET EdgeResponseAnalyzer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OpenCV_DIR}/bin/Release/${OCVFile}d.dll"
            $<TARGET_FILE_DIR:EdgeResponseAnalyzer>
        )
    endforeach()
elseif(WIN32 AND MINGW)
    file(GLOB MINGW_RUNTIME_DLLS
      "${MINGW_RUNTIME_DIR}/*.dll"
    )
    file(COPY ${MINGW_RUNTIME_DLLS} DESTINATION ${BINARY_DIR})
endif()

# Выходной каталог
set_target_properties(EdgeResponseAnalyzer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${BINARY_DIR}
)

# Режим сборки и предупреждения
set(CMAKE_BUILD_TYPE Debug)

if(MSVC)
    target_compile_options(EdgeResponseAnalyzer PRIVATE /W4)
else()
    target_compile_options(EdgeResponseAnalyzer PRIVATE -Wall -Wextra)
endif()

# Дополнительные определения для MSVC
if(MSVC)
    target_compile_definitions(EdgeResponseAnalyzer PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
endif()

# Настройка путей для поиска DLL
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
if(WIN32 AND MSVC)
    set(CMAKE_INSTALL_RPATH "${OpenCV_DIR}/bin;${GLEW_DIR}/bin;${GLFW_DIR}/lib-vc2022")
endif()

# Установка
install(TARGETS EdgeResponseAnalyzer
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
