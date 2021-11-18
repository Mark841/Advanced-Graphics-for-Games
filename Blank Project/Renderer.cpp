#include "Renderer.h"

#include "../nclgl/Light.h"
#include "../nclgl/Camera.h"
#include "../nclgl/HeightMap.h"
#include "../nclgl/Shader.h"

Renderer::Renderer(Window& parent) : OGLRenderer(parent)
{
	quad = Mesh::GenerateQuad();
	time = 0.0f;

	heightMap = new HeightMap(TEXTUREDIR"noiseTexture256x256.png");
	waterMap = new HeightMap();

	waterTex = SOIL_load_OGL_texture(TEXTUREDIR"water.TGA", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	sandTex = SOIL_load_OGL_texture(TEXTUREDIR"sandTexture.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	sandBump = SOIL_load_OGL_texture(TEXTUREDIR"sandBumpMap.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	pebbleTex = SOIL_load_OGL_texture(TEXTUREDIR"pebbleBeachTexture.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	pebbleBump = SOIL_load_OGL_texture(TEXTUREDIR"pebbleBeachBumpMap.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	grassTex = SOIL_load_OGL_texture(TEXTUREDIR"grassTexture.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	grassBump = SOIL_load_OGL_texture(TEXTUREDIR"grassBumpMap.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	stoneTex = SOIL_load_OGL_texture(TEXTUREDIR"stonePathTexture.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	stoneBump = SOIL_load_OGL_texture(TEXTUREDIR"stonePathBumpMap.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	cubeMap = SOIL_load_OGL_cubemap(TEXTUREDIR"tropical_right.jpg", TEXTUREDIR"tropical_left.jpg", TEXTUREDIR"tropical_top.jpg", TEXTUREDIR"tropical_bottom.jpg", TEXTUREDIR"tropical_front.jpg", TEXTUREDIR"tropical_back.jpg", SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	if (!sandTex || !sandBump || !grassTex || !grassBump || !stoneTex || !stoneBump || !cubeMap || !waterTex)
		return;

	SetTextureRepeating(sandTex, true);
	SetTextureRepeating(sandBump, true);
	SetTextureRepeating(grassTex, true);
	SetTextureRepeating(grassBump, true);
	SetTextureRepeating(pebbleTex, true);
	SetTextureRepeating(pebbleBump, true);
	SetTextureRepeating(stoneTex, true);
	SetTextureRepeating(stoneBump, true);
	SetTextureRepeating(waterTex, true);

	waterShader = new Shader("WaterVertex.glsl", "WaterFragment.glsl");
	skyboxShader = new Shader("SkyboxVertex.glsl", "SkyboxFragment.glsl");
	islandShader = new Shader("IslandVertex.glsl", "IslandFragment.glsl");

	if (!waterShader->LoadSuccess() || !skyboxShader->LoadSuccess() || !islandShader->LoadSuccess())
		return;

	Vector3 heightMapSize = heightMap->GetHeightMapSize();
	camera = new Camera(-30.0f, 0.0f, 0.0f, heightMapSize * Vector3(0.5f, 5.0f, 1.0f));
	light = new Light(heightMapSize * Vector3(0.5f, 5.0f, 0.5f), Vector4(1, 1, 1, 1), heightMapSize.x*5);

	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	waterRotate = 0.0f;
	waterCycle = 0.0f;
	init = true;
}
Renderer::~Renderer(void)
{
	delete camera;
	delete heightMap;
	delete waterMap;
	delete quad;
	delete waterShader;
	delete skyboxShader;
	delete islandShader;
	delete light;
}

void Renderer::UpdateScene(float dt)
{
	camera->UpdateCamera(dt);
	time += dt;
	viewMatrix = camera->BuildViewMatrix();
	waterRotate += dt * 0.5f; // 0.5 degrees a second
	waterCycle += dt * 0.0675f; // 2.5 units a second
}
void Renderer::RenderScene()
{
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	// ordering of these sub calls is important
	DrawSkybox();
	DrawHeightMap();
	DrawWater();
}

void Renderer::DrawSkybox()
{
	glDepthMask(GL_FALSE);

	BindShader(skyboxShader);
	UpdateShaderMatrices();

	quad->Draw();

	glDepthMask(GL_TRUE);
}
void Renderer::DrawHeightMap()
{
	BindShader(islandShader);
	SetShaderLight(*light);
	glUniform3fv(glGetUniformLocation(islandShader->GetProgram(), "cameraPos"), 1, (float*)& camera->GetPosition());

	glUniform1i(glGetUniformLocation(islandShader->GetProgram(), "sandTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, sandTex);

	glUniform1i(glGetUniformLocation(islandShader->GetProgram(), "sandBumpTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, sandBump);

	glUniform1i(glGetUniformLocation(islandShader->GetProgram(), "grassTex"), 3);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, grassTex);

	glUniform1i(glGetUniformLocation(islandShader->GetProgram(), "grassBumpTex"), 4);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, grassBump);

	glUniform1i(glGetUniformLocation(islandShader->GetProgram(), "stoneTex"), 5);
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, stoneTex);

	glUniform1i(glGetUniformLocation(islandShader->GetProgram(), "stoneBumpTex"), 6);
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, stoneBump);

	glUniform1i(glGetUniformLocation(islandShader->GetProgram(), "pebbleTex"), 7);
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, pebbleTex);

	glUniform1i(glGetUniformLocation(islandShader->GetProgram(), "pebbleBumpTex"), 8);
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, pebbleBump);

	modelMatrix.ToIdentity();
	textureMatrix.ToIdentity();

	UpdateShaderMatrices();

	heightMap->Draw();
}
void Renderer::DrawWater()
{
	BindShader(waterShader);

	glUniform3fv(glGetUniformLocation(waterShader->GetProgram(), "cameraPos"), 1, (float*)& camera->GetPosition());

	glUniform1f(glGetUniformLocation(waterShader->GetProgram(), "time"), time);
	glUniform2f(glGetUniformLocation(waterShader->GetProgram(), "gerstnerWaves[0].direction"), 1.0f, 0.0f);
	glUniform1f(glGetUniformLocation(waterShader->GetProgram(), "gerstnerWaves[0].amplitude"), 15.0);
	glUniform1f(glGetUniformLocation(waterShader->GetProgram(), "gerstnerWaves[0].steepness"), 0.5);
	glUniform1f(glGetUniformLocation(waterShader->GetProgram(), "gerstnerWaves[0].frequency"), 0.005);
	glUniform1f(glGetUniformLocation(waterShader->GetProgram(), "gerstnerWaves[0].speed"), 0.5);

	glUniform2f(glGetUniformLocation(waterShader->GetProgram(), "gerstnerWaves[1].direction"), 1.0f, 1.0f);
	glUniform1f(glGetUniformLocation(waterShader->GetProgram(), "gerstnerWaves[1].amplitude"), 20.0);
	glUniform1f(glGetUniformLocation(waterShader->GetProgram(), "gerstnerWaves[1].steepness"), 15.0);
	glUniform1f(glGetUniformLocation(waterShader->GetProgram(), "gerstnerWaves[1].frequency"), 0.001);
	glUniform1f(glGetUniformLocation(waterShader->GetProgram(), "gerstnerWaves[1].speed"), 1.0);
	glUniform1i(glGetUniformLocation(waterShader->GetProgram(), "gerstnerWavesLength"), 1);

	glUniform1i(glGetUniformLocation(waterShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(waterShader->GetProgram(), "cubeTex"), 2);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, waterTex);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);

	modelMatrix.ToIdentity();
	textureMatrix.ToIdentity();

	UpdateShaderMatrices();
	waterMap->Draw();
}