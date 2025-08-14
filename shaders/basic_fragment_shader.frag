#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 1) in	 vec2 fragTexCoord;

layout(location = 0) out vec4 outColour;

layout(constant_id = 0) const uint TEXTURE_ARRAY_SIZE = 37;

layout(set = 0, binding = 4) uniform sampler samp;

layout(set = 0, binding = 5) uniform texture2D textures[TEXTURE_ARRAY_SIZE];

layout(set = 1, binding = 1) uniform texture2D albedo;
layout(set = 1, binding = 2) uniform texture2D materialProps;

void main()
{
	outColour = texture(sampler2D(albedo, samp), fragTexCoord);
}