#include "InstancedRenderer2D.hpp"
#include "Path.hpp"
#include <fstream>
#include <numeric>
#include <sstream>

InstancedRenderer2D::InstancedRenderer2D([[maybe_unused]] unsigned max_sprites)
{
	maxInstances = max_sprites;
	instanceData.reserve(maxInstances);
}

void InstancedRenderer2D::Init()
{
	// get max texture units
	// get glsl code and update the fragment shader
	// create the shader
	// set th binding values for textures array

	// get how many texture opengl can draw
	GLint max_tex_units = 0;
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_tex_units); // check with docs.gl to get minimum(16) and maximum
	textureSlots.resize(std::min(max_tex_units, 64));

	// load shaders with parsing
	const std::filesystem::path vertex_file = assets::locate_asset("Assets/shaders/instance.vert");
	std::ifstream				vert_stream(vertex_file);
	std::stringstream			vert_text_stream;
	vert_text_stream << vert_stream.rdbuf();
	const std::string vertex_glsl = vert_text_stream.str();


	const std::filesystem::path fragment_file = assets::locate_asset("Assets/shaders/instance.frag");
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

	// create our fixed buffer data
	// create index buffer data

	// create our instanced buffer
	// create VAO

	constexpr float fixed_sprite_vertices[][4] = {
		// bottom left
		{ -0.5f, -0.5f, 0.0f, 0.0f },
		// bottom right
		{  0.5f, -0.5f, 1.0f, 0.0f },
		// top right
		{  0.5f,	0.5f, 1.0f, 1.0f },
		// top left
		{ -0.5f,	 0.5f, 0.0f, 1.0f }
	};

	constexpr unsigned char indicies[] = { 0, 1, 2, 0, 2, 3 };

	glGenBuffers(1, &fixedVertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, fixedVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(fixed_sprite_vertices), fixed_sprite_vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicies), indicies, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glGenBuffers(1, &instanceBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(QuadInstance) * maxInstances, nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenVertexArrays(1, &vertexArrayObject);
	glBindVertexArray(vertexArrayObject);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer); // only one

	glBindBuffer(GL_ARRAY_BUFFER, fixedVertexBuffer); // can be many array_buffer, ex)instanceBuffer,, but calling with glbuffer with gl_array_buffer it replace it, not use both
	// so we need describe first than bind another array buffer

	/*===================== fixed static buffer, per vertex==============================*/
	//  Position attribute of fixedVertexBufferf - (location 0)
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, nullptr); // first two floats, and stride is [x,y,s,t]->4 floats
	glVertexAttribDivisor(0, 0);

	// Texture coordinate attribute of fixedVertexBuffer - (location 1)
	glEnableVertexAttribArray(1);
	const ptrdiff_t texcoord_offset = 2 * sizeof(float);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, reinterpret_cast<void*>(texcoord_offset));
	glVertexAttribDivisor(1, 0);

	/*===================== dynamic buffer, per instance==============================*/
	glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);

	// model row 0,1 of instanceBuffer - (location 2,3)
	glEnableVertexAttribArray(2);
	const ptrdiff_t modlerow0_offset = offsetof(QuadInstance, transformrow0);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(QuadInstance), reinterpret_cast<void*>(modlerow0_offset));
	glVertexAttribDivisor(
		2, 1); //(location,amount of instances flags - 0 means no instances, per vertex, 1 means this attributes loaded in for each instance,2 meand for every 2 instances, and so on...)

	glEnableVertexAttribArray(3);
	const ptrdiff_t modlerow1_offset = offsetof(QuadInstance, transformrow1);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(QuadInstance), reinterpret_cast<void*>(modlerow1_offset));
	glVertexAttribDivisor(3, 1);

	// Tint attribute of instanceBuffer (location 4)
	glEnableVertexAttribArray(4);
	const ptrdiff_t tint_offset = offsetof(QuadInstance, tint);
	glVertexAttribPointer(4, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(QuadInstance), reinterpret_cast<void*>(tint_offset));
	glVertexAttribDivisor(4, 1);

	// Texcoord sacle attribute of instanceBuffer (location 5)
	glEnableVertexAttribArray(5);
	const ptrdiff_t tex_scale_offset = offsetof(QuadInstance, texScale);
	glVertexAttribPointer(5, 2, GL_FLOAT, GL_FALSE, sizeof(QuadInstance), reinterpret_cast<void*>(tex_scale_offset));
	glVertexAttribDivisor(5, 1);

	// Texcoord offset attribute of instanceBuffer (location 6)
	glEnableVertexAttribArray(6);
	const ptrdiff_t tex_offset_offset = offsetof(QuadInstance, texOffset);
	glVertexAttribPointer(6, 2, GL_FLOAT, GL_FALSE, sizeof(QuadInstance), reinterpret_cast<void*>(tex_offset_offset));
	glVertexAttribDivisor(6, 1);

	// Texture index attribute of instanceBuffer (location 7)
	glEnableVertexAttribArray(7);
	const ptrdiff_t tex_index_offset = offsetof(QuadInstance, textureIndex);
	// just single int, not array so we use glVertexAttribIPointer instead to preserve intgers
	glVertexAttribIPointer(7, 1, GL_INT, sizeof(QuadInstance), reinterpret_cast<void*>(tex_index_offset));
	glVertexAttribDivisor(7, 1);

	// Unbind VAO and buffers
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void InstancedRenderer2D::Shutdown()
{
	glDeleteVertexArrays(1, &vertexArrayObject), vertexArrayObject = 0;
	glDeleteBuffers(1, &fixedVertexBuffer), fixedVertexBuffer	   = 0;
	glDeleteBuffers(1, &instanceBuffer), instanceBuffer			   = 0;
	glDeleteBuffers(1, &indexBuffer), indexBuffer				   = 0;
	glDeleteProgram(shader.Shader), shader.Shader				   = 0;
}

void InstancedRenderer2D::BeginScene([[maybe_unused]] std::span<const float, 9> ndc_matrix)
{
	glUseProgram(shader.Shader);
	glUniformMatrix3fv(shader.UniformLocations.at("uToNDC"),1, GL_FALSE, ndc_matrix.data());
	glUseProgram(0);

	startBatch();
}

void InstancedRenderer2D::EndScene()
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

void InstancedRenderer2D::DrawQuad(
	[[maybe_unused]] std::span<const float, 9> transform, [[maybe_unused]] OpenGL::Handle texture, [[maybe_unused]] std::span<const float, 4> texture_coords_lbrt,
	[[maybe_unused]] std::span<const float, 4> tint_color)
{
	if (instanceData.size() >= maxInstances)
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

	const float left   = texture_coords_lbrt[0];
	const float bottom = texture_coords_lbrt[1];
	const float right  = texture_coords_lbrt[2];
	const float top	   = texture_coords_lbrt[3];

	QuadInstance instance;

	instance.textureIndex = tex_index;

	instance.texScale[0] = right - left;
	instance.texScale[1] = top - bottom;
	instance.texOffset[0] = left;
	instance.texOffset[1] = bottom;

	instance.transformrow0[0] = transform[0];
	instance.transformrow0[1] = transform[3];
	instance.transformrow0[2] = transform[6];

	instance.transformrow1[0] = transform[1];
	instance.transformrow1[1] = transform[4];
	instance.transformrow1[2] = transform[7];

	instance.tint = pack_color(tint_color);

	instanceData.push_back(instance);

}

void InstancedRenderer2D::startBatch()
{
	instanceData.clear();
	activeTextureSize = 0;
}

void InstancedRenderer2D::flush()
{
	if (instanceData.empty()) [[unlikely]]
		return;

	//update the instance buffer data
	glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(QuadInstance) * instanceData.size(), instanceData.data());
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// select our texture
	for (size_t i = 0; i < activeTextureSize; ++i)
	{
		glActiveTexture(static_cast<GLenum>(GL_TEXTURE0 + i));
		glBindTexture(GL_TEXTURE_2D, textureSlots[i]);
	}

	glUseProgram(shader.Shader);
	glBindVertexArray(vertexArrayObject);
	glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr, static_cast<GLsizei>(instanceData.size()));


	glBindVertexArray(0);
	glUseProgram(0);

	startBatch();
}