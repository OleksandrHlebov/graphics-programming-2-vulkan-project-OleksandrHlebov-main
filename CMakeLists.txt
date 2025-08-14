# CMakeList.txt : CMake project for vulkan, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project("GP_Vulkan")

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	add_compile_options(-Wno-error=unknown-pragmas)
endif ()

include(FetchContent)
set(FETCHCONTENT_BASE_DIR ${PROJECT_SOURCE_DIR}/libs CACHE PATH "" FORCE)
FetchContent_Declare(  
	glfw
	URL https://github.com/glfw/glfw/archive/refs/tags/3.4.tar.gz
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
	glm
	URL https://github.com/g-truc/glm/archive/refs/tags/1.0.1.tar.gz
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
	assimp
	URL https://github.com/assimp/assimp/archive/refs/tags/v5.4.3.tar.gz
)
FetchContent_MakeAvailable(assimp)

set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
set(ASSIMP_BUILD_TESTS OFF)

add_executable(GP_Vulkan "src/main.cpp"  "src/Instance.cpp" "inc/ColorDefines.h" "inc/Globals.h" "src/DebugMessenger.cpp" "inc/SwapChain.h" "src/SwapChain.cpp" "inc/TempHelpers.h" "src/TempHelpers.cpp" "inc/Device.h" "src/Device.cpp" "inc/DeletionQueue.h" "inc/RenderPass.h" "inc/Subpass.h" "src/RenderPass.cpp" "inc/DescriptorSetLayout.h" "inc/PipelineLayout.h" "inc/Pipeline.h"  "src/Pipeline.cpp" "inc/DataTypes.h" "src/Image.cpp" "inc/CommandPool.h"  "src/CommandPool.cpp" "inc/Buffer.h" "src/Buffer.cpp" "inc/DescriptorPool.h" "src/DescriptorPool.cpp" "inc/DescriptorSet.h" "src/DescriptorSet.cpp" "src/Helper.cpp" "inc/Scene.h" "src/Scene.cpp" "inc/Mesh.h" "inc/ShaderStage.h" "src/ShaderStage.cpp" "inc/Sampler.h" "inc/Application.h" "inc/DynamicRenderingApp.h" "src/DynamicRenderingApp.cpp" "inc/Camera.h" "inc/WorldTime.h" "src/WorldTime.cpp")
target_include_directories(${PROJECT_NAME} PRIVATE "inc/" "libs/assimp-src/include/" "libs/assimp-src/contrib/stb/")

find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan glfw glm::glm-header-only assimp::assimp)

make_directory(${CMAKE_BINARY_DIR}/shaders)
add_compile_definitions(GLM_FORCE_DEPTH_ZERO_TO_ONE)
add_compile_definitions(GLM_FORCE_RADIANS)
add_compile_definitions(GLM_ENABLE_EXPERIMENTAL)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${PROJECT_SOURCE_DIR}/compile_shader.bat ${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR}
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/resources ${PROJECT_BINARY_DIR}/resources)