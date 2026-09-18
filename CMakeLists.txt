cmake_minimum_required(VERSION 3.22)
project(ShortestPathApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/ui")

if (DEFINED ENV{QT_DIR})
    list(PREPEND CMAKE_PREFIX_PATH "$ENV{QT_DIR}")
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(APP_SOURCES
        src/main.cpp
        src/MainWindow.cpp
        src/MainWindow.h
        src/GraphWidget.cpp
        src/GraphWidget.h
        src/GraphModel.cpp
        src/GraphModel.h
        src/ShortestPath.cpp
        src/ShortestPath.h
        src/Theme.cpp
        src/Theme.h
        src/GraphConstants.h
        src/GraphFormat.h
        ui/MainWindow.ui
)

add_executable(${PROJECT_NAME} ${APP_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/ui"
)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets)

function(grafo_warnings target)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -Wall -Wextra)
    endif()
endfunction()

grafo_warnings(${PROJECT_NAME})

enable_testing()
add_executable(ShortestPathTests
        tests/test_shortest_path.cpp
        tests/test_graph_model.cpp
        tests/test_support.h
        src/ShortestPath.cpp
        src/GraphModel.cpp
        src/GraphModel.h
        src/ShortestPath.h
        src/GraphConstants.h
        src/GraphFormat.h
)
target_include_directories(ShortestPathTests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests"
)
target_link_libraries(ShortestPathTests PRIVATE Qt6::Core)
set_target_properties(ShortestPathTests PROPERTIES
        AUTOMOC OFF
        AUTOUIC OFF
        AUTORCC OFF
)
grafo_warnings(ShortestPathTests)
add_test(NAME ShortestPathTests COMMAND ShortestPathTests)
set_tests_properties(ShortestPathTests PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
)

if (WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${CMAKE_PREFIX_PATH}/bin")
    if (WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND "${WINDEPLOYQT_EXECUTABLE}" --compiler-runtime
                "$<TARGET_FILE:${PROJECT_NAME}>"
                COMMENT "windeployqt: copiando DLLs de Qt y runtime de MinGW junto al .exe")
    else()
        message(WARNING "No se encontró windeployqt en ${CMAKE_PREFIX_PATH}/bin")
    endif()
endif()
