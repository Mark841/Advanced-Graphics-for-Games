#version 330 core

#define M_PI 3.1415926535897932384626433832795

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;
uniform mat4 textureMatrix;

uniform float time;

in vec3 position;
in vec3 normal;
in vec2 texCoord;

out Vertex
{
	vec3 colour;
	vec2 texCoord;
	vec3 normal;
	vec3 worldPos;
} OUT;

void main(void)
{
	mat3 normalMatrix = transpose(inverse(mat3(modelMatrix)));

	OUT.texCoord = (textureMatrix * vec4(texCoord, 0.0, 1.0)).xy;
	OUT.normal = normalize(normalMatrix * normalize(normal));

	float amplitude = 0.5f;
	float waveSpeed = 0.5f;
	float wavelength = 0.25f;
	float waterDepth = 0.5f;
	float km = (2 * M_PI) / wavelength;
	float offset = amplitude * (sin((time * ((2 *  M_PI)* waveSpeed)) + wavelength) * cos((time * ((2 *  M_PI) * waveSpeed)) + wavelength) * (position.x * position.y));
	// float offset = amplitude * (position.x + sin((time * ((2 *  M_PI)* waveSpeed)) + wavelength))  * (position.y - cos((time * ((2 *  M_PI) * waveSpeed)) + wavelength));
	vec4 worldPos = (modelMatrix * vec4(position.x, position.y, position.z + offset, 1));

	OUT.worldPos = worldPos.xyz;

	gl_Position = (projMatrix * viewMatrix) * worldPos;
}