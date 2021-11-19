#version 330 core

uniform sampler2D sandTex;
uniform sampler2D sandBumpTex;
uniform sampler2D pebbleTex;
uniform sampler2D pebbleBumpTex;
uniform sampler2D grassTex;
uniform sampler2D grassBumpTex;
uniform sampler2D stoneTex;
uniform sampler2D stoneBumpTex;

uniform vec3 cameraPos;
uniform vec4 lightColour;
uniform vec3 lightPos;
uniform float lightRadius;

in Vertex 
{
	vec3 colour;
	vec2 texCoord;
	vec3 normal;
	vec3 tangent;
	vec3 binormal;
	vec3 worldPos;
} IN;

out vec4 fragColour;

void main(void)
{
	vec3 incident = normalize(lightPos - IN.worldPos);
	vec3 viewDir = normalize(cameraPos - IN.worldPos);
	vec3 halfDir = normalize(incident + viewDir);

	mat3 TBN = mat3(normalize(IN.tangent), normalize(IN.binormal), normalize(IN.normal));
	
	vec4 diffuse = texture(sandTex, IN.texCoord);
	vec3 bumpNormal = texture(sandBumpTex, IN.texCoord).rgb;
	
	int sandStart = 200;
	int pebbleStart = 220;
	int grassStart = 260;
	int stoneStart = 320;

	// PAINT SAND
	if (IN.worldPos.y < sandStart)
	{
		diffuse = texture(sandTex, IN.texCoord);
		bumpNormal = texture(sandBumpTex, IN.texCoord).rgb;
	}
	// BLUR BETWEEN SAND AND PEBBLES
	if (IN.worldPos.y >= sandStart && IN.worldPos.y < pebbleStart)
	{
		vec4 topTexture = texture(pebbleTex, IN.texCoord);
		vec4 bottomTexture = texture(sandTex, IN.texCoord);
		vec3 topTextureBump = texture(pebbleBumpTex, IN.texCoord).rgb;
		vec3 bottomTextureBump = texture(sandBumpTex, IN.texCoord).rgb;
		float weighting = (IN.worldPos.y / 20) - 10;
		diffuse = mix(bottomTexture, topTexture, weighting);
		bumpNormal = mix(bottomTextureBump, topTextureBump, weighting);
	}
	// BLUR BETWEEN PEBBLES AND GRASS
	if (IN.worldPos.y >= pebbleStart && IN.worldPos.y < grassStart)
	{
		vec4 topTexture = texture(grassTex, IN.texCoord);
		vec4 bottomTexture = texture(pebbleTex, IN.texCoord);
		vec3 topTextureBump = texture(grassBumpTex, IN.texCoord).rgb;
		vec3 bottomTextureBump = texture(pebbleBumpTex, IN.texCoord).rgb;
		float weighting = ((IN.worldPos.y / 20) / 2) - 5.5;
		diffuse = mix(bottomTexture, topTexture, weighting);
		bumpNormal = mix(bottomTextureBump, topTextureBump, weighting);
	}
	// PAINT GRASS
	if (IN.worldPos.y >= grassStart && IN.worldPos.y < 300)
	{
		diffuse = texture(grassTex, IN.texCoord);
		bumpNormal = texture(grassBumpTex, IN.texCoord).rgb;
	}
	// BLUR BETWEEN GRASS AND STONE
	if (IN.worldPos.y >= 300 && IN.worldPos.y < stoneStart)
	{
		vec4 topTexture = texture(stoneTex, IN.texCoord);
		vec4 bottomTexture = texture(grassTex, IN.texCoord);
		vec3 topTextureBump = texture(stoneBumpTex, IN.texCoord).rgb;
		vec3 bottomTextureBump = texture(grassBumpTex, IN.texCoord).rgb;
		float weighting = (IN.worldPos.y / 20) - 15;
		diffuse = mix(bottomTexture, topTexture, weighting);
		bumpNormal = mix(bottomTextureBump, topTextureBump, weighting);
	}
	// PAINT STONE
	if (IN.worldPos.y >= stoneStart)
	{
		diffuse = texture(stoneTex, IN.texCoord);
		bumpNormal = texture(stoneBumpTex, IN.texCoord).rgb;
	}

	bumpNormal = normalize(TBN * normalize(bumpNormal * 2.0 - 1.0));

	float lambert = max(dot(incident, bumpNormal), 0.0f);
	float distance = length(lightPos - IN.worldPos);
	float attenuation = 1.0f - clamp(distance / lightRadius, 0.0, 1.0);

	float specFactor = clamp(dot(halfDir, bumpNormal), 0.0, 1.0);
	specFactor = pow(specFactor, 60.0);
	vec3 surface = (diffuse.rgb * lightColour.rgb);
	fragColour.rgb = surface * lambert * attenuation;
	fragColour.rgb += (lightColour.rgb * specFactor) * attenuation * 0.33;
	fragColour.rgb += surface * 0.1f;
	fragColour.a = diffuse.a;
}