#version 450
// Pixel Shader Code
// Input
layout(location = 0) in vec3 fragLocalPosition;
layout(location = 0) out vec4 outColor;

// Descriptor Bindings
layout(set = 0, binding = 0) uniform sampler textureSampler;
layout(set = 0, binding = 1) uniform textureCube environmentImage;

const float PI = 3.14159265358979323846;

// Shader
void main()
{
	const vec3 worldUp	= vec3(0.0, 0.0, 1.0);
	const vec3 normal	= normalize(fragLocalPosition);
    const vec3 right	= normalize(cross(worldUp, normal));
    const vec3 up		= normalize(cross(normal, right));

	vec3 irradiance = vec3(.0);

	float sampleDelta = 0.025;
	float nrSamples = 0.0; 
	for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
	{
	    for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
	    {
	        // spherical to cartesian (in tangent space)
	        vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
	        // tangent space to world
	        vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal; 
	
	        irradiance += texture(samplerCube(environmentImage, textureSampler), normalize(sampleVec)).rgb * cos(theta) * sin(theta);
	        nrSamples++;
	    }
	}
	irradiance = PI * irradiance * (1.0 / float(nrSamples));

	outColor = vec4(clamp(irradiance, .0, 1.), 1.);
}