#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <memory>
#include <vector>

class Device;
class CommandBuffer;
class CommandPool;
class Buffer;
class Image final
{
public:
	struct Transition
	{
		VkImageLayout		 newLayout;
		VkAccessFlags2		 srcAccess;
		VkAccessFlags2		 dstAccess;
		VkPipelineStageFlags srcStage;
		VkPipelineStageFlags dstStage;
		uint32_t			 layerCount{ 1 };
		uint32_t			 baseLayerLevel{};
	};

	~Image() = default;

	VkImage*			GetImagePtr()		{ return &m_Image; }
	VkImageView*		GetFirstViewPtr()	{ return &m_Views[0]; }
	std::vector<VkImageView>& GetViews()	{ return m_Views; }
	VkImageLayout		GetCurrentLayout()	{ return m_CurrentLayout; }
	VkImageAspectFlags	GetAspect()			{ return m_Aspect; }
	VkExtent2D			GetExtent()			{ return m_Extent; }
	VkFormat			GetFormat()			{ return m_Format; }
	uint32_t			GetLayers()			{ return m_Layers; }

	void DestroyExtraViews(Device* device);

	void MakeTransition(Device* device, CommandBuffer* command, const Transition& transition);
	 
	void Destroy(VkDevice device);

private:
	friend class Buffer;
	friend class SwapchainBuilder;
	friend class ImageBuilder;
	Image() = default;
		
	VkImage						m_Image;
	std::vector<VkImageView>	m_Views; // regret of making image views part of image
	VkDeviceMemory				m_Memory;
	VkExtent2D					m_Extent;
	VkFormat					m_Format;
	VkImageAspectFlags			m_Aspect;
	uint32_t					m_Layers;

	VkImageLayout	m_CurrentLayout{ VK_IMAGE_LAYOUT_UNDEFINED };
};
 
class ImageBuilder final
{
public:
	ImageBuilder()	= default;
	~ImageBuilder() = default;
	
	ImageBuilder(const ImageBuilder&) 					= delete;
	ImageBuilder(ImageBuilder&&) noexcept 				= delete;
	ImageBuilder& operator=(const ImageBuilder&) 	 	= delete;
	ImageBuilder& operator=(ImageBuilder&&) noexcept 	= delete;

	ImageBuilder& SetFormat(VkFormat format);

	ImageBuilder& SetTiling(VkImageTiling tiling);

	ImageBuilder& SetFlags(VkImageCreateFlags flags);

	// required field
	// ignored if loaded from file
	ImageBuilder& SetDimensions(uint32_t width, uint32_t height);

	ImageBuilder& SetLayers(uint32_t layers);

	ImageBuilder& SetViewType(VkImageViewType type);
	
	ImageBuilder& SetImageType(VkImageType type);

	ImageBuilder& SetInitialLayout(VkImageLayout layout);

	ImageBuilder& SetAspect(VkImageAspectFlags m_Aspect);

	ImageBuilder& SetFilePath(const std::string& path);
	
	void Build(std::unique_ptr<Image>& image, Device* device, CommandPool* commandPool, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
	Image Build(Device* device, CommandPool* commandPool, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);

private:
	friend class Scene;
	friend class SwapchainBuilder;
	void CreateView(Image& image, VkDevice device);
	void Build(Image& image, Device* device, CommandPool* commandPool, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);

	VkImageLayout m_InitialLayout{ VK_IMAGE_LAYOUT_UNDEFINED };
	VkFormat m_Format{ VK_FORMAT_R8G8B8A8_SRGB };
	VkImageTiling m_Tiling{ VK_IMAGE_TILING_OPTIMAL };
	VkImageAspectFlags m_Aspect{ VK_IMAGE_ASPECT_COLOR_BIT };
	VkImageType		m_ImageType{ VK_IMAGE_TYPE_2D };
	VkImageViewType m_ViewType{ VK_IMAGE_VIEW_TYPE_2D };
	VkImageCreateFlags m_Flags{};
	uint32_t m_Width;
	uint32_t m_Height;
	uint32_t m_Layers{ 1 };
	std::string m_FilePath;
};