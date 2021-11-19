#version 330 core

uniform sampler2D waterTex;
uniform samplerCube skyboxTex;

uniform vec3 cameraPos;

in Vertex
{
	vec4 colour;
	vec2 texCoord;
	vec3 normal;
	vec3 worldPos;
} IN;

out vec4 fragColour;

void main(void)
{
	vec4 diffuse = texture(waterTex, IN.texCoord);
	vec3 viewDir = normalize(cameraPos - IN.worldPos);

	vec3 reflectDir = reflect(-viewDir, normalize(IN.normal));
	vec4 reflectTex = texture(skyboxTex, reflectDir);

	fragColour = reflectTex + (diffuse * 0.25f);
	fragColour.a = 0.65f;
}