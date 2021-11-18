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

vec3 gerstner_wave_position(vec3 position, float time)
{
	vec3 wavePosition = vec3(position.x, 100, position.z);
	for (int i = 0; i < gerstnerWavesLength; ++i)
	{	
		float phase = time * gerstnerWaves[i].speed;
		float theta = (dot(position.xz, gerstnerWaves[i].direction) * gerstnerWaves[i].frequency) + phase;

		wavePosition.y += gerstnerWaves[i].amplitude * sin(theta);

		float width = gerstnerWaves[i].steepness * gerstnerWaves[i].amplitude * cos(theta);

		wavePosition.x += (gerstnerWaves[i].direction.x * width);
		wavePosition.z += (gerstnerWaves[i].direction.y * width);

		if (position.x == 1.0 || position.x == -1.0)
		{
			wavePosition.x = position.x;
		}
		if (position.z == 1.0 || position.z == -1.0)
		{
			wavePosition.z = position.z;
		}
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
	//OUT.normal = normalize(normalMatrix * normalize(normal));
		
	vec4 worldPos = (modelMatrix * vec4(gerstner_wave_position(position, time), 1));
	OUT.normal = gerstner_wave_normal(worldPos.xyz, time);
	
	OUT.worldPos = worldPos.xyz;
	gl_Position = (projMatrix * viewMatrix) * worldPos;
}