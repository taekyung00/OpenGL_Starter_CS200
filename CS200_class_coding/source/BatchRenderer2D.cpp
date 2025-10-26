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

	// setup index buffer

	// unlike vertex buffer that gonna change every frame, index don't change
	// i.e. 0 1 2 2 3 0 ... << this pattern repeat
	// but just amount of index gonna change(how many do we need?)
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

	glGenBuffers(1, &indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned) * maxIndices, indice_values.data(), GL_STATIC_DRAW); // sizeof(unsigned) * maxIndices == sizeof(indice_values)
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// Create vertex array object
	glGenVertexArrays(1, &vertexArrayObject);
	glBindVertexArray(vertexArrayObject);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexBuffer);

	// Position attribute (location 0)
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), nullptr);
	glVertexAttribDivisor(0, 0);

	// Texture coordinate attribute (location 1)
	glEnableVertexAttribArray(1);
	const ptrdiff_t texcoord_offset = 2 * sizeof(float);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), reinterpret_cast<void*>(texcoord_offset));
	glVertexAttribDivisor(1, 0);

	// Tint attribute (location 2)
	glEnableVertexAttribArray(2);
	// const ptrdiff_t tint_offset = 4 * sizeof(float);
	const ptrdiff_t tint_offset = offsetof(QuadVertex, tint); // automatically calculate offset!
	glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(QuadVertex), reinterpret_cast<void*>(tint_offset));
	glVertexAttribDivisor(2, 0);

	// Unbind VAO and buffers
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// Enable blending for transparency
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
}

void BatchRenderer2D::Shutdown()
{
	glDeleteVertexArrays(1, &vertexArrayObject);
	glDeleteBuffers(1, &vertexBuffer);
	glDeleteBuffers(1, &indexBuffer);
	glDeleteProgram(shader.Shader);
}

void BatchRenderer2D::BeginScene(std::span<const float, 9> ndc_matrix)
{
	glUseProgram(shader.Shader);
	glUniformMatrix3fv(shader.UniformLocations.at("uToNDC"), 1, GL_FALSE, ndc_matrix.data());
	glUseProgram(0);

	startBatch();
}

void BatchRenderer2D::EndScene()
{
	flush();
}

void BatchRenderer2D::startBatch()
{
	vertexDataEnd = vertexData.data();
	indexCount	  = 0;
}