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

	bool followWaypoints;

protected:
	bool InitialiseTextures();
	bool InitialiseShaders();

	void MoveCamera();
	bool CheckCameraDistance(Vector3 distance, float speed);

	void DrawNode(SceneNode* n);

	void DayNightCycle(float dt);
	Vector3 Rotate(float angle, Vector3 axis, Vector3 position);
	void DrawSkybox();

	void passInfoToShader(Shader* shader, Matrix4 model, SceneNode* n);

	SceneNode* root;

	Shader* sceneShader;
	Shader* islandShader;
	Shader* waterShader;
	Shader* skyboxShader;
	Shader* treeShader;
	Shader* skeletonShader;

	Mesh* quad;

	Light* sun;
	Camera* camera;
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

	float waterRotate;
	float waterCycle;
	float time;
	bool activeDayNight;

	int currentFrame;
	float frameTime;
};