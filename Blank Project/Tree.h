#pragma once
#include "..\nclgl\scenenode.h"

class Tree : public SceneNode
{
public:
	Tree(Mesh* trunkMesh, Mesh* leavesMesh, Shader* shader, Vector3 location);
	~Tree(void) {};
	void Update(float dt) override;

protected:
	SceneNode* leaves;
};