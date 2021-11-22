#include "SkeletalAnimation.h"

SkeletalAnimation::SkeletalAnimation(Mesh* mesh, MeshAnimation* anim, MeshMaterial* material, Shader* shader, Vector3 location)
{
	this->mesh = mesh;
	this->anim = anim;
	this->material = material;
	animated = true;
	SetShader(shader);

	for (int i = 0; i < mesh->GetSubMeshCount(); ++i)
	{
		const MeshMaterialEntry* matEntry = material->GetMaterialForLayer(i);

		const string* filename = nullptr;
		matEntry->GetEntry("Diffuse", &filename);
		string path = TEXTUREDIR + *filename;
		GLuint texID = SOIL_load_OGL_texture(path.c_str(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y);
		matTextures.emplace_back(texID);
	}
	SetTransform(Matrix4::Translation(location)* Matrix4::Scale(Vector3(3, 800, 3)));

	currentFrame = 0;
	frameTime = 0.0f;
}
SkeletalAnimation::~SkeletalAnimation(void)
{
	delete mesh;
	delete anim;
	delete material;
}

void SkeletalAnimation::Update(float dt)
{
	frameTime -= dt;
	while (frameTime < 0.0f)
	{
		currentFrame = (currentFrame + 1) % anim->GetFrameCount();
		frameTime += 1.0f / anim->GetFrameRate();
	}
}