#pragma once
#include "../nclgl/SceneNode.h"

class SkeletalAnimation : public SceneNode
{
public:
	SkeletalAnimation(Mesh* mesh, MeshAnimation* anim, MeshMaterial* material, Shader* shader, Vector3 location);
	~SkeletalAnimation(void);
	void Update(float dt) override;

protected:
	int currentFrame;
	float frameTime;
};