#pragma once

#include "../nclgl/OGLRenderer.h"

class HeightMap;
class Camera;
class Shader;

class Renderer : public OGLRenderer
{
public:
	Renderer(Window& parent);
	~Renderer(void);

	void RenderScene() override;
	void UpdateScene(float dt) override;

protected:
	void DrawHeightMap();
	void DrawWater();
	void DrawSkybox();

	Shader* islandShader;
	Shader* waterShader;
	Shader* skyboxShader;

	HeightMap* heightMap;
	HeightMap* waterMap;
	Mesh* quad;

	Light* light;
	Camera* camera;

	GLuint cubeMap;
	GLuint waterTex;
	GLuint sandTex;
	GLuint sandBump;
	GLuint grassTex;
	GLuint grassBump;
	GLuint pebbleTex;
	GLuint pebbleBump;
	GLuint stoneTex;
	GLuint stoneBump;

	float waterRotate;
	float waterCycle;
	float time;
};