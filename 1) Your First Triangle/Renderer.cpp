#include "Renderer.h"

Renderer::Renderer(Window& parent) : OGLRenderer(parent)
{
	triangle = Mesh::GenerateTriangle();

	basicShader = new Shader("basicVertex.glsl", "colourFragment.glsl");

	if (!basicShader->LoadSuccess())
	{
		return;
	}
	init = true;
}
Renderer::~Renderer(void) 
{
	delete triangle;
	delete basicShader;
}

//float movement = 0.01f;
Vector3 movement((float)(rand() % 5) / 100, (float)(rand() % 5) / 100, (float)(rand() % 5) / 100);
bool bounce = false;
float size_multiplier = 1.0f;
void Renderer::RenderScene()
{
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	BindShader(basicShader);
	triangle->Draw();

	glBindBuffer(GL_ARRAY_BUFFER, triangle->getBufferObject(MeshBuffer::COLOUR_BUFFER));
	Vector4* triangles_colour = (Vector4*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
	for (int i = 0; i < 4; i++)
	{ // Randomise all the colours
		triangles_colour[i].x = (float)(rand() % 1000) / 1000;
		triangles_colour[i].y = (float)(rand() % 1000) / 1000;
		triangles_colour[i].z = (float)(rand() % 1000) / 1000;
	}
	/*for (int i = 0; i < 4; i++)
	{ // Fade the colours to black
		triangles_colour[i].x = triangles_colour[i].x * 0.99;
		triangles_colour[i].y = triangles_colour[i].y * 0.99;
		triangles_colour[i].z = triangles_colour[i].z * 0.99;
	}*/
	glUnmapBuffer(GL_ARRAY_BUFFER);

	glBindBuffer(GL_ARRAY_BUFFER, triangle->getBufferObject(MeshBuffer::VERTEX_BUFFER));
	Vector3* vertexs_positions = (Vector3*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
	/*for (int i = 0; i < 4; i++)
	{ // Randomise all points
		vertexs_positions[i].x = ((float)(rand() % 1000) / 1000) - 0.5f;
		vertexs_positions[i].y = ((float)(rand() % 1000) / 1000) - 0.5f;
		vertexs_positions[i].z = ((float)(rand() % 1000) / 1000) - 0.5f;
	}*/
	//for (int i = 0; i < 4; i++)
	//{ // Shrink the quad
	//	vertexs_positions[i].x = vertexs_positions[i].x * 0.99;
	//	vertexs_positions[i].y = vertexs_positions[i].y * 0.99;
	//	vertexs_positions[i].z = vertexs_positions[i].z * 0.99;
	//}

	//// Bounce and shrink the quad
	//if (vertexs_positions[1].x >= 1.0f)
	//{
	//	bounce = true;
	//	size_multiplier = 0.992f;
	//	movement = -movement;
	//}
	//if (vertexs_positions[2].x <= -1.0f)
	//{
	//	bounce = false;
	//	size_multiplier = 1.008f;
	//	movement = -movement;
	//}
	//for (int i = 0; i < 4; i++)
	//{ 
	//	vertexs_positions[i].x = (vertexs_positions[i].x * size_multiplier) + movement;
	//	vertexs_positions[i].y = (vertexs_positions[i].y * size_multiplier) + movement;
	//	vertexs_positions[i].z = (vertexs_positions[i].z * size_multiplier) + movement;
	//}

	// Bounce and shrink the quad randomly
	if (vertexs_positions[1].x >= 1.0f)
	{ // if top right point goes out of right side
		size_multiplier = 0.99f;
		movement.x = -movement.x;
	}
	if (vertexs_positions[2].x <= -1.0f)
	{ // if bottom left point goes out of left side
		size_multiplier = 1.01f;
		movement.x = -movement.x;
	}
	if (vertexs_positions[1].y >= 1.0f)
	{ // if top right point goes out of top side
		size_multiplier = 0.99f;
		movement.y = -movement.y;
	}
	if (vertexs_positions[2].y <= -1.0f)
	{ // if bottom left point goes out of bottom side
		size_multiplier = 1.01f;
		movement.y = -movement.y;
	}
	for (int i = 0; i < 4; i++)
	{ 
		vertexs_positions[i] = (vertexs_positions[i] * size_multiplier) + movement;
	}
	glUnmapBuffer(GL_ARRAY_BUFFER);
}