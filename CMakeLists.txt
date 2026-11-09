cmake_minimum_required(VERSION 3.16)

# Single source of truth for the firmware version. release-please bumps
# version.txt; the value ends up in esp_app_get_description()->version.
set(VERSION_FILE "${CMAKE_CURRENT_LIST_DIR}/version.txt")
file(READ "${VERSION_FILE}" PROJECT_VER)
string(STRIP "${PROJECT_VER}" PROJECT_VER)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${VERSION_FILE}")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(openpuck)
