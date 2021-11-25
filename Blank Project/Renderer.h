#pragma once

#include "../nclgl/OGLRenderer.h"
#include "../nclgl/SceneNode.h"

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
	void ToggleDayNight() { activeDayNight = !activeDayNight; }
	void SetWaves(int w) { waves = w; }

	bool followWaypoints;

protected:
	bool InitialiseTextures();
	bool InitialiseShaders();
	bool InitialiseBuffers();

	void MoveCamera();
	void UpdateMiniMapCamera();
	bool CheckCameraDistance(Vector3 distance, float speed);

	void PresentScene();
	void DrawPostProcess();
	void DrawScene();
	void DrawNode(SceneNode* n);

	void DayNightCycle(float dt);
	Vector3 Rotate(float angle, Vector3 axis, Vector3 position);
	void DrawSkybox();

	void passInfoToShader(Shader* shader, Matrix4 model, SceneNode* n);

	HeightMap* heightMapMesh;
	HeightMap* waterMapMesh;
	SceneNode* root;

	Shader* sceneShader;
	Shader* islandShader;
	Shader* waterShader;
	Shader* skyboxShader;
	Shader* treeShader;
	Shader* skeletonShader;
	Shader* postProcessShader;

	GLuint bufferFBO;
	GLuint mapFBO;
	GLuint processFBO;
	GLuint bufferColourTex[2];
	GLuint bufferDepthTex;
	GLuint mapDepthTex;
	GLuint mapColourTex[2];

	Mesh* quad;
	Matrix4 mapViewMatrix;

	Light* sun;
	Camera* camera;
	Camera* miniMapCamera;
	int waypointReached;
	Vector3* cameraWaypoints;

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
	GLuint treeTex;
	GLuint treeBump;
	GLuint heightMapTex;

	float waterRotate;
	float waterCycle;
	float time;
	bool activeDayNight;
	int waves;
};