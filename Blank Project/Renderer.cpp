#include "Renderer.h"

#include "../nclgl/Light.h"
#include "../nclgl/Camera.h"
#include "../nclgl/HeightMap.h"
#include "../nclgl/Shader.h"

Renderer::Renderer(Window& parent) : OGLRenderer(parent)
{
	quad = Mesh::GenerateQuad();
	time = 0.0f;

	heightMap = new HeightMap(TEXTUREDIR"noise.png");

	waterTex = SOIL_load_OGL_texture(TEXTUREDIR"water.TGA", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	earthTex = SOIL_load_OGL_texture(TEXTUREDIR"Barren Reds.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	earthBump = SOIL_load_OGL_texture(TEXTUREDIR"Barren RedsDOT3.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	cubeMap = SOIL_load_OGL_cubemap(TEXTUREDIR"tropical_right.jpg", TEXTUREDIR"tropical_left.jpg", TEXTUREDIR"tropical_top.jpg", TEXTUREDIR"tropical_bottom.jpg", TEXTUREDIR"tropical_front.jpg", TEXTUREDIR"tropical_back.jpg", SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	if (!earthTex || !earthBump || !cubeMap || !waterTex)
		return;

	SetTextureRepeating(earthTex, true);
	SetTextureRepeating(earthBump, true);
	SetTextureRepeating(waterTex, true);

	reflectShader = new Shader("ReflectVertex.glsl", "ReflectFragment.glsl");
	skyboxShader = new Shader("SkyboxVertex.glsl", "SkyboxFragment.glsl");
	lightShader = new Shader("PerPixelVertex.glsl", "PerPixelFragment.glsl");

	if (!reflectShader->LoadSuccess() || !skyboxShader->LoadSuccess() || !lightShader->LoadSuccess())
		return;

	Vector3 heightMapSize = heightMap->GetHeightMapSize();
	camera = new Camera(-45.0f, 0.0f, 0.0f, heightMapSize * Vector3(0.5f, 5.0f, 0.5f));
	light = new Light(heightMapSize * Vector3(0.5f, 1.5f, 0.5f), Vector4(1, 1, 1, 1), heightMapSize.x);

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
	delete quad;
	delete reflectShader;
	delete skyboxShader;
	delete lightShader;
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
	BindShader(lightShader);
	SetShaderLight(*light);
	glUniform3fv(glGetUniformLocation(lightShader->GetProgram(), "cameraPos"), 1, (float*)& camera->GetPosition());

	glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, earthTex);

	glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "bumpTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, earthBump);

	modelMatrix.ToIdentity();
	textureMatrix.ToIdentity();

	UpdateShaderMatrices();

	heightMap->Draw();
}
void Renderer::DrawWater()
{
	BindShader(reflectShader);

	glUniform3fv(glGetUniformLocation(reflectShader->GetProgram(), "cameraPos"), 1, (float*)& camera->GetPosition());

	glUniform1f(glGetUniformLocation(reflectShader->GetProgram(), "time"), time);
	glUniform2f(glGetUniformLocation(reflectShader->GetProgram(), "gerstnerWaves[0].direction"), 1.0f, 0.0f);
	glUniform1f(glGetUniformLocation(reflectShader->GetProgram(), "gerstnerWaves[0].amplitude"), 1.0);
	glUniform1f(glGetUniformLocation(reflectShader->GetProgram(), "gerstnerWaves[0].steepness"), 0.5);
	glUniform1f(glGetUniformLocation(reflectShader->GetProgram(), "gerstnerWaves[0].frequency"), 1.0);
	glUniform1f(glGetUniformLocation(reflectShader->GetProgram(), "gerstnerWaves[0].speed"), 1.0);

	glUniform2f(glGetUniformLocation(reflectShader->GetProgram(), "gerstnerWaves[1].direction"), 1.0f, 0.0f);
	glUniform1f(glGetUniformLocation(reflectShader->GetProgram(), "gerstnerWaves[1].amplitude"), 115.5);
	glUniform1f(glGetUniformLocation(reflectShader->GetProgram(), "gerstnerWaves[1].steepness"), 0.75);
	glUniform1f(glGetUniformLocation(reflectShader->GetProgram(), "gerstnerWaves[1].frequency"), 1.5);
	glUniform1f(glGetUniformLocation(reflectShader->GetProgram(), "gerstnerWaves[1].speed"), 15.0);
	glUniform1ui(glGetUniformLocation(reflectShader->GetProgram(), "gerstnerWavesLength"), 2);

	glUniform1i(glGetUniformLocation(reflectShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(reflectShader->GetProgram(), "cubeTex"), 2);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, waterTex);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);

	Vector3 hSize = heightMap->GetHeightMapSize();

	modelMatrix = Matrix4::Translation(hSize * 0.5f) * Matrix4::Scale(hSize * 0.5f) * Matrix4::Rotation(90, Vector3(1, 0, 0));
	textureMatrix = Matrix4::Translation(Vector3(waterCycle, 0.0f, waterCycle)) * Matrix4::Scale(Vector3(10, 10, 10)) * Matrix4::Rotation(waterRotate, Vector3(0, 0, 1));

	UpdateShaderMatrices();
	quad->Draw();
}