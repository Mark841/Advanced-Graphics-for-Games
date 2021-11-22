#include "Tree.h"

Tree::Tree(Mesh* trunkMesh, Mesh* leavesMesh, Shader* shader, Vector3 location)
{
	SceneNode* treeBody = new  SceneNode(trunkMesh, Vector4(0.545, 0.271, 0.075, 1));    //Red!
	treeBody->SetShader(shader);
	treeBody->SetModelScale(Vector3(5, 8, 5));
	treeBody->SetTransform(Matrix4::Translation(location) * Matrix4::Scale(Vector3(5, 8, 5)));
	treeBody->SetBoundingRadius(15.0f);
	AddChild(treeBody);
	
	leaves = new  SceneNode(leavesMesh, Vector4(0.502, 0.502, 0, 1));                // Green!
	leaves->SetShader(shader);
	leaves->SetModelScale(Vector3(5, 5, 5));
	leaves->SetTransform(Matrix4::Translation(Vector3(0, 10, 0)) * Matrix4::Rotation(-90, Vector3(1,0,0)));
	leaves->SetBoundingRadius(5.0f);
	treeBody->AddChild(leaves);
}

void Tree::Update(float dt)
{
	//SetTransform(GetTransform() * Matrix4::Scale(Vector3(1, 1.01, 1)));
	SceneNode::Update(dt);
}