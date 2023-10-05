cmake_minimum_required(VERSION 3.20)
set(CMAKE_TOOLCHAIN_FILE E:/CANA/vs-code-template-rocket-league-plugin-with-aws-sdk/Rocket_League_Plugin_with_AWS_SDK/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file")

project(Dashboard)

set(CMAKE_CXX_STANDARD 20)
set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "")

  
set(CMAKE_BUILD_TYPE Release)

# Statically link all libraries to DLL runtime on Windows
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
if(POLICY CMP0091)
  cmake_policy(SET CMP0091 NEW)
endif()

# Set the directories that should be included in the build command
include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR})

# VPCKG Integrations via PkgConfig
set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH};E:/CANA/vs-code-template-rocket-league-plugin-with-aws-sdk/vcpkg_installed/x64-windows-static/lib/pkgconfig")


#### BakkesMod SDK ####
set(BAKKESMOD_SDK_PATH "E:/CANA/vs-code-template-rocket-league-plugin-with-aws-sdk/Rocket_League_Plugin_with_AWS_SDK/BakkesModSDK")
link_directories("${BAKKESMOD_SDK_PATH}/lib")


#### AWS VCPKG Libraries (vcpkg.json) ####
set(TIFF_LIBRARIES ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/tiff.lib)
find_package(AWSSDK CONFIG COMPONENTS core dynamodb REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(protobuf REQUIRED)


#### BakkesMod VCPKG Libraries (vcpkg.json) ####
find_package(imgui REQUIRED)
find_package(fmt REQUIRED)

#### NLOHMANN JSON VCPKG Library (vcpkg.json) ####
find_package(nlohmann_json CONFIG REQUIRED)


# The main plugin file
set(SOURCE_FILES Dashboard.cpp)

# Create DLL
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
                          ${BAKKESMOD_SDK_PATH}/include
                          ${AWSSDK_INCLUDE_DIRS} 
                          ${TIFF_INCLUDE_DIRS}
                          ${nlohmann_json_INCLUDE_DIRS}
                          )

# Link static libraries to DLL
target_link_libraries(${PROJECT_NAME}
                      pluginsdk
                      ${AWSSDK_LIBRARIES}
                      ${TIFF_LIBRARIES}
                      ZLIB::ZLIB
                      PNG::PNG
                      protobuf::libprotobuf-lite
                      imgui::imgui
                      fmt::fmt-header-only
                      nlohmann_json::nlohmann_json
)

# Bundle system runtime libraries into DLL
include(InstallRequiredSystemLibraries)
install(FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/bin")