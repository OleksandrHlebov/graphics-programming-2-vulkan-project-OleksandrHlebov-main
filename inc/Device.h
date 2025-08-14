#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <cassert>
#include <memory>
#include <optional>
#include "Globals.h"

class Device final
{
public:
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool IsComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
	};

	~Device() = default;

	Device(const Device &) = delete;

	Device(Device &&) noexcept = delete;

	Device &operator=(const Device &) = delete;

	Device &operator=(Device &&) noexcept = delete;

	VkDevice *        GetDevicePtr() { return &m_Device; }
	VkPhysicalDevice *GetPhysicalDevicePtr() { return &m_PhysicalDevice; }

	VkQueue *GetGraphicsQueuePtr() { return &m_GraphicsQueue; }
	VkQueue *GetPresentQueuePtr() { return &m_PresentQueue; }

	VkFormat FindSupportedFormats
	(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	QueueFamilyIndices FindQueueFamilies(VkSurfaceKHR surface);

	void SetObjectName(VkObjectType type, uint64_t handle, const char *name);

	void Destroy();

private:
	friend class DeviceBuilder;

	Device() = default;

	static QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physDevice, VkSurfaceKHR surface);

	VkDevice         m_Device;
	VkPhysicalDevice m_PhysicalDevice{ VK_NULL_HANDLE };
	VkQueue          m_GraphicsQueue{};
	VkQueue          m_PresentQueue{};

	PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT{ nullptr };
};

class Instance;

class DeviceBuilder final
{
public:
	DeviceBuilder() = default;

	~DeviceBuilder() = default;

	DeviceBuilder(const DeviceBuilder &) = delete;

	DeviceBuilder(DeviceBuilder &&) noexcept = delete;

	DeviceBuilder &operator=(const DeviceBuilder &) = delete;

	DeviceBuilder &operator=(DeviceBuilder &&) noexcept = delete;

	DeviceBuilder &AddExtension(const char *extension)
	{
		m_Extensions.emplace_back(extension);
		return *this;
	}

	DeviceBuilder &AddMultipleExtensions(const std::vector<const char *> &extensions)
	{
		m_Extensions.insert(m_Extensions.end(), extensions.begin(), extensions.end());
		return *this;
	}

	void Build(std::unique_ptr<Device> &device, Instance *instance, VkSurfaceKHR surface);

	// sets pnext and stype when build is triggered
	DeviceBuilder &SetEnabledFeatures(const VkPhysicalDeviceCustomBorderColorFeaturesEXT &features);

	// sets pnext and stype when build is triggered
	DeviceBuilder &SetEnabledFeatures(const VkPhysicalDeviceVulkan13Features &features);

	// sets pnext and stype when build is triggered
	DeviceBuilder &SetEnabledFeatures(const VkPhysicalDeviceVulkan12Features &features);

	// sets pnext and stype when build is triggered
	DeviceBuilder &SetEnabledFeatures(const VkPhysicalDeviceVulkan11Features &features);

	// sets pnext and stype when build is triggered
	DeviceBuilder &SetEnabledFeatures(const VkPhysicalDeviceFeatures2 &features);

	// sets pnext and stype when build is triggered
	DeviceBuilder &SetEnabledFeatures(const VkPhysicalDeviceFeatures &features);

	DeviceBuilder &PreferDedicatedGPU()
	{
		m_PreferdGPU = true;
		return *this;
	}

private:
	void PickPhysicalDevice(VkPhysicalDevice *physDevice, VkInstance instance, VkSurfaceKHR surface);

	int RateDeviceSuitability(VkPhysicalDevice device, VkSurfaceKHR surface);

	bool CheckDeviceExtensionSupport(VkPhysicalDevice device);

	bool CheckFeatureSupport(VkPhysicalDevice device);

	void CreateLogicalDevice();

	template<typename FeatureType>
	bool CompareFeatures(FeatureType *available, FeatureType *enabled)
	{
		// the layout of vulkan structs is consistent and never changes
		// hence we can reinterpret cast it and increment pointer value
		// to check feature availability
		const VkBool32 *availablePtr{ reinterpret_cast<VkBool32 *>(available) };
		const VkBool32 *enabledPtr{ reinterpret_cast<VkBool32 *>(enabled) };

		// systems guarantee 8 byte memory alignment
		// add padding for VkStructureType as it is only 4 bytes
		const uint32_t padding = 8 - sizeof(VkStructureType);
		// first members of features structs are always sType and pNext
		// we need an offset to start comparing only booleans against each other
		const uint32_t offset = (sizeof(VkStructureType) + padding + sizeof(void *)) / sizeof(VkBool32);

		for (uint32_t index{ offset }; index < sizeof(FeatureType) / sizeof(VkBool32); ++index)
			// if not available, but enabled
			if (!availablePtr[index] && enabledPtr[index])
				return false;

		return true;
	}

	VkPhysicalDeviceCustomBorderColorFeaturesEXT m_CustomBorderColorFeatures{};
	VkPhysicalDeviceVulkan13Features             m_DeviceFeatures13{};
	VkPhysicalDeviceVulkan12Features             m_DeviceFeatures12{};
	VkPhysicalDeviceVulkan11Features             m_DeviceFeatures11{};
	VkPhysicalDeviceFeatures2                    m_DeviceFeatures{};

	std::vector<const char *> m_Extensions{};

	bool m_PreferdGPU{};
};

// specialization for debugging and initial implementation
#ifndef NDEBUG
template<>
inline bool DeviceBuilder::CompareFeatures
(VkPhysicalDeviceCustomBorderColorFeaturesEXT *available, VkPhysicalDeviceCustomBorderColorFeaturesEXT *enabled)
{
	// the layout of vulkan structs is consistent and never changes
	// hence we can reinterpret cast it and increment pointer value
	// to check feature availability
	const VkBool32 *availablePtr{ reinterpret_cast<VkBool32 *>(available) };
	const VkBool32 *enabledPtr{ reinterpret_cast<VkBool32 *>(enabled) };

	// debug related to check sizes
	const size_t vkstructure = sizeof(VkStructureType);
	const size_t voidptr     = sizeof(void *);
	const size_t vkbool      = sizeof(VkBool32);

	// systems guarantee 8 byte memory alignment
	// add padding for VkStructureType as it is only 4 bytes
	const uint32_t padding = 8 - sizeof(VkStructureType);
	// first members of features structs are always sType and pNext
	// we need an offset to start comparing only booleans against each other
	const uint32_t offset = (sizeof(VkStructureType) + padding + sizeof(void *)) / sizeof(VkBool32);

	// assert to verify that we are indeed looking at the first boolean to check
	assert(&availablePtr[offset] == &available->customBorderColors);

	for (uint32_t index{ offset }; index < sizeof(VkPhysicalDeviceCustomBorderColorFeaturesEXT) / sizeof(VkBool32); ++
		 index)
		// if not available, but enabled
		if (!availablePtr[index] && enabledPtr[index])
			return false;

	return true;
}
#endif

// exceptional case
template<>
inline bool DeviceBuilder::CompareFeatures(VkPhysicalDeviceFeatures2 *available, VkPhysicalDeviceFeatures2 *enabled)
{
	// the layout of vulkan structs is consistent and never changes
	// hence we can reinterpret cast it and increment pointer value
	// to check feature availability
	const VkBool32 *availablePtr{ reinterpret_cast<VkBool32 *>(&available->features) };
	const VkBool32 *enabledPtr{ reinterpret_cast<VkBool32 *>(&enabled->features) };

	// VkPhysicalDeviceFeatures contains only booleans
	// hence no offset
	const uint32_t offset = 0;

	// assert to verify that we are indeed looking at the first boolean to check
	assert(&availablePtr[offset] == &available->features.robustBufferAccess);

	for (uint32_t index{ offset }; index < sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32); ++index)
		// if not available, but enabled
		if (!availablePtr[index] && enabledPtr[index])
			return false;

	return true;
}
