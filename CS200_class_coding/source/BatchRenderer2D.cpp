#include "BatchRenderer2D.hpp"

#include "Path.hpp"
#include <fstream>
#include <numeric>

BatchRenderer2D::BatchRenderer2D(unsigned max_quads)
{
	maxVertices = max_quads * 4; // each quad have 4 vertices
	maxIndices	= max_quads * 6;
	vertexData.resize(maxVertices);

	// wait until other stuffs are ready.. ->Init
}

void BatchRenderer2D::Init()
{
	// get how many texture opengl can draw
	GLint max_tex_units = 0;
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_tex_units); // check with docs.gl to get minimum(16) and maximum
	textureSlots.resize(static_cast<size_t>(std::min(max_tex_units, 64)));

	// load shaders with parsing
	const std::filesystem::path vertex_file = assets::locate_asset("Assets/shaders/batch.vert");
	std::ifstream				vert_stream(vertex_file);
	std::stringstream			vert_text_stream;
	vert_text_stream << vert_stream.rdbuf();
	const std::string vertex_glsl = vert_text_stream.str();


	const std::filesystem::path fragment_file = assets::locate_asset("Assets/shaders/batch.frag");
	std::ifstream				frag_stream(fragment_file);
	std::stringstream			frag_text_stream;
	frag_text_stream << frag_stream.rdbuf();
	std::string		  frag_glsl		= frag_text_stream.str();
	const size_t	  first_newline = frag_glsl.find('\n');
	const std::string define_line	= "\n#define MAX_TEXTURE_SLOTS " + std::to_string(textureSlots.size());
	frag_glsl.insert(first_newline, define_line);

	shader = OpenGL::CreateShader(std::string_view{ vertex_glsl }, std::string_view{ frag_glsl });

	// have to set their binding index
	glUseProgram(shader.Shader);

	std::vector<int> sampler_binding_values(textureSlots.size());
	std::iota(sampler_binding_values.begin(), sampler_binding_values.end(), 0);
	std::iota(std::begin(sampler_binding_values), std::end(sampler_binding_values), 0);
	const GLint location = glGetUniformLocation(shader.Shader, "uTextures");
	glUniform1iv(location, static_cast<GLsizei>(textureSlots.size()), sampler_binding_values.data());

	glUseProgram(0);

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
		offset += 4;
	}

	glGenBuffers(1, &indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned) * maxIndices, indice_values.data(), GL_STATIC_DRAW); // sizeof(unsigned) * maxIndices == sizeof(indice_values)
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// Create vertex array object
	glGenVertexArrays(1, &vertexArrayObject);
	glBindVertexArray(vertexArrayObject);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);

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

	// texture index attribute(location 3)
	glEnableVertexAttribArray(3);
	const ptrdiff_t tex_index_offset = offsetof(QuadVertex, textureIndex);
	glVertexAttribIPointer(3, 1, GL_INT, sizeof(QuadVertex), reinterpret_cast<void*>(tex_index_offset));
	glVertexAttribDivisor(3, 0);

	// depth attribute(location 4)
	glEnableVertexAttribArray(4);
	const ptrdiff_t depth_offset = offsetof(QuadVertex, depth);
	glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), reinterpret_cast<void*>(depth_offset));
	glVertexAttribDivisor(4, 0);

	// Unbind VAO and buffers
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// Enable blending for transparency
	// glEnable(GL_BLEND);
	// glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// glDisable(GL_DEPTH_TEST);
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
	glUniformMatrix3fv(shader.UniformLocations.at("uViewNDC"), 1, GL_FALSE, ndc_matrix.data());
	glUseProgram(0);

	startBatch();
}

void BatchRenderer2D::EndScene()
{
	flush();
}

namespace
{
	std::array<unsigned char, 4> pack_color(const std::span<const float, 4>& rgba)
	{
		unsigned char r = static_cast<unsigned char>(rgba[0] * 255.0f);
		unsigned char g = static_cast<unsigned char>(rgba[1] * 255.0f);
		unsigned char b = static_cast<unsigned char>(rgba[2] * 255.0f);
		unsigned char a = static_cast<unsigned char>(rgba[3] * 255.0f);
		return std::array<unsigned char, 4>{ r, g, b, a };
	}
}

void BatchRenderer2D::DrawQuad(std::span<const float, 9> transform,float depth, OpenGL::Handle texture, std::span<const float, 4> texture_coords_lbrt, std::span<const float, 4> tint_color)
{
	if (indexCount + 6 > maxIndices)
	{
		flush();
	}

	int	 tex_index = 0;
	bool found	   = false;
	for (size_t i = 0; i < activeTextureSize; ++i)
	{
		if (textureSlots[i] == texture)
		{
			found	  = true;
			tex_index = static_cast<int>(i);
		}
	}

	if (!found)
	{
		if (activeTextureSize >= textureSlots.size())
		{
			flush();
		}
		tex_index						= static_cast<int>(activeTextureSize);
		textureSlots[activeTextureSize] = texture;
		++activeTextureSize;
	}

	// Convert texture_coords_lbrt (left, bottom, right, top) to texture coordinate transform matrix
	const float left   = texture_coords_lbrt[0];
	const float bottom = texture_coords_lbrt[1];
	const float right  = texture_coords_lbrt[2];
	const float top	   = texture_coords_lbrt[3];

	const std::array<float, 2> texture_coords[4] = {
		{  left, bottom }, //  bottom left
		{ right, bottom }, //  bottom right
		{ right,	 top }, //  top right
		{  left,	top }  //  top left
	};

	// we don't have to make texcoord_transform matrix, just use 4 texture coords right away!

	const std::array<unsigned char, 4> tint = pack_color(tint_color);

	constexpr std::array<float, 2> model_positions[4] = {
		{ -0.5, -0.5 }, //  bottom left
		{ +0.5, -0.5 }, //  bottom right
		{ +0.5, +0.5 }, //  top right
		{ -0.5, +0.5 }	//  top left
	};


	for (unsigned i = 0; i < 4; ++i) // i is for 4 vertex(bottom/top - right/left)
	{
		// matrix multiply manually (3by 3, transform matrix) * (3 by 1, position matrix) => model to world!
		const float x = model_positions[i][0] /*bottom_left->left(x)*/ * transform[0] + model_positions[i][1] /*bottom_left->bottom(y)*/ * transform[3] + transform[6];
		const float y = model_positions[i][0] * transform[1] + model_positions[i][1] * transform[4] + transform[7];

		vertexDataEnd->x			= x;
		vertexDataEnd->y			= y;
		vertexDataEnd->s			= texture_coords[i][0];
		vertexDataEnd->t			= texture_coords[i][1];
		vertexDataEnd->tint			= tint;
		vertexDataEnd->textureIndex = tex_index;
		vertexDataEnd->depth		= depth;
		++vertexDataEnd;
	}
	indexCount += 6;
}

void BatchRenderer2D::startBatch()
{
	vertexDataEnd	  = vertexData.data();
	indexCount		  = 0;
	activeTextureSize = 0;
}

void BatchRenderer2D::flush()
{
	if (indexCount == 0)
		return;

	// upload our vertices(vertex buffer is dynamic)
	const auto vertex_count = vertexDataEnd - vertexData.data(); // pointer subtraction returns amount of element!
	const auto size_bytes	= static_cast<unsigned long>(vertex_count) * sizeof(QuadVertex);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(size_bytes), vertexData.data());

	// select our texture
	for (size_t i = 0; i < activeTextureSize; ++i)
	{
		glActiveTexture(static_cast<GLenum>(GL_TEXTURE0 + i));
		glBindTexture(GL_TEXTURE_2D, textureSlots[i]);
	}


	// draw
	glUseProgram(shader.Shader);
	glBindVertexArray(vertexArrayObject);
	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount), GL_UNSIGNED_INT, nullptr);

	// unbind stuff
	glBindVertexArray(0);
	glUseProgram(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	startBatch(); // reset
}