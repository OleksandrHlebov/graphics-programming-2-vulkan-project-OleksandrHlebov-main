#include "SwapChain.h"
#include "Globals.h"
#include "Image.h"
#include "Device.h"

#include <stdexcept>
#include <algorithm>
#include <limits>

Swapchain::SupportDetails Swapchain::QuerySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	Swapchain::SupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

void Swapchain::Destroy(VkDevice device)
{
	for (auto& image : m_Images)
		vkDestroyImageView(device, *image.GetFirstViewPtr(), nullptr);

	for (auto& buffer : m_Framebuffers)
		vkDestroyFramebuffer(device, buffer, nullptr);

	vkDestroySwapchainKHR(device, m_SwapChain, nullptr);
}

Swapchain::Swapchain() = default;

void SwapchainBuilder::Build(std::unique_ptr<Swapchain>& swapchain, Device* device, VkSurfaceKHR surface, GLFWwindow* window, uint32_t imageCount)
{
	swapchain.reset(new Swapchain(Build(device, surface, window, imageCount)));
}

Swapchain SwapchainBuilder::Build(Device* device, VkSurfaceKHR surface, GLFWwindow* window, uint32_t imageCount)
{
	Swapchain swapchain{};

	Swapchain::SupportDetails swapChainSupport{ Swapchain::QuerySwapchainSupport(*device->GetPhysicalDevicePtr(), surface) };

	Device::QueueFamilyIndices indices{ device->FindQueueFamilies(surface) };
	VkSurfaceFormatKHR surfaceFormat{ ChooseSwapSurfaceFormat(swapChainSupport.formats) };
	VkPresentModeKHR presentMode{ ChooseSwapPresentMode(swapChainSupport.presentModes) };
	VkExtent2D extent{ ChooseSwapExtent(swapChainSupport.capabilities, window) };
	m_ImageCount = imageCount;
	if (swapChainSupport.capabilities.maxImageCount > 0 && m_ImageCount > swapChainSupport.capabilities.maxImageCount)
		m_ImageCount = swapChainSupport.capabilities.maxImageCount;
	if (m_ImageCount < swapChainSupport.capabilities.minImageCount)
		m_ImageCount = swapChainSupport.capabilities.minImageCount;

	if (!(swapChainSupport.capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT))
		throw std::runtime_error("surface cannot be destination");

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = m_ImageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	uint32_t queueFamilyIndices[]{ indices.graphicsFamily.value(), indices.presentFamily.value() };

	if (indices.graphicsFamily != indices.presentFamily)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	if (vkCreateSwapchainKHR(*device->GetDevicePtr(), &createInfo, nullptr, &swapchain.m_SwapChain) != VK_SUCCESS)
		throw std::runtime_error("failed to create swap chain");

	std::vector<VkImage> tempImages;


	vkGetSwapchainImagesKHR(*device->GetDevicePtr(), swapchain.m_SwapChain, &m_ImageCount, nullptr);
	tempImages.resize(m_ImageCount);
	swapchain.m_Images.resize(m_ImageCount, Image());
	vkGetSwapchainImagesKHR(*device->GetDevicePtr(), swapchain.m_SwapChain, &m_ImageCount, tempImages.data());
	swapchain.m_Extent = extent;
	swapchain.m_ImageFormat = surfaceFormat.format;

	for (uint32_t index{}; index < m_ImageCount; ++index)
	{
		swapchain.m_Images[index].m_Image = tempImages[index];
		swapchain.m_Images[index].m_Aspect = VK_IMAGE_ASPECT_COLOR_BIT;
		swapchain.m_Images[index].m_Extent = extent;
		swapchain.m_Images[index].m_Format = surfaceFormat.format;
	}



	ImageBuilder builder{};
	builder
		.SetAspect(VK_IMAGE_ASPECT_COLOR_BIT)
		.SetFormat(swapchain.m_ImageFormat);
	for (auto& image : swapchain.m_Images)
		builder.CreateView(image, *device->GetDevicePtr());

	return swapchain;
}

VkSurfaceFormatKHR SwapchainBuilder::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return availableFormat;
	}

	return availableFormats[0];
}

VkPresentModeKHR SwapchainBuilder::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	for (const auto& availablePresentMode : availablePresentModes)
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			return availablePresentMode;

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapchainBuilder::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		return capabilities.currentExtent;

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);

	VkExtent2D actualExtent{ static_cast<uint32_t>(width),
							 static_cast<uint32_t>(height) };

	actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

	return actualExtent;
}

void SwapchainFramebufferBuilder::Build(std::unique_ptr<Swapchain>& swapchain, VkDevice device, VkRenderPass renderPass)
{
	Build(*swapchain, device, renderPass);
}

void SwapchainFramebufferBuilder::Build(Swapchain& swapchain, VkDevice device, VkRenderPass renderPass)
{
	uint32_t imageCount = swapchain.m_Images.size();
	swapchain.m_Framebuffers.resize(imageCount);

	for (size_t index = 0; index < imageCount; ++index)
	{
		std::vector<VkImageView> attachments{ *(swapchain.m_Images[index].GetFirstViewPtr()) };
		attachments.insert(attachments.end(), m_Attachments.begin(), m_Attachments.end());

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = attachments.size();
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapchain.m_Extent.width;
		framebufferInfo.height = swapchain.m_Extent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapchain.m_Framebuffers[index]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create framebuffer");
		}
	}
}
