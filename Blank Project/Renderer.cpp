#include "Renderer.h"

#include "../nclgl/Light.h"
#include "../nclgl/Camera.h"
#include "../nclgl/HeightMap.h"
#include "../nclgl/Shader.h"
#include "../nclgl/MeshAnimation.h"
#include "../nclgl/MeshMaterial.h"
#include "SkeletalAnimation.h"
#include "Tree.h"

Renderer::Renderer(Window& parent) : OGLRenderer(parent)
{
	quad = Mesh::GenerateQuad();
	time = 0.0f;
	activeDayNight = true;

	HeightMap* heightMapMesh = new HeightMap(TEXTUREDIR"noiseTexture512.png");
	HeightMap* waterMapMesh = new HeightMap();

	if (!InitialiseTextures())
		return;

	if (!InitialiseShaders())
		return;

	Vector3 heightMapSize = heightMapMesh->GetHeightMapSize();
	camera = new Camera(-15.0f, 0.0f, 0.0f, heightMapSize * Vector3(0.5f, 1.0f, 1.0f));
	sun = new Light(heightMapSize * Vector3(0.5f, 8.0f, 0.5f), Vector4(1, 1, 1, 1), heightMapSize.x*10);
	
	std::cout << heightMapSize << std::endl;

	waypointReached = 0;
	followWaypoints = true;
	cameraWaypoints = new Vector3[2];
	cameraWaypoints[0] = Vector3(heightMapSize * Vector3(0.5f, 0.5f, 0.5f));
	cameraWaypoints[1] = Vector3(heightMapSize * Vector3(0.325f, 0.8f, 0.325f));

	root = new SceneNode();
	SceneNode* heightMap = new SceneNode();
	SceneNode* waterMap = new SceneNode();

	heightMap->SetMesh(heightMapMesh);
	waterMap->SetMesh(waterMapMesh);

	heightMap->SetShader(islandShader);
	waterMap->SetShader(waterShader);
	
	root->AddChild(heightMap);
	root->AddChild(waterMap);

	SkeletalAnimation* soldier = new SkeletalAnimation(Mesh::LoadFromMeshFile("Role_T.msh"), new MeshAnimation("Role_T.anm"), new MeshMaterial("Role_T.mat"), skeletonShader, Vector3(250,200,250));
	heightMap->AddChild(soldier);

	Mesh* cylinder = Mesh::LoadFromMeshFile("../Meshes/Cylinder.msh");
	Mesh* cone = Mesh::LoadFromMeshFile("../Meshes/Cone.msh");
	for (int i = 0; i < 500; ++i)
	{
		int xCoord = rand() % ((int)heightMapSize.x / 2) + (heightMapSize.x / 4);
		int zCoord = rand() % ((int)heightMapSize.z / 2) + (heightMapSize.z / 4);
		int yCoord = heightMapMesh->GetHeightAtCoord(xCoord, zCoord) + 30;
		heightMap->AddChild(new Tree(cylinder, cone, treeShader, Vector3(xCoord, yCoord, zCoord)));
	}
	
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	currentFrame = 0;
	frameTime = 0.0f;
	waterRotate = 0.0f;
	waterCycle = 0.0f;
	init = true;
}
Renderer::~Renderer(void)
{
	delete root;
	delete camera;
	delete[] cameraWaypoints;
	delete quad;
	delete waterShader;
	delete skyboxShader;
	delete islandShader;
	delete sun;
}

void Renderer::UpdateScene(float dt)
{
	if (followWaypoints && waypointReached != 3)
		MoveCamera();
	else
		camera->UpdateCamera(dt);

	time += dt;
	viewMatrix = camera->BuildViewMatrix();
	waterRotate += dt * 0.5f; // 0.5 degrees a second
	waterCycle += dt * 0.0675f; // 2.5 units a second

	root->Update(dt);

	if (activeDayNight)
		DayNightCycle(dt);
}
void Renderer::RenderScene()
{
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	// ordering of these sub calls is important
	DrawSkybox();
	DrawNode(root);
}

void Renderer::MoveCamera()
{
	// MOVE TO WAYPOINT 1
	if (waypointReached == 0)
	{
		Vector3 direction = cameraWaypoints[0] - camera->GetPosition();
		float speed = 5;

		camera->SetPosition(camera->GetPosition() + (direction.Normalised() * speed));
		if (CheckCameraDistance(direction, speed))
		{
			camera->SetPosition(cameraWaypoints[0]);
			waypointReached++;
		}
	}
	// GET BIRDS EYE VIEW
	if (waypointReached == 1)
	{
		float degreesForRotation = 0.5;
		camera->SetPosition(Rotate(degreesForRotation, Vector3(4088, 765, 4088), camera->GetPosition()));
		camera->SetYaw(camera->GetYaw() + degreesForRotation);

		Vector3 direction = cameraWaypoints[0] - camera->GetPosition();
		
		if (CheckCameraDistance(direction, 1))
		{
			camera->SetPosition(cameraWaypoints[0]);
			waypointReached++;
		}
	}
	// GO TO TOP OF MOUNTAIN
	if (waypointReached == 2)
	{
		Vector3 direction = cameraWaypoints[1] - camera->GetPosition();
		float speed = 5;

		camera->SetPosition(camera->GetPosition() + (direction.Normalised() * speed));
		camera->SetYaw(camera->GetYaw() + (speed/10) + 0.05);

		if (CheckCameraDistance(direction, speed))
		{
			camera->SetPosition(cameraWaypoints[1]);
			waypointReached++;
		}
	}
	// IF REACHED FINAL POINT
	if (waypointReached == 3)
	{
		followWaypoints = false;
		waypointReached = -1;
	}
}
bool Renderer::CheckCameraDistance(Vector3 distance, float speed)
{
	if ((distance.x <= (0.5 * speed) && distance.x >= -(0.5 * speed)) && (distance.y <= (0.5 * speed) && distance.y >= -(0.5 * speed)) &&(distance.z <= (0.5 * speed) && distance.z >= -(0.5 * speed)))
		return true;
	return false;
}

void Renderer::DrawNode(SceneNode* n)
{
	if (n->IsAnimated())
	{
		Matrix4 model = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());
		passInfoToShader(n->GetShader(), model, n);
		glUniform1i(glGetUniformLocation(n->GetShader()->GetProgram(), "diffuseTex"), 10);
		UpdateShaderMatrices();

		vector<Matrix4> frameMatrices;

		const Matrix4* invBindPose = n->GetMesh()->GetInverseBindPose();
		const Matrix4* frameData = n->GetAnim()->GetJointData(currentFrame);

		for (unsigned int i = 0; i < n->GetMesh()->GetJointCount(); ++i)
		{
			frameMatrices.emplace_back(frameData[i] * invBindPose[i]);
		}

		int j = glGetUniformLocation(n->GetShader()->GetProgram(), "joints");
		glUniformMatrix4fv(j, frameMatrices.size(), false, (float*)frameMatrices.data());

		for (int i = 0; i < n->GetMesh()->GetSubMeshCount(); ++i)
		{
			glActiveTexture(GL_TEXTURE10);
			glBindTexture(GL_TEXTURE_2D, n->GetMatTextures()[i]);
			n->GetMesh()->DrawSubMesh(i);
		}
	}
	else
	{
		if (n->GetMesh())
		{
			Matrix4 model = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());
			modelMatrix = model;
			passInfoToShader(n->GetShader(), model, n);
			SetShaderLight(*sun);
			UpdateShaderMatrices();

			n->Draw(*this);
		}
	}

	for (vector<SceneNode*>::const_iterator i = n->GetChildIteratorStart(); i != n->GetChildIteratorEnd(); ++i)
	{
		DrawNode(*i);
	}
}

void Renderer::DayNightCycle(float dt)
{
	sun->SetPosition(Rotate(3 * dt, Vector3(4088, 0, 4088), sun->GetPosition()));
}
Vector3 Renderer::Rotate(float angle, Vector3 axis, Vector3 position)
{
	Matrix4 rotationMatrix = Matrix4::Rotation(angle, axis);
	Vector3 currentPosition = position;
	Vector3 newPosition;
	newPosition.x = (currentPosition.x * rotationMatrix.values[0]) + (currentPosition.y * rotationMatrix.values[4]) + (currentPosition.z * rotationMatrix.values[8]);
	newPosition.y = (currentPosition.x * rotationMatrix.values[1]) + (currentPosition.y * rotationMatrix.values[5]) + (currentPosition.z * rotationMatrix.values[9]);
	newPosition.z = (currentPosition.x * rotationMatrix.values[2]) + (currentPosition.y * rotationMatrix.values[6]) + (currentPosition.z * rotationMatrix.values[10]);
	return newPosition;
}
void Renderer::DrawSkybox()
{
	glDepthMask(GL_FALSE);

	BindShader(skyboxShader);
	UpdateShaderMatrices();

	quad->Draw();

	glDepthMask(GL_TRUE);
}

bool Renderer::InitialiseTextures()
{
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
		return false;

	SetTextureRepeating(sandTex, true);
	SetTextureRepeating(sandBump, true);
	SetTextureRepeating(grassTex, true);
	SetTextureRepeating(grassBump, true);
	SetTextureRepeating(pebbleTex, true);
	SetTextureRepeating(pebbleBump, true);
	SetTextureRepeating(stoneTex, true);
	SetTextureRepeating(stoneBump, true);
	SetTextureRepeating(waterTex, true);

	return true;
}
bool Renderer::InitialiseShaders()
{
	waterShader = new Shader("WaterVertex.glsl", "WaterFragment.glsl");
	skyboxShader = new Shader("SkyboxVertex.glsl", "SkyboxFragment.glsl");
	islandShader = new Shader("IslandVertex.glsl", "IslandFragment.glsl");
	sceneShader = new Shader("SceneVertex.glsl", "SceneFragment.glsl");
	treeShader = new Shader("TreeVertex.glsl", "TreeFragment.glsl");
	skeletonShader = new Shader("SkinningVertex.glsl", "TexturedFragment.glsl");

	if (!waterShader->LoadSuccess() || !skyboxShader->LoadSuccess() || !islandShader->LoadSuccess() || !sceneShader->LoadSuccess() || !treeShader->LoadSuccess() || !skeletonShader->LoadSuccess())
		return false;

	return true;
}
void Renderer::passInfoToShader(Shader* shader, Matrix4 model, SceneNode* n)
{
	BindShader(shader);
	// CAMERA
	glUniform3fv(glGetUniformLocation(shader->GetProgram(), "cameraPos"), 1, (float*)& camera->GetPosition());

	// SCENE NODE
	glUniformMatrix4fv(glGetUniformLocation(shader->GetProgram(), "modelMatrix"), 1, false, model.values);

	glUniform4fv(glGetUniformLocation(shader->GetProgram(), "nodeColour"), 1, (float*)& n->GetColour());

	glUniform1i(glGetUniformLocation(shader->GetProgram(), "useTexture"), 0);

	// TEXTURES
	glUniform1i(glGetUniformLocation(shader->GetProgram(), "sandTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, sandTex);

	glUniform1i(glGetUniformLocation(shader->GetProgram(), "sandBumpTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, sandBump);

	glUniform1i(glGetUniformLocation(shader->GetProgram(), "skyboxTex"), 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);

	glUniform1i(glGetUniformLocation(shader->GetProgram(), "grassTex"), 3);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, grassTex);

	glUniform1i(glGetUniformLocation(shader->GetProgram(), "grassBumpTex"), 4);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, grassBump);

	glUniform1i(glGetUniformLocation(shader->GetProgram(), "stoneTex"), 5);
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, stoneTex);

	glUniform1i(glGetUniformLocation(shader->GetProgram(), "stoneBumpTex"), 6);
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, stoneBump);

	glUniform1i(glGetUniformLocation(shader->GetProgram(), "pebbleTex"), 7);
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, pebbleTex);

	glUniform1i(glGetUniformLocation(shader->GetProgram(), "pebbleBumpTex"), 8);
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, pebbleBump);

	glUniform1i(glGetUniformLocation(shader->GetProgram(), "waterTex"), 9);
	glActiveTexture(GL_TEXTURE9);
	glBindTexture(GL_TEXTURE_2D, waterTex);
	
	// WAVE DATA 
	glUniform1f(glGetUniformLocation(shader->GetProgram(), "time"), time);
	glUniform2f(glGetUniformLocation(shader->GetProgram(), "gerstnerWaves[0].direction"), 1.0f, 0.0f);
	glUniform1f(glGetUniformLocation(shader->GetProgram(), "gerstnerWaves[0].amplitude"), 15.0);
	glUniform1f(glGetUniformLocation(shader->GetProgram(), "gerstnerWaves[0].steepness"), 0.5);
	glUniform1f(glGetUniformLocation(shader->GetProgram(), "gerstnerWaves[0].frequency"), 0.005);
	glUniform1f(glGetUniformLocation(shader->GetProgram(), "gerstnerWaves[0].speed"), 0.5);

	glUniform2f(glGetUniformLocation(shader->GetProgram(), "gerstnerWaves[1].direction"), 1.0f, 1.0f);
	glUniform1f(glGetUniformLocation(shader->GetProgram(), "gerstnerWaves[1].amplitude"), 20.0);
	glUniform1f(glGetUniformLocation(shader->GetProgram(), "gerstnerWaves[1].steepness"), 15.0);
	glUniform1f(glGetUniformLocation(shader->GetProgram(), "gerstnerWaves[1].frequency"), 0.001);
	glUniform1f(glGetUniformLocation(shader->GetProgram(), "gerstnerWaves[1].speed"), 1.0);
	glUniform1i(glGetUniformLocation(shader->GetProgram(), "gerstnerWavesLength"), 1);
}