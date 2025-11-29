#include "Path.hpp"
#include "Random.hpp"
#include "Shader.hpp"
#include "Shape.hpp"
#include <GL/glew.h>
#include <SDL.h>
#include <array>
#include <imgui.h>
#include <numbers>
#include <stb_image.h>
#include <vector>

extern int gWidth;
extern int gHeight;

struct Object
{
	Shape theShape;
	float translateX;
	float translateY;
	float scale;
};

OpenGL::Handle		   gTextureHandle = 0;
float				   gRotationAngle = 0.0f;
OpenGL::CompiledShader gShader{};
std::vector<Object>	   gShapes;

void create_all_shapes();

void demo_setup()
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);

	const std::filesystem::path vertex_file	  = assets::locate_asset("Assets/shaders/shape.vert");
	const std::filesystem::path fragment_file = assets::locate_asset("Assets/shaders/shape.frag");
	gShader									  = OpenGL::CreateShader(vertex_file, fragment_file);

	const std::filesystem::path image_path = assets::locate_asset("Assets/Robot.png");
	stbi_set_flip_vertically_on_load(true);
	int		   w = 0, h = 0;
	const int  num_channels		  = 4;
	int		   files_num_channels = 0;
	const auto image_bytes		  = stbi_load(image_path.string().c_str(), &w, &h, &files_num_channels, num_channels);

	glGenTextures(1, &gTextureHandle);
	glBindTexture(GL_TEXTURE_2D, gTextureHandle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_bytes);
	stbi_image_free(image_bytes);
	glBindTexture(GL_TEXTURE_2D, 0);

	create_all_shapes();
}

void demo_draw()
{
	glClearColor(0.34f * 0.5f, 0.56f * 0.5f, 0.9f * 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	std::array<float, 9> to_ndc{ 2.0f / static_cast<float>(gWidth), 0.0f, 0.0f, 0.0f, 2.0f / static_cast<float>(gHeight), 0.0f, 0.0f, 0.0f, 1.0f };

	glUseProgram(gShader.Shader);
	glUniformMatrix3fv(gShader.UniformLocations.at("uToNDC"), 1, GL_FALSE, to_ndc.data());
	for (const auto& shape_object : gShapes)
	{
		const float		 rad	   = gRotationAngle;
		const float		 c		   = std::cos(rad);
		const float		 s		   = std::sin(rad);
		const std::array transform = { shape_object.scale * c,	shape_object.scale * s,	 0.0f, -shape_object.scale * s, shape_object.scale * c, 0.0f,
									   shape_object.translateX, shape_object.translateY, 1.0f };
		glUniformMatrix3fv(gShader.UniformLocations.at("uTransform"), 1, GL_FALSE, transform.data());
		shape_object.theShape.Draw();
	}
	glUseProgram(0);
}

void demo_shutdown()
{
	glDeleteTextures(1, &gTextureHandle);
	glDeleteProgram(gShader.Shader);
	gShapes.clear();
}

void demo_imgui()
{
	ImGui::Begin("Demo Settings");
	ImGui::SliderAngle("Rotation", &gRotationAngle);
	ImGui::End();
}

void create_all_shapes()
{
	gShapes.clear();

	// points
	{
		std::vector<Shape::Vertex> vertices;
		constexpr size_t		   count = 100;
		vertices.reserve(count);
		for (size_t i = 0; i < count; ++i)
		{
			const float x = util::random(-0.5f, 0.5f);
			const float y = util::random(-0.5f, 0.5f);
			const auto	r = static_cast<unsigned char>(util::random(128, 255));
			const auto	g = static_cast<unsigned char>(util::random(128, 255));
			const auto	b = static_cast<unsigned char>(util::random(128, 255));
			vertices.emplace_back(x, y, std::array<unsigned char, 4>{ r, g, b, 255 }); // emplace_back - calling a constructor
		}
		gShapes.emplace_back(Shape{ Shape::PrimitivePattern::Points, vertices }, -300.f, 250.f, 150.f); // center is 0,0
	}

	// lines - grid pattern
	{
		std::vector<Shape::Vertex>			   vertices;
		constexpr size_t					   num_lines = 10;
		constexpr std::array<unsigned char, 4> color	 = { 76, 204, 230, 225 };

		// vertical lines
		for (size_t i = 0; i <= num_lines; ++i)
		{
			constexpr float y = 0.5f;
			const float		x = -0.5f + i * (1.f / num_lines);
			vertices.emplace_back(x, y, color);
			vertices.emplace_back(x, -y, color);
		}
		// horizontal lines
		for (size_t i = 0; i <= num_lines; ++i)
		{
			constexpr float x = 0.5f;
			const float		y = -0.5f + i * (1.f / num_lines);
			vertices.emplace_back(x, y, color);
			vertices.emplace_back(-x, y, color);
		}
		gShapes.emplace_back(Shape{ Shape::PrimitivePattern::Lines, vertices }, 0.f, 250.f, 150.f);
	}

	// line strip - sine wave
	{
		std::vector<Shape::Vertex> vertices;
		constexpr size_t		   num_points = 100;
		for (size_t i = 0; i < num_points; ++i)
		{
			const auto	t = i / static_cast<float>(num_points);
			const float x = -0.5f + t;
			const float y = std::sin(t * 6.0f * std::numbers::pi_v<float>) * 0.3f;
			const auto	r = static_cast<unsigned char>(128 + 127 * t);
			const auto	g = static_cast<unsigned char>(225 - 127 * t);
			vertices.emplace_back(x, y, std::array<unsigned char, 4>{ r, g, 204, 255 });
		}
		gShapes.emplace_back(Shape{ Shape::PrimitivePattern::LineStrip, vertices }, 300.f, 250.f, 200.f);
	}
	// line loop - star shape
	{
		std::vector<Shape::Vertex>			   vertices;
		constexpr size_t					   num_points	= 5;
		constexpr float						   outer_radius = 0.5f;
		constexpr float						   inner_radius = outer_radius / 3.f;
		constexpr std::array<unsigned char, 4> color		= { 225, 204, 51, 225 };
		for (size_t i = 0; i < num_points * 2; ++i)
		{
			const float angle  = i / static_cast<float>(num_points) * std::numbers::pi_v<float>; // 0 - 2pi
			const float radius = (i & 1) ? inner_radius : outer_radius;
			const float x	   = std::cos(angle) * radius;
			const float y	   = std::sin(angle) * radius;
			vertices.emplace_back(x, y, color);
		}
		gShapes.emplace_back(Shape{ Shape::PrimitivePattern::LineLoop, vertices }, -300.f, 0.f, 150.f);
	}
	// Triangles - Separate colored triangles
	{
		std::vector<Shape::Vertex>             vertices;
        constexpr std::array<unsigned char, 4> red{ 255, 0, 0, 255 };
        constexpr std::array<unsigned char, 4> green{ 0, 255, 0, 255 };
        constexpr std::array<unsigned char, 4> blue{ 0, 0, 255, 255 };
        // Triangle 1 - Red
        vertices.emplace_back(-50.0f, -40.0f, 0.0f, 0.0f, red);
        vertices.emplace_back(0.0f, 40.0f, 0.0f, 0.0f, red);
        vertices.emplace_back(-100.0f, 40.0f, 0.0f, 0.0f, red);
        // Triangle 2 - Green
        vertices.emplace_back(0.0f, -40.0f, 0.0f, 0.0f, green);
        vertices.emplace_back(50.0f, 40.0f, 0.0f, 0.0f, green);
        vertices.emplace_back(-50.0f, 40.0f, 0.0f, 0.0f, green);
        // Triangle 3 - Blue
        vertices.emplace_back(50.0f, -40.0f, 0.0f, 0.0f, blue);
        vertices.emplace_back(100.0f, 40.0f, 0.0f, 0.0f, blue);
        vertices.emplace_back(0.0f, 40.0f, 0.0f, 0.0f, blue);

        gShapes.push_back({ Shape(Shape::PrimitivePattern::Triangles, vertices), 0.0f, -50.0f, 1.0f });
	}

	// TriangleStrip - Ribbon/banner
	{
		std::vector<Shape::Vertex> vertices;
		const int				   numSegments = 20;
		const float				   width	   = 200.0f;
		const float				   height	   = 40.0f;

		for (int i = 0; i <= numSegments; ++i)
		{
			const float t	 = static_cast<float>(i) / static_cast<float>(numSegments);
			const float x	 = -width / 2.0f + t * width;
			const float wave = std::sin(t * 4.0f * std::numbers::pi_v<float>) * 10.0f;

			const auto r = static_cast<unsigned char>(204);
			const auto g = static_cast<unsigned char>(76 + 102 * t);
			const auto b = static_cast<unsigned char>(230 - 102 * t);

			vertices.emplace_back(x, -height / 2.0f + wave, 0.0f, 0.0f, std::array<unsigned char, 4>{ r, g, b, 255 });
			vertices.emplace_back(x, height / 2.0f + wave, 0.0f, 0.0f, std::array<unsigned char, 4>{ r, g, b, 255 });
		}
		gShapes.emplace_back(Shape(Shape::PrimitivePattern::TriangleStrip, vertices), 300.0f, -50.0f, 1.0f);
	}

	// TriangleFan - Textured circle
	{
		std::vector<Shape::Vertex> vertices;
		const int				   numSegments = 32;
		const float				   radius	   = 80.0f;

		// Center vertex with texture center
		vertices.emplace_back(0.0f, 0.0f, 0.5f, 0.5f);

		// Outer vertices
		for (int i = 0; i <= numSegments; ++i)
		{
			const float angle = static_cast<float>(i) * 2.0f * std::numbers::pi_v<float> / static_cast<float>(numSegments);
			const float x	  = std::cos(angle) * radius;
			const float y	  = std::sin(angle) * radius;
			const float s	  = 0.5f + (std::cos(angle)) * 1.0f / 6.0f;
			const float t	  = 0.5f + (std::sin(angle)) * 0.5f;
			vertices.emplace_back(x, y, s, t);
		}

		gShapes.emplace_back(Shape(Shape::PrimitivePattern::TriangleFan, gTextureHandle, vertices), -300.0f, -300.0f, 1.0f);
	}
}
