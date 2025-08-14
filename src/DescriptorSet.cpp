#include "DescriptorSet.h"
#include "Device.h"
#include "Buffer.h"
#include <stdexcept>
#include "../inc/Image.h"
#include <iostream>

DescriptorSet& DescriptorSet::AddWriteDescriptorSet(Buffer* buffer, uint32_t offset, uint32_t binding, uint32_t arrayElement, VkDescriptorType type)
{
	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = *buffer->GetBufferPtr();
	bufferInfo.offset = offset;
	bufferInfo.range = buffer->GetSize(); 

	VkWriteDescriptorSet writeDescriptor{};
	writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptor.dstSet = m_Set;
	writeDescriptor.dstBinding = binding;
	writeDescriptor.dstArrayElement = arrayElement;
	writeDescriptor.descriptorType = type;
	writeDescriptor.descriptorCount = 1;
	writeDescriptor.pImageInfo = nullptr;
	writeDescriptor.pTexelBufferView = nullptr;

	m_WriteDescriptorSets.push_back(std::make_tuple(writeDescriptor, bufferInfo, std::vector<VkDescriptorImageInfo>{}));
	return *this;
}

DescriptorSet& DescriptorSet::AddWriteDescriptorSet(Image* image, VkSampler sampler, uint32_t binding, uint32_t arrayElement)
{
	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = image->GetCurrentLayout();
	imageInfo.imageView = *image->GetFirstViewPtr();
	imageInfo.sampler = sampler;
	 
	VkWriteDescriptorSet writeDescriptor{};
	writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptor.dstSet = m_Set;
	writeDescriptor.dstBinding = binding;
	writeDescriptor.dstArrayElement = arrayElement;
	writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptor.descriptorCount = 1;
	writeDescriptor.pImageInfo = &imageInfo;
	writeDescriptor.pBufferInfo = nullptr;
	writeDescriptor.pTexelBufferView = nullptr;

	m_WriteDescriptorSets.push_back(std::make_tuple(writeDescriptor, VkDescriptorBufferInfo(nullptr), std::vector<VkDescriptorImageInfo>{ std::move(imageInfo) }));
	return *this;
}

DescriptorSet& DescriptorSet::AddWriteDescriptorSet(VkSampler sampler, uint32_t binding, uint32_t arrayElement)
{
	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.imageView = nullptr;
	imageInfo.sampler = sampler;

	VkWriteDescriptorSet writeDescriptor{};
	writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptor.dstSet = m_Set;
	writeDescriptor.dstBinding = binding;
	writeDescriptor.dstArrayElement = arrayElement;
	writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	writeDescriptor.descriptorCount = 1;
	writeDescriptor.pImageInfo = &imageInfo;
	writeDescriptor.pBufferInfo = nullptr;
	writeDescriptor.pTexelBufferView = nullptr;

	m_WriteDescriptorSets.push_back(std::make_tuple(writeDescriptor, VkDescriptorBufferInfo(nullptr), std::vector<VkDescriptorImageInfo>{ std::move(imageInfo) }));
	return *this;
}

DescriptorSet& DescriptorSet::AddWriteDescriptorSet(std::vector<Image>& images, uint32_t binding, uint32_t arrayElement)
{
	std::vector<VkDescriptorImageInfo> imageInfos;
	for (Image& image : images)
	{
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = image.GetCurrentLayout();
		imageInfo.imageView = *image.GetFirstViewPtr();
		imageInfo.sampler = nullptr;
		imageInfos.push_back(imageInfo);
	}
	 
	VkWriteDescriptorSet writeDescriptor{};
	writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;  
	writeDescriptor.dstSet = m_Set;
	writeDescriptor.dstBinding = binding;
	writeDescriptor.dstArrayElement = arrayElement; 
	writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; 
	writeDescriptor.descriptorCount = imageInfos.size(); 
	writeDescriptor.pImageInfo = imageInfos.data();
	writeDescriptor.pBufferInfo = nullptr;
	writeDescriptor.pTexelBufferView = nullptr;

	m_WriteDescriptorSets.push_back(std::make_tuple(writeDescriptor, VkDescriptorBufferInfo(nullptr), std::move(imageInfos)));
	return *this;
}

DescriptorSet& DescriptorSet::AddWriteDescriptorSet(Image* image, uint32_t binding, uint32_t arrayElement)
{
	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = image->GetCurrentLayout();
	imageInfo.imageView = *image->GetFirstViewPtr();
	imageInfo.sampler = nullptr;

	VkWriteDescriptorSet writeDescriptor{};
	writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptor.dstSet = m_Set;
	writeDescriptor.dstBinding = binding;
	writeDescriptor.dstArrayElement = arrayElement;
	writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	writeDescriptor.descriptorCount = 1;
	writeDescriptor.pImageInfo = &imageInfo;
	writeDescriptor.pBufferInfo = nullptr;
	writeDescriptor.pTexelBufferView = nullptr;

	m_WriteDescriptorSets.push_back(std::make_tuple(writeDescriptor, VkDescriptorBufferInfo(nullptr), std::vector<VkDescriptorImageInfo>{ imageInfo }));
	return *this;
}

void DescriptorSet::Update(Device* device)
{
	std::vector<VkWriteDescriptorSet> sets;
	for (int index{}; index < m_WriteDescriptorSets.size(); ++index)
	{
		VkWriteDescriptorSet& set{ std::get<0>(m_WriteDescriptorSets[index]) };
		set.pBufferInfo = &std::get<1>(m_WriteDescriptorSets[index]);
		set.pImageInfo	= std::get<2>(m_WriteDescriptorSets[index]).data();
		sets.push_back(set);
	}
	vkUpdateDescriptorSets(*device->GetDevicePtr(), sets.size(), sets.data(), 0, nullptr);
	m_WriteDescriptorSets.clear();
}

void DescriptorSetBuilder::Build(std::vector<DescriptorSet>& sets, Device* device, uint32_t count, VkDescriptorPool pool, VkDescriptorSetLayout* layouts)
{
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = pool;
	allocInfo.descriptorSetCount = count;
	allocInfo.pSetLayouts = layouts;

	std::vector<VkDescriptorSet> temp(count);
	if (auto result = vkAllocateDescriptorSets(*device->GetDevicePtr(), &allocInfo, temp.data()); result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor sets");
	} 
	for (int index{}; index < count; ++index)
		sets.push_back(DescriptorSet(std::move(temp[index])));
	temp.clear();
}

void DescriptorSetBuilder::Build(std::unique_ptr<DescriptorSet>& set, Device* device, uint32_t count, VkDescriptorPool pool, VkDescriptorSetLayout* layouts)
{
	set.reset(new DescriptorSet(std::move(Build(device, count, pool, layouts))));
}

DescriptorSet DescriptorSetBuilder::Build(Device* device, uint32_t count, VkDescriptorPool pool, VkDescriptorSetLayout* layouts)
{
	DescriptorSet set{};

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = pool;
	allocInfo.descriptorSetCount = count;
	allocInfo.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(*device->GetDevicePtr(), &allocInfo, &set.m_Set) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor sets");
	}
	return set;
}
