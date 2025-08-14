#version 450

layout(set = 1, binding = 0) uniform ModelViewProjection
{
	mat4 model;
	mat4 view;
	mat4 projection;
} mvp;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColour;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 normal;
layout(location = 4) in vec3 tangent;
layout(location = 5) in vec3 bitangent;

layout(location = 2) out vec3 fragColour;
layout(location = 1) out vec2 fragTexCoord;

void main()
{
	gl_Position = mvp.projection * mvp.view * mvp.model * vec4(inPosition, 1.);
	fragColour = inColour;
	fragTexCoord = inTexCoord;
}