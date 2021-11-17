#version 330 core

#define M_PI 3.1415926535897932384626433832795

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;
uniform mat4 textureMatrix;

uniform float time;
uniform int gerstnerWavesLength;
uniform struct GerstnerWave
{
	vec2 direction;
	float amplitude;
	float steepness;
	float frequency;
	float speed;
} gerstnerWaves[5];

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

vec3 gerstner_wave_position(vec2 position, float time)
{
	vec3 wavePosition = vec3(position.x, position.y, 0);
	for (int i = 0; i < gerstnerWavesLength; ++i)
	{
		//float phase = time * gerstnerWaves[i].speed;
		//float theta = dot(position.xy, gerstnerWaves[i].direction) * gerstnerWaves[i].frequency + phase;
		//float height = gerstnerWaves[i].amplitude * sin(theta);

		//wavePosition.z += height;

		//float width = gerstnerWaves[i].steepness * gerstnerWaves[i].amplitude * cos(theta);

		//wavePosition.x += gerstnerWaves[i].direction.x * width;
		//wavePosition.y += gerstnerWaves[i].direction.y * width;

		wavePosition.z += ((gerstnerWaves[i].steepness * gerstnerWaves[i].amplitude) * cos(gerstnerWaves[i].steepness * dot(gerstnerWaves[i].direction, position.xy) + 0.5 * time));
	}
	return wavePosition;
}

vec3 gerstner_wave_normal(vec3 position, float time)
{
	vec3 waveNormal = vec3(0.0, 1.0, 0.0);
	for (int i = 0; i < gerstnerWavesLength; ++i)
	{
		float proj = dot(position.xz, gerstnerWaves[i].direction); 
		float phase = time * gerstnerWaves[i].speed;
		float psi = proj * gerstnerWaves[i].frequency + phase;
		float Af = gerstnerWaves[i].amplitude * gerstnerWaves[i].frequency;
		float alpha = Af * sin(psi);

		waveNormal.y -= gerstnerWaves[i].steepness * alpha;

		float x = gerstnerWaves[i].direction.x; 
		float y = gerstnerWaves[i].direction.y;
		float omega = Af * cos(psi);

		waveNormal.x -= x * omega;
		waveNormal.z -= y * omega;
	}
	return waveNormal;
}

void main(void)
{
	mat3 normalMatrix = transpose(inverse(mat3(modelMatrix)));

	OUT.texCoord = (textureMatrix * vec4(texCoord, 0.0, 1.0)).xy;
	OUT.normal = normalize(normalMatrix * normalize(normal));

	// float amplitude = 0.5f;
	// float waveSpeed = 0.25f;
	// float waveLength = 5.0f;
	// float waterDepth = 0.25f;
	
	//float offset = amplitude * (sin((time * ((2 *  M_PI)* waveSpeed)) + waveLength) * cos((time * ((2 *  M_PI) * waveSpeed)) + waveLength) * (position.x * position.y));
	
	// float offset = amplitude * (position.x + sin((time * ((2 *  M_PI)* waveSpeed)) + wavelength))  * (position.y - cos((time * ((2 *  M_PI) * waveSpeed)) + wavelength));
		
	// vec4 worldPos = (modelMatrix * vec4(position.x, position.y, position.z + offset, 1));
	// vec4 worldPos = (modelMatrix * vec4(position, 1));

	vec4 worldPos = (modelMatrix * vec4(gerstner_wave_position(position.xy, time), 1));
	//normal = gerstner_wave_normal(worldPos.xyz, time);
	
	//OUT.normal = normalize(normalMatrix * normalize(normal));
	OUT.worldPos = worldPos.xyz;
	gl_Position = (projMatrix * viewMatrix) * worldPos;
}