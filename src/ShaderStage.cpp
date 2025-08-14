#include "ShaderStage.h"
#include "Device.h"
#include <stdexcept>

ShaderStage::ShaderStage(Device* device, const std::vector<char>& code, VkShaderStageFlagBits stageFlag)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	if (vkCreateShaderModule(*device->GetDevicePtr(), &createInfo, nullptr, &m_ShaderModule) != VK_SUCCESS)
		throw std::runtime_error("failed to create shader module");

	m_Info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	m_Info.flags = 0;
	m_Info.stage = stageFlag;
	m_Info.module = m_ShaderModule;
	m_Info.pName = "main";
}

ShaderStage::~ShaderStage() = default;

void ShaderStage::AddSpecialization(uint32_t size, uint32_t count, void* data)
{
	for (int index{}; index < count; ++index)
	{
		m_MapEntries.emplace_back(index, index * size, size);
	}

	m_SpecializationInfo.mapEntryCount = count;
	m_SpecializationInfo.pMapEntries = m_MapEntries.data();
	m_SpecializationInfo.pData = data;
	m_SpecializationInfo.dataSize = size * count;
	m_Info.pSpecializationInfo = (m_SpecializationInfo.mapEntryCount > 0) ? &m_SpecializationInfo : nullptr;
}

void ShaderStage::Destroy(Device* device)
{
	vkDestroyShaderModule(*device->GetDevicePtr(), m_ShaderModule, nullptr);
}
