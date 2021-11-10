void Tutorial11(Renderer& renderer)
{
	std::vector<Vector3> verts;

	for (int i = 0; i < 100; ++i)
	{
		float x = (float)(rand() % 100 - 50);
		float y = (float)(rand() % 100 - 50);
		float z = (float)(rand() % 100 - 50);

		verts.push_back(Vector3(x, y, z));
	}

	OGLMesh* pointSprites = new OGLMesh();
	pointSprites->SetVertexPositions(verts);
	pointSprites->SetPrimitiveType(GeometryPrimitive::Points);
	pointSprites->UploadToGPU();

	Matrix4 modelMat = Matrix4::Translation(Vector3(0, 0, -30));

	OGLShader* newShader = new OGLShader("RasterisationVert.glsl", "RasterisationFrag.glsl", "PointGeom.glsl");

	RenderObject* object = new RenderObject(pointSprites, modelMat);

	object->SetBaseTexture(OGLTexture::RGBATextureFromFilename("nyan.png"));
	object->SetShader(newShader);

	renderer.AddRenderObject(object);
}