#include "BatchRenderer2D.hpp"
#include "Path.hpp"

BatchRenderer2D::BatchRenderer2D(unsigned max_quads)
{
	maxVertices = max_quads * 4; // each quad have 4 vertices
	maxIndices	= max_quads * 6;
	vertexData.resize(maxVertices);

	// wait until other stuffs are ready.. ->Init
}

void BatchRenderer2D::Init()
{
	// load shaders
	const std::filesystem::path vertex_file	  = assets::locate_asset("Assets/shaders/batch.vert");
	const std::filesystem::path fragment_file = assets::locate_asset("Assets/shaders/batch.frag");
	shader									  = OpenGL::CreateShader(vertex_file, fragment_file);

	// create vertex array object, buffer vertices, buffer indices
	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(QuadVertex) * maxVertices, nullptr, GL_DYNAMIC_DRAW); // every frame we'll update our buffer
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//setup index buffer
	//unlike vertex buffer that gonna change every frame, index don't change
	//i.e. 0 1 2 2 3 0 ... << this pattern repeat
	//but just amount of index gonna change(how many do we need?)
	std::vector<unsigned> indice_values(maxIndices);
	unsigned			  offset = 0;
	for (unsigned i = 0; i < maxIndices; i += 6)
	{
		indice_values[i + 0] = offset + 0;
		indice_values[i + 1] = offset + 1;
		indice_values[i + 2] = offset + 2;
		indice_values[i + 3] = offset + 2;
		indice_values[i + 4] = offset + 3;
		indice_values[i + 5] = offset + 0;
	}
}