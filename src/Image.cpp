#include "Image.h"
#include "TempHelpers.h"
#include "Device.h"
#include <stdexcept>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <variant>
#include "CommandPool.h"
#include "Buffer.h"

void Image::DestroyExtraViews(Device* device)
{
	for (int index{ 1 }; index < m_Views.size(); ++index)
	{
		vkDestroyImageView(*device->GetDevicePtr(), m_Views[index], nullptr);
	}
	m_Views.erase(m_Views.begin() + 1, m_Views.end());
}

void Image::MakeTransition(Device* device, CommandBuffer* command, const Transition& transition)
{
	VkImageMemoryBarrier barrier{}; 
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = m_CurrentLayout; 
	barrier.newLayout = transition.newLayout;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; 
	
	barrier.image = m_Image;
	barrier.subresourceRange.aspectMask = m_Aspect;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.layerCount = transition.layerCount;
	barrier.subresourceRange.baseArrayLayer = transition.baseLayerLevel;

	barrier.srcAccessMask = transition.srcAccess;
	barrier.dstAccessMask = transition.dstAccess;

	vkCmdPipelineBarrier(
		*command->GetBufferPtr(),
		transition.srcStage, transition.dstStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	m_CurrentLayout = transition.newLayout;
}

void Image::Destroy(VkDevice device)
{
	vkDestroyImage(device, m_Image, nullptr);
	for (VkImageView& imageView : m_Views)
		vkDestroyImageView(device, imageView, nullptr);
	vkFreeMemory(device, m_Memory, nullptr);
} 

ImageBuilder& ImageBuilder::SetFormat(VkFormat format)
{
	m_Format = format;
	return *this;
}

ImageBuilder& ImageBuilder::SetTiling(VkImageTiling tiling)
{
	m_Tiling = tiling;
	return *this;
}

ImageBuilder& ImageBuilder::SetFlags(VkImageCreateFlags flags)
{
	m_Flags = flags;
	return *this;
}

ImageBuilder& ImageBuilder::SetDimensions(uint32_t width, uint32_t height)
{
	m_Width = width;
	m_Height = height;
	return *this;
}

ImageBuilder& ImageBuilder::SetLayers(uint32_t layers)
{
	m_Layers = layers;
	return *this;
}

ImageBuilder& ImageBuilder::SetViewType(VkImageViewType type)
{
	m_ViewType = type;
	return *this;
}

ImageBuilder& ImageBuilder::SetImageType(VkImageType type)
{
	m_ImageType = type;
	return *this;
}

ImageBuilder& ImageBuilder::SetInitialLayout(VkImageLayout layout)
{
	m_InitialLayout = layout;
	return *this;
}

ImageBuilder& ImageBuilder::SetAspect(VkImageAspectFlags aspect)
{
	m_Aspect = aspect;
	return *this;
}

ImageBuilder& ImageBuilder::SetFilePath(const std::string& path)
{
	m_FilePath = path;
	return *this;
}

void ImageBuilder::Build(std::unique_ptr<Image>& image, Device* device, CommandPool* commandPool, VkImageUsageFlags usage, VkMemoryPropertyFlags properties)
{
	image.reset(new Image());
	Build(*image, device, commandPool, usage, properties);
}

void ImageBuilder::Build(Image& image, Device* device, CommandPool* commandPool, VkImageUsageFlags usage, VkMemoryPropertyFlags properties)
{
	std::optional<Buffer> stagingBuffer{};
	// if loaded from file first read the file to get extent info
	if (!m_FilePath.empty())
	{
		int textureWidth{};
		int textureHeight{};
		int textureChannels{};

		VkDeviceSize imageSize{ 1 };
		std::variant<stbi_uc*, float*> pixels;
		if (m_Format == VK_FORMAT_R32G32B32A32_SFLOAT)
		{
			imageSize *= sizeof(float);
			pixels = stbi_loadf(m_FilePath.c_str(), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);
		}
		else
			pixels = stbi_load(m_FilePath.c_str(), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);
		
		imageSize *= static_cast<VkDeviceSize>(textureWidth * textureHeight * 4);

		if (!std::get_if<0>(&pixels) && !std::get_if<1>(&pixels))
			throw std::runtime_error("failed to load texture " + m_FilePath);

		BufferBuilder builder{};
		stagingBuffer = builder
			.MapMemory()
			.Build(device, commandPool, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		m_Width = static_cast<uint32_t>(textureWidth);
		m_Height = static_cast<uint32_t>(textureHeight);

		stagingBuffer->UpdateMappedData(((m_Format == VK_FORMAT_R32G32B32A32_SFLOAT) ? (void*)std::get<float*>(pixels) : (void*)std::get<stbi_uc*>(pixels)), imageSize, 0);

		stbi_image_free(((m_Format == VK_FORMAT_R32G32B32A32_SFLOAT) ? (void*)std::get<float*>(pixels) : (void*)std::get<stbi_uc*>(pixels)));
	}

	image.m_Extent.width = m_Width;
	image.m_Extent.height = m_Height;
	image.m_Format = m_Format;
	image.m_Aspect = m_Aspect;
	image.m_CurrentLayout = m_InitialLayout;

	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = m_ImageType;
	imageInfo.extent.width = m_Width;
	imageInfo.extent.height = m_Height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = m_Layers;
	imageInfo.format = m_Format;
	imageInfo.tiling = m_Tiling;
	imageInfo.initialLayout = m_InitialLayout;
	imageInfo.usage = usage | (!m_FilePath.empty() * VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = m_Flags;

	image.m_Layers = m_Layers;

	if (vkCreateImage(*device->GetDevicePtr(), &imageInfo, nullptr, &image.m_Image) != VK_SUCCESS)
		throw std::runtime_error("failed to create image");

	VkMemoryRequirements memRequirements{};
	vkGetImageMemoryRequirements(*device->GetDevicePtr(), image.m_Image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = device->FindMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(*device->GetDevicePtr(), &allocInfo, nullptr, &image.m_Memory) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate image memory");

	vkBindImageMemory(*device->GetDevicePtr(), image.m_Image, image.m_Memory, 0);

	// after VkImage is created copy loaded image to VkImage
	if (!m_FilePath.empty())
	{
		// transition to dst
		SingleTimeCommand command = commandPool->AllocateSingleTimeCommand(*device->GetDevicePtr());

		command.Start();
		{
			Image::Transition transition{};
			transition.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			transition.srcAccess = 0;
			transition.dstAccess = VK_ACCESS_TRANSFER_WRITE_BIT;
			transition.srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			transition.dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			image.MakeTransition(device, &command, transition);
		}
		stagingBuffer->CopyTo(&image, &command, device, commandPool);
		// transition to read
		{
			Image::Transition transition{};
			transition.newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
			transition.srcAccess = VK_ACCESS_TRANSFER_WRITE_BIT;
			transition.dstAccess = VK_ACCESS_SHADER_READ_BIT;
			transition.srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			transition.dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			image.MakeTransition(device, &command, transition);
		}
		command.End(device);
		
		stagingBuffer->Destroy(device);
	}

	CreateView(image, *device->GetDevicePtr());
}

Image ImageBuilder::Build(Device* device, CommandPool* commandPool, VkImageUsageFlags usage, VkMemoryPropertyFlags properties)
{
	Image temp{};

	Build(temp, device, commandPool, usage, properties);

	return temp;
}

void ImageBuilder::CreateView(Image& image, VkDevice device)
{
	image.m_Views.resize(m_Layers);
	if (m_ViewType == VK_IMAGE_VIEW_TYPE_CUBE)
	{
		image.m_Views.emplace_back();
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image.m_Image;
		viewInfo.format = m_Format;
		viewInfo.viewType = m_ViewType;
		viewInfo.subresourceRange.aspectMask = m_Aspect;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.layerCount = 6;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		if (vkCreateImageView(device, &viewInfo, nullptr, &image.m_Views[0]) != VK_SUCCESS)
			throw std::runtime_error("failed to create texture image view");
	}

	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image.m_Image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = m_Format;
	viewInfo.subresourceRange.aspectMask = m_Aspect;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.layerCount = 1;
	for (int index{}; index < m_Layers; ++index)
	{
		viewInfo.subresourceRange.baseArrayLayer = index;
		if (vkCreateImageView(device, &viewInfo, nullptr, &image.m_Views[index + 1 * (m_ViewType == VK_IMAGE_VIEW_TYPE_CUBE)]) != VK_SUCCESS)
			throw std::runtime_error("failed to create texture image view");
	}

}
