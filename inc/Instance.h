#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include "Globals.h"

class Instance final
{
public:
	~Instance() = default;
	
	Instance(const Instance&) 					= delete;
	Instance(Instance&&) noexcept 				= delete;
	Instance& operator=(const Instance&) 	 	= delete;
	Instance& operator=(Instance&&) noexcept 	= delete;

	VkInstance* GetInstancePtr() { return &m_Instance; }
	const std::vector<const char*>& GetValidationLayers() { return m_ValidationLayers; }

private:
	friend class InstanceBuilder;
	Instance() = default;

	VkInstance m_Instance;
	std::vector<const char*> m_ValidationLayers{};
};

class InstanceBuilder final
{
public:
	InstanceBuilder() = default;
	~InstanceBuilder() = default;
	
	InstanceBuilder(const InstanceBuilder&) 				= delete;
	InstanceBuilder(InstanceBuilder&&) noexcept 			= delete;
	InstanceBuilder& operator=(const InstanceBuilder&) 	 	= delete;
	InstanceBuilder& operator=(InstanceBuilder&&) noexcept 	= delete;

	InstanceBuilder& AddDebugMessenger(const VkDebugUtilsMessengerCreateInfoEXT& createInfo) 
	{ 
		m_DebugMsngrCreateInfo = createInfo; 
		return *this; 
	}
	InstanceBuilder& AddRequiredExtension(const char* extension) 
	{ 
		m_RequiredExtensions.push_back(extension); 
		return *this; 
	}
	InstanceBuilder& AddMultipleRequiredExtensions(const std::vector<const char*>& extensions)
	{
		m_RequiredExtensions.insert(m_RequiredExtensions.end(), extensions.begin(), extensions.end());
		return *this;
	}
	InstanceBuilder& AddValidationLayer(const char* layer)
	{ 
		m_ValidationLayers.push_back(layer); 
		return *this; 
	}
	InstanceBuilder& AddMultipleValidationLayers(const std::vector<const char*>& layers) 
	{ 
		m_ValidationLayers.insert(m_ValidationLayers.end(), layers.begin(), layers.end());
		return *this;
	}

	InstanceBuilder& SetEngineName(const std::string& name) 
	{ 
		m_EngineName = name; 
		return *this;
	}

	InstanceBuilder& SetAppName(const std::string& name)
	{
		m_AppName = name;
		return *this;
	}

	InstanceBuilder& SetEngineVersion(uint32_t version)
	{
		m_EngineVersion = version;
		return *this;
	}

	InstanceBuilder& SetAppVersion(uint32_t version)
	{
		m_AppVersion = version;
		return *this;
	}

	void Build(std::unique_ptr<Instance>& instance);

private:
	bool CheckValidationLayerSupport();

	std::string m_EngineName{ "no engine" };
	uint32_t m_EngineVersion{ VK_MAKE_VERSION(1, 0, 0) };
	std::string m_AppName	{ "Vulkan" };
	uint32_t m_AppVersion	{ VK_MAKE_VERSION(1, 0, 0) };
	// only support vulkan 1.3
	const uint32_t apiVersion{ VK_API_VERSION_1_3 };
	
	std::vector<const char*> m_ValidationLayers{};
	std::vector<const char*> m_RequiredExtensions{};
	std::optional<VkDebugUtilsMessengerCreateInfoEXT> m_DebugMsngrCreateInfo;
};