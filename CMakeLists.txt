cmake_minimum_required(VERSION 3.16)
project(diffyinajiffy VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# Find additional libraries for document handling (optional)
find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_check_modules(POPPLER poppler-qt6)
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/diffview.cpp
    src/diffengine.cpp
    src/documentparser.cpp
    src/folderview.cpp
)

set(HEADERS
    src/mainwindow.h
    src/diffview.h
    src/diffengine.h
    src/documentparser.h
    src/folderview.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

if(POPPLER_FOUND)
    target_link_libraries(${PROJECT_NAME} ${POPPLER_LIBRARIES})
    target_include_directories(${PROJECT_NAME} PRIVATE ${POPPLER_INCLUDE_DIRS})
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_POPPLER)
    message(STATUS "Poppler-Qt6 found - PDF support enabled")
else()
    message(WARNING "Poppler-Qt6 not found - PDF support disabled")
endif()

# Install
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
