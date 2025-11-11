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
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
}

void InstancedRenderer2D::Shutdown()
{
}

void InstancedRenderer2D::BeginScene([[maybe_unused]] std::span<const float, 9> ndc_matrix)
{
}

void InstancedRenderer2D::EndScene()
{
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
}

void InstancedRenderer2D::startBatch()
{
}

void InstancedRenderer2D::flush()
{
}