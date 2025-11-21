#pragma once

#include "Handle.hpp"
#include <GL/glew.h>
#include <array>
#include <memory>
#include <span>

class Shape
{
public:
	enum class PrimitivePattern : GLenum
	{
		Points		  = GL_POINTS,
		Lines		  = GL_LINES,
		LineStrip	  = GL_LINE_STRIP,
		LineLoop	  = GL_LINE_LOOP,
		Triangles	  = GL_TRIANGLES,
		TriangleStrip = GL_TRIANGLE_STRIP,
		TriangleFan	  = GL_TRIANGLE_FAN
	};

	struct Vertex
	{
		float						 x = 0, y = 0;
		float						 s = 0, t = 0;
		std::array<unsigned char, 4> tint{ 255, 255, 255, 255 };

		constexpr Vertex() noexcept = default;

		constexpr Vertex(float x_, float y_, float s_, float t_, std::array<unsigned char, 4> rgba) noexcept : x(x_), y(y_), s(s_), t(t_), tint(rgba)
		{
		}

		constexpr Vertex(float x_, float y_, std::array<unsigned char, 4> rgba) noexcept : Vertex(x_, y_, 0.0f, 0.0f, rgba)
		{
		}

		constexpr Vertex(float x_, float y_) noexcept : Vertex(x_, y_, 0.0f, 0.0f, { 255, 255, 255, 255 })
		{
		}

		constexpr Vertex(float x_, float y_, float s_, float t_) noexcept : Vertex(x_, y_, s_, t_, { 255, 255, 255, 255 })
		{
		}
	};

public:
	Shape(PrimitivePattern pattern, std::span<const Vertex> vertices);
	Shape(PrimitivePattern pattern, OpenGL::Handle texture, std::span<const Vertex> vertices);

	~Shape();
	Shape(const Shape&)			   = delete;
	Shape& operator=(const Shape&) = delete;
	Shape(Shape&& other) noexcept;
	Shape& operator=(Shape&& other) noexcept;

	void Draw() const noexcept;


private:
	OpenGL::Handle	 vertexBuffer	   = 0;
	OpenGL::Handle	 vertexArrayObject = 0;
	OpenGL::Handle	 textureHandle	   = 0;
	PrimitivePattern primitivePattern  = PrimitivePattern::Triangles;
	GLsizei			 vertexCount	   = 0;

private:
	void setup(PrimitivePattern pattern, OpenGL::Handle texture, std::span<const Vertex> vertices);
};
