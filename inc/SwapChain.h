#pragma once
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <memory>
#include "TempHelpers.h"

class Device;
class Image;
class Swapchain final
{
public:

	struct SupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	~Swapchain() = default;

	static Swapchain::SupportDetails QuerySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

	void Destroy(VkDevice device);

	VkSwapchainKHR* GetSwapchainPtr()	{ return &m_SwapChain; }
	VkFormat*		GetFormatPtr()		{ return &m_ImageFormat; }
	VkExtent2D*		GetExtentPtr()		{ return &m_Extent; }
	std::vector<Image>& GetImages()		{ return m_Images; }

	const std::vector<VkFramebuffer>& GetFrameBuffers() { return m_Framebuffers; }

private:
	friend class SwapchainBuilder;
	friend class SwapchainFramebufferBuilder;
	Swapchain();

	VkSwapchainKHR			 m_SwapChain;
	VkFormat				 m_ImageFormat;
	VkExtent2D				 m_Extent;

	std::vector<Image>			m_Images;
	std::vector<VkFramebuffer>	m_Framebuffers;
};

class SwapchainBuilder final
{
public:
	SwapchainBuilder() = default;
	~SwapchainBuilder() = default;
	
	SwapchainBuilder(const SwapchainBuilder&) 					= delete;
	SwapchainBuilder(SwapchainBuilder&&) noexcept 				= delete;
	SwapchainBuilder& operator=(const SwapchainBuilder&) 	 	= delete;
	SwapchainBuilder& operator=(SwapchainBuilder&&) noexcept 	= delete;

	void Build(std::unique_ptr<Swapchain>& swapchain, Device* device, VkSurfaceKHR surface, GLFWwindow* window, uint32_t imageCount);
	Swapchain Build(Device* device, VkSurfaceKHR surface, GLFWwindow* window, uint32_t imageCount);

private:
	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);

	uint32_t m_ImageCount{ 3 };

};

class SwapchainFramebufferBuilder final
{
public:
	SwapchainFramebufferBuilder() = default;
	~SwapchainFramebufferBuilder() = default;
	
	SwapchainFramebufferBuilder(const SwapchainFramebufferBuilder&) 				= delete;
	SwapchainFramebufferBuilder(SwapchainFramebufferBuilder&&) noexcept 			= delete;
	SwapchainFramebufferBuilder& operator=(const SwapchainFramebufferBuilder&) 	 	= delete;
	SwapchainFramebufferBuilder& operator=(SwapchainFramebufferBuilder&&) noexcept 	= delete;
	
	SwapchainFramebufferBuilder& AddAttachment(VkImageView attachment) { m_Attachments.emplace_back(attachment); return *this; }

	void Build(std::unique_ptr<Swapchain>& swapchain, VkDevice device, VkRenderPass renderPass);
	void Build(Swapchain& swapchain, VkDevice device, VkRenderPass renderPass);

private:
	std::vector<VkImageView> m_Attachments;
	
};