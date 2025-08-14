#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "DeletionQueue.h"

class Device;

class ShaderStage final
{
public:
	ShaderStage() = delete;
	ShaderStage(Device* device, const std::vector<char>& code, VkShaderStageFlagBits stageFlag);
	~ShaderStage();	

	VkShaderModule GetModule() { return m_ShaderModule; }

	// supports only data of the same size
	void AddSpecialization(uint32_t sizes, uint32_t count, void* data);

	void Destroy(Device* device);

	VkPipelineShaderStageCreateInfo& GetInfo() { return m_Info; }

private:
	VkShaderModule m_ShaderModule;
	VkPipelineShaderStageCreateInfo m_Info{};
	std::vector<VkSpecializationMapEntry> m_MapEntries{};
	VkSpecializationInfo m_SpecializationInfo{};
};