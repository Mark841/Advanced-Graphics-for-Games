#version 330 core

uniform vec4 nodeColour;
in Vertex 
{
	smooth vec4 colour;
	vec2 texCoord;
	vec3 normal;
	vec3 tangent;
	vec3 binormal;
	vec3 worldPos;
} IN;

out vec4 fragColour;
void main(void)
{
	fragColour = nodeColour;
}