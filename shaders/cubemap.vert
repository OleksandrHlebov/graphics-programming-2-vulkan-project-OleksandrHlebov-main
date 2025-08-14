#version 450

// Vertex Shader Code
// Hardcoded vertices for 3D unit cube
const vec3 vertexPositions[36] = vec3[]
(
    // +X
    vec3(1, -1, -1), vec3(1, -1,  1), vec3(1,  1,  1),
    vec3(1, -1, -1), vec3(1,  1,  1), vec3(1,  1, -1),

    // -X
    vec3(-1, -1, -1), vec3(-1,  1,  1), vec3(-1, -1,  1),
    vec3(-1, -1, -1), vec3(-1,  1, -1), vec3(-1,  1,  1),

    // +Y
    vec3(-1, 1, -1), vec3(1, 1,  1), vec3(1, 1, -1),
    vec3(-1, 1, -1), vec3(-1, 1,  1), vec3(1, 1,  1),

    // -Y
    vec3(-1, -1, -1), vec3(1, -1, -1), vec3(1, -1,  1),
    vec3(-1, -1, -1), vec3(1, -1,  1), vec3(-1, -1,  1),

    // +Z
    vec3(1, -1, 1), vec3(-1, -1, 1), vec3(-1, 1, 1),
    vec3(1, -1, 1), vec3(-1, 1, 1), vec3(1, 1, 1),

    // -Z
    vec3(-1, -1, -1), vec3(1, 1, -1), vec3(1, -1, -1),
    vec3(-1, -1, -1), vec3(-1, 1, -1), vec3(1, 1, -1)
);

// Output
layout(location = 0) out vec3 fragLocalPosition;

layout(push_constant) uniform constants
{
	mat4 projection;
	mat4 view;
} pushConstants;

// Shader
void main()
{
    vec3 position = vertexPositions[gl_VertexIndex];
    fragLocalPosition = position;
    gl_Position = pushConstants.projection * pushConstants.view
                  * vec4(position.xyz, 1.f);
}
