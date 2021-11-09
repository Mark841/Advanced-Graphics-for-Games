#version 330 core

uniform sampler2D diffuseTex;

in Vertex
{
	vec2 texCoord;
	// 1 - smooth vec4 colour;
	// 2 - vec4 jointWeights;
} IN;

out vec4 fragColour;

void main(void)
{
	// 1 - fragColour = texture(diffuseTex, IN.texCoord) * IN.colour;
	// 2 - if(IN.jointWeights.yz == vec2(0,0)){
    // 2 -     fragColour = texture(diffuseTex, IN.texCoord) * IN.jointWeights;
    // 2 - }
	// 2 - else {
	// 2 - 	fragColour = texture(diffuseTex, IN.texCoord) * vec4(0, 1, 0, 0);
	// 2 - }
	fragColour = texture(diffuseTex, IN.texCoord);
}

// The commented out "1 -" sections were used to make shading on the terrain mesh
// The commented out "2 -" sections were used to make colouring on the skeletal animation