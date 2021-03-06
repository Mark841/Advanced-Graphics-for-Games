#include "Renderer.h"

#include "../nclgl/Light.h"
#include "../nclgl/Camera.h"
#include "../nclgl/HeightMap.h"
#include "../nclgl/Shader.h"
#include "../nclgl/MeshAnimation.h"
#include "../nclgl/MeshMaterial.h"
#include "SkeletalAnimation.h"
#include "Tree.h"

const int POST_PASSES = 100;

Renderer::Renderer(Window& parent) : OGLRenderer(parent)
{
	quad = Mesh::GenerateQuad();
	time = 0.0f;
	waves = 1;
	activeDayNight = true;

	heightMapMesh = new HeightMap(TEXTUREDIR"noiseTexture512.png");
	waterMapMesh = new HeightMap();

	if (!InitialiseTextures())
		return;

	if (!InitialiseShaders())
		return;

	if (!InitialiseBuffers())
		return;

	Vector3 heightMapSize = heightMapMesh->GetHeightMapSize();
	camera = new Camera(-15.0f, 0.0f, 0.0f, heightMapSize * Vector3(0.5f, 1.0f, 1.0f));
	miniMapCamera = new Camera(-90.0f, 0.0f, 0.0f, camera->GetPosition());
	sun = new Light(heightMapSize * Vector3(0.5f, 8.0f, 0.5f), Vector4(1, 1, 1, 1), heightMapSize.x*3);
	
	waypointReached = 0;
	followWaypoints = true;
	cameraWaypoints = new Vector3[3];
	cameraWaypoints[0] = Vector3(heightMapSize * Vector3(0.5f, 0.5f, 0.5f));
	cameraWaypoints[1] = Vector3(heightMapSize * Vector3(0.325f, 0.8f, 0.325f));
	cameraWaypoints[2] = Vector3(2590.5, 902, 2640);

	root = new SceneNode();
	SceneNode* heightMap = new SceneNode();
	SceneNode* waterMap = new SceneNode();

	heightMap->SetMesh(heightMapMesh);
	waterMap->SetMesh(waterMapMesh);

	heightMap->SetShader(islandShader);
	waterMap->SetShader(waterShader);
	
	root->AddChild(heightMap);
	root->AddChild(waterMap);

	SkeletalAnimation* soldier = new SkeletalAnimation(Mesh::LoadFromMeshFile("Role_T.msh"), new MeshAnimation("Role_T.anm"), new MeshMaterial("Role_T.mat"), skeletonShader, Vector3(heightMapSize.x / 2, 200, heightMapSize.z / 2));
	heightMap->AddChild(soldier);

	Mesh* cylinder = Mesh::LoadFromMeshFile("../Meshes/Cylinder.msh");
	Mesh* cone = Mesh::LoadFromMeshFile("../Meshes/Cone.msh");
	for (int i = 0; i < 50; ++i)
	{
		int xCoord = rand() % ((int)heightMapSize.x / 2) + (heightMapSize.x / 4);
		int zCoord = rand() % ((int)heightMapSize.z / 2) + (heightMapSize.z / 4);
		int yCoord = heightMapMesh->GetHeightAtCoord(xCoord, zCoord) + 30;
		heightMap->AddChild(new Tree(cylinder, cone, treeShader, Vector3(xCoord, yCoord, zCoord)));
	}
	
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
	delete root;
	delete camera;
	delete miniMapCamera;
	delete[] cameraWaypoints;
	delete quad;
	delete waterShader;
	delete skyboxShader;
	delete islandShader;
	delete skeletonShader;
	delete postProcessShader;
	delete sun;
	delete heightMapMesh;
	delete waterMapMesh;

	glDeleteTextures(2, bufferColourTex);
	glDeleteTextures(1, &bufferDepthTex);
	glDeleteFramebuffers(1, &bufferFBO);
	glDeleteFramebuffers(1, &processFBO);
	glDeleteFramebuffers(1, &mapFBO);
	glDeleteTextures(2, mapColourTex);
	glDeleteTextures(1, &mapDepthTex);
}

void Renderer::UpdateScene(float dt)
{
	if (followWaypoints && waypointReached != 4)
		MoveCamera();
	else
		camera->UpdateCamera(dt);

	UpdateMiniMapCamera();

	time += dt;
	viewMatrix = camera->BuildViewMatrix();
	mapViewMatrix = miniMapCamera->BuildViewMatrix();
	waterRotate += dt * 0.5f; // 0.5 degrees a second
	waterCycle += dt * 0.0675f; // 2.5 units a second

	root->Update(dt);

	if (activeDayNight)
		DayNightCycle(dt);
}
void Renderer::RenderScene()
{
	// Draw main scene
	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	DrawScene();
	DrawPostProcess();

	// Draw scene again but for map
	glBindFramebuffer(GL_FRAMEBUFFER, mapFBO);
	viewMatrix = mapViewMatrix;
	DrawScene();

	// Display scene
	PresentScene();
}

void Renderer::DrawScene()
{
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	DrawSkybox();
	DrawNode(root);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void Renderer::DrawPostProcess()
{
	// If camera is underwater apply blur, cant get exact location for when camera goes underwater as calculations are applied in the shader for manipulating the mesh vertexs
	if ((camera->GetPosition().x > 0 && camera->GetPosition().x < waterMapMesh->GetHeightMapSize().x) && (camera->GetPosition().z > 0 && camera->GetPosition().z < waterMapMesh->GetHeightMapSize().z))
	{
		if (camera->GetPosition().y < waterMapMesh->GetHeightAtCoord(camera->GetPosition().x, camera->GetPosition().z) + 200)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, processFBO);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[1], 0);
			glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

			BindShader(postProcessShader);
			modelMatrix.ToIdentity();
			viewMatrix.ToIdentity();
			projMatrix.ToIdentity();
			UpdateShaderMatrices();

			glDisable(GL_DEPTH_TEST);

			glActiveTexture(GL_TEXTURE0);
			glUniform1i(glGetUniformLocation(postProcessShader->GetProgram(), "sceneTex"), 0);
			for (int i = 0; i < POST_PASSES; ++i)
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[1], 0);
				glUniform1i(glGetUniformLocation(postProcessShader->GetProgram(), "isVertical"), 0);

				glBindTexture(GL_TEXTURE_2D, bufferColourTex[0]);
				quad->Draw();

				// Now to swap the colour buffers and do the second blur pass
				glUniform1i(glGetUniformLocation(postProcessShader->GetProgram(), "isVertical"), 1);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[0], 0);
				glBindTexture(GL_TEXTURE_2D, bufferColourTex[1]);
				quad->Draw();
			}
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glEnable(GL_DEPTH_TEST);
		}
	}
}
void Renderer::PresentScene()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	BindShader(sceneShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "diffuseTex"), 0);

	// draw minimap
	glBindTexture(GL_TEXTURE_2D, mapColourTex[0]);
	glViewport(0, (height / 4)*3, width / 4, height / 4);
	quad->Draw();

	// draw scene
	glBindTexture(GL_TEXTURE_2D, bufferColourTex[0]);
	glViewport(0, 0, width, height);
	quad->Draw();
}

void Renderer::MoveCamera()
{
	// MOVE TO WAYPOINT 1
	if (waypointReached == 0)
	{
		Vector3 direction = cameraWaypoints[0] - camera->GetPosition();
		float speed = 2.5;

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
		float degreesForRotation = 0.25;
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
		float speed = 2.5;

		camera->SetPosition(camera->GetPosition() + (direction.Normalised() * speed));
		camera->SetYaw(camera->GetYaw() + (speed/10) + 0.05);

		if (CheckCameraDistance(direction, speed))
		{
			camera->SetPosition(cameraWaypoints[1]);
			waypointReached++;
		}
	}
	// ROTATE TO FACE SOLDIER
	if (waypointReached == 3)
	{
		float degreesForRotation = 0.25;
		camera->SetPosition(Rotate(degreesForRotation, Vector3(2657, 765, 2657), camera->GetPosition()));
		camera->SetYaw(camera->GetYaw() + degreesForRotation);
		camera->SetPitch(camera->GetPitch() + degreesForRotation/10);

		Vector3 direction = cameraWaypoints[2] - camera->GetPosition();
		if (CheckCameraDistance(direction, 1))
		{
			camera->SetPosition(cameraWaypoints[2]);
			waypointReached++;
		}
	}
	// IF REACHED FINAL POINT
	if (waypointReached == 4)
	{
		followWaypoints = false;
		waypointReached = -1;
	}
}
void Renderer::UpdateMiniMapCamera()
{
	miniMapCamera->SetPosition(camera->GetPosition());
	miniMapCamera->SetPosition(Vector3(miniMapCamera->GetPosition().x, heightMapMesh->GetHeightMapSize().y * 3, miniMapCamera->GetPosition().z));
	miniMapCamera->SetPitch(-90);
	miniMapCamera->SetYaw(0);
	miniMapCamera->SetRoll(0);
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
		modelMatrix = model;
		passInfoToShader(n->GetShader(), model, n);
		glUniform1i(glGetUniformLocation(n->GetShader()->GetProgram(), "diffuseTex"), 11);
		UpdateShaderMatrices();
		
		vector<Matrix4> frameMatrices;

		const Matrix4* invBindPose = n->GetMesh()->GetInverseBindPose();
		const Matrix4* frameData = n->GetAnim()->GetJointData(n->GetCurrentFrame());

		for (unsigned int i = 0; i < n->GetMesh()->GetJointCount(); ++i)
		{
			frameMatrices.emplace_back(frameData[i] * invBindPose[i]);
		}

		int j = glGetUniformLocation(n->GetShader()->GetProgram(), "joints");
		glUniformMatrix4fv(j, frameMatrices.size(), false, (float*)frameMatrices.data());

		for (int i = 0; i < n->GetMesh()->GetSubMeshCount(); ++i)
		{
			glActiveTexture(GL_TEXTURE11);
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
	treeTex = SOIL_load_OGL_texture(TEXTUREDIR"treeTexture.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	treeBump = SOIL_load_OGL_texture(TEXTUREDIR"treeBumpMap.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	heightMapTex = SOIL_load_OGL_texture(TEXTUREDIR"noiseTexture512.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	cubeMap = SOIL_load_OGL_cubemap(TEXTUREDIR"tropical_right.jpg", TEXTUREDIR"tropical_left.jpg", TEXTUREDIR"tropical_top.jpg", TEXTUREDIR"tropical_bottom.jpg", TEXTUREDIR"tropical_front.jpg", TEXTUREDIR"tropical_back.jpg", SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	if (!sandTex || !sandBump || !grassTex || !grassBump || !stoneTex || !stoneBump || !cubeMap || !waterTex || !heightMapTex || !treeTex || !treeBump)
		return false;

	SetTextureRepeating(sandTex, true);
	SetTextureRepeating(sandBump, true);
	SetTextureRepeating(grassTex, true);
	SetTextureRepeating(grassBump, true);
	SetTextureRepeating(pebbleTex, true);
	SetTextureRepeating(pebbleBump, true);
	SetTextureRepeating(stoneTex, true);
	SetTextureRepeating(stoneBump, true);
	SetTextureRepeating(treeTex, true);
	SetTextureRepeating(treeBump, true);
	SetTextureRepeating(waterTex, true);

	return true;
}
bool Renderer::InitialiseShaders()
{
	waterShader = new Shader("WaterVertex.glsl", "WaterFragment.glsl");
	skyboxShader = new Shader("SkyboxVertex.glsl", "SkyboxFragment.glsl");
	islandShader = new Shader("IslandVertex.glsl", "IslandFragment.glsl");
	sceneShader = new Shader("TexturedVertex.glsl", "TexturedFragment.glsl");
	treeShader = new Shader("TreeVertex.glsl", "TreeFragment.glsl");
	skeletonShader = new Shader("SkinningVertex.glsl", "TexturedFragment.glsl");
	postProcessShader = new Shader("TexturedVertex.glsl", "ProcessFrag.glsl");

	if (!waterShader->LoadSuccess() || !skyboxShader->LoadSuccess() || !islandShader->LoadSuccess() || !sceneShader->LoadSuccess() || !treeShader->LoadSuccess() || !skeletonShader->LoadSuccess() || !postProcessShader->LoadSuccess())
		return false;

	return true;
}
bool Renderer::InitialiseBuffers()
{
	// Generate scene depth texture
	glGenTextures(1, &bufferDepthTex);
	glBindTexture(GL_TEXTURE_2D, bufferDepthTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

	// and colour texture
	for (int i = 0; i < 2; ++i)
	{
		glGenTextures(1, &bufferColourTex[i]);
		glBindTexture(GL_TEXTURE_2D, bufferColourTex[i]);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	}

	// Generate map depth texture
	glGenTextures(1, &mapDepthTex);
	glBindTexture(GL_TEXTURE_2D, mapDepthTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

	// and colour texture
	for (int i = 0; i < 2; ++i)
	{
		glGenTextures(1, &mapColourTex[i]);
		glBindTexture(GL_TEXTURE_2D, mapColourTex[i]);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	}

	glGenFramebuffers(1, &bufferFBO);	// Render scene into this
	glGenFramebuffers(1, &mapFBO);	// Render map into this
	glGenFramebuffers(1, &processFBO);	// Do post processing here

	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, bufferDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, bufferDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[0], 0);

	glBindFramebuffer(GL_FRAMEBUFFER, mapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, mapDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, mapDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mapColourTex[0], 0);

	// We can check FBO attacment success using this command
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE || !bufferDepthTex || !bufferColourTex[0] || !mapDepthTex || !mapColourTex[0])
	{
		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

	glUniform1i(glGetUniformLocation(shader->GetProgram(), "heightMapTex"), 10);
	glActiveTexture(GL_TEXTURE10);
	glBindTexture(GL_TEXTURE_2D, heightMapTex);

	glUniform1i(glGetUniformLocation(shader->GetProgram(), "treeTex"), 12);
	glActiveTexture(GL_TEXTURE12);
	glBindTexture(GL_TEXTURE_2D, treeTex);

	glUniform1i(glGetUniformLocation(shader->GetProgram(), "treeBumpTex"), 13);
	glActiveTexture(GL_TEXTURE13);
	glBindTexture(GL_TEXTURE_2D, treeBump);
	
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

	glUniform2f(glGetUniformLocation(shader->GetProgram(), "gerstnerWaves[2].direction"), 1.0f, 0.0f);
	glUniform1f(glGetUniformLocation(shader->GetProgram(), "gerstnerWaves[2].amplitude"), 50.0);
	glUniform1f(glGetUniformLocation(shader->GetProgram(), "gerstnerWaves[2].steepness"), 2.5);
	glUniform1f(glGetUniformLocation(shader->GetProgram(), "gerstnerWaves[2].frequency"), 0.0005);
	glUniform1f(glGetUniformLocation(shader->GetProgram(), "gerstnerWaves[2].speed"), 1.0);

	glUniform2f(glGetUniformLocation(shader->GetProgram(), "gerstnerWaves[3].direction"), 1.0f, 1.0f);
	glUniform1f(glGetUniformLocation(shader->GetProgram(), "gerstnerWaves[3].amplitude"), 50.0);
	glUniform1f(glGetUniformLocation(shader->GetProgram(), "gerstnerWaves[3].steepness"), 15.0);
	glUniform1f(glGetUniformLocation(shader->GetProgram(), "gerstnerWaves[3].frequency"), 0.00001);
	glUniform1f(glGetUniformLocation(shader->GetProgram(), "gerstnerWaves[3].speed"), 0.5);

	glUniform1i(glGetUniformLocation(shader->GetProgram(), "gerstnerWavesLength"), waves);
}