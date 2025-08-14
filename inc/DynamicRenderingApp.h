#pragma once
#include "Application.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <memory>
#include <string>
#include "DeletionQueue.h"
#include "DataTypes.h"
#include "Globals.h"
#include "Scene.h"
#include "SwapChain.h"

class Image;
class Buffer;
class CommandBuffer;
class CommandPool;
class DescriptorPool;
class Pipeline;
class PipelineLayout;
class DescriptorSetLayout;
class Swapchain;
class Device;
class DebugMessenger;
class Instance;
class DescriptorSet;
class Sampler;
class Camera;
class ShaderStage;

class DynamicRenderingApp final : public Application
{
public:
	DynamicRenderingApp();
	~DynamicRenderingApp();

	void Run() override;

	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

	static void MouseMovedCallback(GLFWwindow* window, double xpos, double ypos);

private:
	void RenderToCubeMap(ShaderStage* vertexShader, ShaderStage* pixelShader
						 , Image* inputImage, Sampler* sampler
						 , Image* outputCubeMapImage);

	void GenerateShadowMap();

	void InitWindow();

	void CreateSwapchain();

	void CreateDepthResources();

	void CreateSurface();

	void InitVulkan();

	std::vector<const char*> GetRequiredExtensions();

	void CreateTextureSampler();

	void CreateDescriptorSets();

	VkFormat FindDepthFormat();

	void CreateSyncObjects();

	static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);

	void RecreateSwapChain();

	void RecordCommandBufferWithPrepass(CommandBuffer& commandBuffer, uint32_t imageIndex);

	void DrawFrame();

	void UpdateUniformBuffer(uint32_t currentImage);

	void SubmitQueue();

	void Present(uint32_t imageIndex);

	void MainLoop();

	void End();

	DeletionQueue m_DeletionQueue;

	const std::string m_AppName{ "Refactor" };

	template<typename T>
	using uptr = std::unique_ptr<T>;

	GLFWwindow* m_WindowPtr{};
	uptr<Instance>				m_InstancePtr;
	uptr<DebugMessenger>		m_DebugMessengerPtr;
	uptr<Device>				m_DevicePtr;
	uptr<Swapchain>				m_SwapChainPtr;
	uptr<DescriptorSetLayout>	m_GlobalSetLayoutPtr;
	uptr<DescriptorSetLayout>	m_LocalSetLayoutPtr;
	uptr<DescriptorSetLayout>	m_FrameDescriptorSetLayoutPtr;
	uptr<PipelineLayout>		m_PrepassPipelineLayoutPtr;
	uptr<PipelineLayout>		m_GBufferPipelineLayoutPtr;
	uptr<PipelineLayout>		m_LightingPipelineLayoutPtr;
	uptr<Pipeline>				m_PrepassPipelinePtr;
	uptr<Pipeline>				m_GBufferPipelinePtr;
	uptr<Pipeline>				m_LightingPipelinePtr;
	uptr<Pipeline>				m_BlitPipelinePtr;
	uptr<CommandPool>			m_CommandPoolPtr;
	uptr<DescriptorPool>		m_DescriptorPoolPtr; 
	 
	uptr<Camera>	m_CameraPtr	{};
	uptr<Scene>		m_ScenePtr	{ std::make_unique<Scene>() };
	
	// save states of builders to reuse them with the same parameters
	SwapchainBuilder			m_SwapchainBuilder{};

	VkSurfaceKHR				m_Surface;
	 
	uptr<Image>		m_DepthTexturePtr;
	uptr<Image>		m_AlbedoTexturePtr;
	uptr<Image>		m_MaterialPropsTexturePtr; 
	uptr<Image>		m_HDRRenderTargetPtr;
	uptr<Image>		m_CubeMapPtr; 
	uptr<Image>		m_DiffuseIrradiancePtr;
	std::vector<Image> m_ShadowDepthMaps;

	uptr<Sampler>	m_TextureSamplerPtr;
	uptr<Sampler>	m_ShadowSamplerPtr;

	std::vector<datatype::PointLight> m_PointLights;
	std::vector<datatype::DirectionalLight> m_DirectionalLights;
	  
	std::vector<Buffer>			m_MVPUBuffers;
	std::vector<Buffer>			m_PointLightsSSBO;
	std::vector<Buffer>			m_DirectionalLightsSSBO;
	std::vector<DescriptorSet>	m_GlobalDescriptorSets;
	std::vector<DescriptorSet>	m_LocalDescriptorSets;
	std::vector<DescriptorSet>	m_FrameDescriptorSets;
	 
	std::vector<CommandBuffer>	m_CommandBuffers;
	std::vector<VkSemaphore>	m_ImageAvailableSemaphores;
	std::vector<VkSemaphore>	m_RenderFinishedSemaphores;
	std::vector<VkFence>		m_InFlightFences; 

	bool m_IsFramebufferResized{};
	 
	const std::vector<const char*> m_DeviceExtensions{ VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME, VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME, VK_EXT_CUSTOM_BORDER_COLOR_EXTENSION_NAME };

	const std::vector<const char*> m_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };
	
	//std::vector<Mesh*> m_Meshes;
	
	uint32_t m_CurrentFrame{};
	inline static const uint32_t WIDTH{ 1280 };
	inline static const uint32_t HEIGHT{ 720 };
};
