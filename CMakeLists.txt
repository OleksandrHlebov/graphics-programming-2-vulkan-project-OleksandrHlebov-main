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

find_program(GLSLANG glslang)

if (!GLSLANG)
	message(FATAL_ERROR "Shader compiler not found")
endif ()
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan glfw glm::glm-header-only assimp::assimp)

make_directory(${CMAKE_BINARY_DIR}/shaders)
add_compile_definitions(GLM_FORCE_DEPTH_ZERO_TO_ONE)
add_compile_definitions(GLM_FORCE_RADIANS)
add_compile_definitions(GLM_ENABLE_EXPERIMENTAL)

set(SHADER_SOURCES
    "basic_fragment_shader.frag"
    "basic_triangle_shader.vert"
	"blit.frag"
    "cubemap.vert"
    "depth_prepass.frag"
	"diffuse_irradiance.frag"
"environment.frag"
"gbuffer_generation.frag"
"gbuffer_generation.vert"
"lighting.frag"
"lighting.vert"
"quad_shader.vert"
"shadow_prepass.frag"
"shadow_prepass.vert")

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)
macro(Shader_Compile name)
	get_filename_component(SHADER_FILENAME_WE ${name} NAME_WE)
	get_filename_component(SHADER_EXT ${name} EXT)
	string(SUBSTRING "${SHADER_EXT}" 1 -1 SHADER_EXT) # Remove the leading dot
	set(SHADER_SOURCE ${PROJECT_SOURCE_DIR}/shaders/${name})
	set(SHADER_SPIRV_PATH ${CMAKE_BINARY_DIR}/shaders/${SHADER_FILENAME_WE}_${SHADER_EXT}.spv)

	add_custom_command(OUTPUT ${SHADER_SPIRV_PATH}
	                   COMMAND glslang -V --target-env vulkan1.3 ${SHADER_SOURCE} -o ${SHADER_SPIRV_PATH}
	                   DEPENDS ${SHADER_SOURCE}
	                   COMMENT "Compiled ${SHADER_SPIRV_PATH}")
	list(APPEND COMPILED_SHADERS ${SHADER_SPIRV_PATH})
endmacro()

foreach (shader IN LISTS SHADER_SOURCES)
	Shader_Compile(${shader})
endforeach ()

add_custom_target(CompileShaders DEPENDS ${COMPILED_SHADERS})
add_dependencies(${PROJECT_NAME} CompileShaders)