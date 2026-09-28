# ============================================================
# 🧩 CMakeLists.txt – ShortestPathApp (Qt 6)
# ============================================================

cmake_minimum_required(VERSION 3.22)
project(ShortestPathApp LANGUAGES CXX)

# ------------------------------------------------------------
# ⚙️ Configuración general del compilador
# ------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ------------------------------------------------------------
# 📁 Rutas adicionales para los archivos .ui
# ------------------------------------------------------------
set(CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/ui")

# ------------------------------------------------------------
# 🔍 Buscar Qt6
# ------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Test)

# ------------------------------------------------------------
# ⚠️ Advertencias del compilador
# ------------------------------------------------------------
function(enable_warnings target)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -Wall -Wextra)
    elseif (MSVC)
        target_compile_options(${target} PRIVATE /W4)
    endif()
endfunction()

# ------------------------------------------------------------
# 🧠 Núcleo: modelo del grafo y algoritmos (solo QtCore)
# ------------------------------------------------------------
add_library(graphcore STATIC
        src/Graph.cpp src/Graph.h
        src/ShortestPath.cpp src/ShortestPath.h
        src/Weight.cpp src/Weight.h
        src/Layout.cpp src/Layout.h
)
target_include_directories(graphcore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(graphcore PUBLIC Qt6::Core)
enable_warnings(graphcore)

# ------------------------------------------------------------
# 🖼️ Interfaz: ventana, lienzo y mensajes (librería para poder probarla)
# ------------------------------------------------------------
add_library(appui STATIC
        src/MainWindow.cpp src/MainWindow.h
        src/GraphWidget.cpp src/GraphWidget.h
        src/ResultBanner.cpp src/ResultBanner.h
        src/Messages.cpp src/Messages.h
        src/Theme.cpp src/Theme.h
        ui/MainWindow.ui
)
target_include_directories(appui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(appui PUBLIC graphcore Qt6::Widgets)
enable_warnings(appui)

# ------------------------------------------------------------
# 🖥️ Aplicación
# ------------------------------------------------------------
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE appui)
enable_warnings(${PROJECT_NAME})

if (WIN32)
    # Busca windeployqt junto a qmake de la instalación de Qt encontrada
    get_target_property(QT_QMAKE_LOCATION Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BIN_DIR "${QT_QMAKE_LOCATION}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE NAMES windeployqt windeployqt6 HINTS "${QT_BIN_DIR}")
    if (WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND "${WINDEPLOYQT_EXECUTABLE}" --compiler-runtime
                "$<TARGET_FILE:${PROJECT_NAME}>"
                # Con el Qt de MSYS2 faltan además sus dependencias (libstdc++, ICU…)
                COMMAND "${CMAKE_COMMAND}"
                -DEXE=$<TARGET_FILE:${PROJECT_NAME}>
                -DSEARCH_DIR=${QT_BIN_DIR}
                -DOBJDUMP=${CMAKE_OBJDUMP}
                -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CopyRuntimeDeps.cmake"
                COMMENT "windeployqt: copiando DLLs de Qt y runtime de MinGW junto al .exe"
                VERBATIM)
    else()
        message(WARNING "No se encontró windeployqt en ${QT_BIN_DIR}")
    endif()
endif()

# ------------------------------------------------------------
# 🧪 Tests (ctest)
# ------------------------------------------------------------
enable_testing()

add_executable(tst_shortestpath tests/tst_shortestpath.cpp)
target_link_libraries(tst_shortestpath PRIVATE graphcore Qt6::Test)
enable_warnings(tst_shortestpath)
add_test(NAME tst_shortestpath COMMAND tst_shortestpath)

add_executable(tst_mainwindow tests/tst_mainwindow.cpp)
target_link_libraries(tst_mainwindow PRIVATE appui Qt6::Test)
enable_warnings(tst_mainwindow)
add_test(NAME tst_mainwindow COMMAND tst_mainwindow)
# Los tests usan las DLL y plugins de la instalación de Qt (windeployqt solo
# copia el plugin «windows» junto al .exe, y los tests de interfaz usan «offscreen»).
get_target_property(QT_CORE_LOCATION Qt6::Core IMPORTED_LOCATION_DEBUG)
if (NOT QT_CORE_LOCATION)
    get_target_property(QT_CORE_LOCATION Qt6::Core IMPORTED_LOCATION_RELEASE)
endif()
get_filename_component(QT_DLL_DIR "${QT_CORE_LOCATION}" DIRECTORY)
set_tests_properties(tst_shortestpath tst_mainwindow PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${QT_DLL_DIR}")
set_tests_properties(tst_mainwindow PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_PLUGIN_PATH=${QT6_INSTALL_PREFIX}/${QT6_INSTALL_PLUGINS}")
