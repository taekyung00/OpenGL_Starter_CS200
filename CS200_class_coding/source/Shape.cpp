#include "Shape.hpp"

namespace
{
    OpenGL::Handle WhiteTexture() // just default texture
    {
        static OpenGL::Handle white_texture = []()
        {
            GLuint default_white_texture;
            glGenTextures(1, &default_white_texture);
            glBindTexture(GL_TEXTURE_2D, default_white_texture);

            constexpr unsigned char white_pixel[4] = { 255, 255, 255, 255 };
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white_pixel);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

            glBindTexture(GL_TEXTURE_2D, 0);
            return default_white_texture;
        }();//putting curly brace at the end of lambda -> immediately invoking lambda
        //because white_texture is static, lambda is needed to be invoked only one time!
        //once we make static variable, it stays in stack until program is ended
        //so when we call WhiteTexture() once again, it just return white_texture
        return white_texture;
    }
}

Shape::Shape(PrimitivePattern pattern, std::span<const Vertex> vertices)
{
    setup(pattern, WhiteTexture(), vertices);
}

Shape::Shape(PrimitivePattern pattern, OpenGL::Handle texture, std::span<const Vertex> vertices)
{
    setup(pattern, texture, vertices);
}

Shape::~Shape()
{
    glDeleteVertexArrays(1, &vertexArrayObject);
    vertexArrayObject = 0;

    glDeleteBuffers(1, &vertexBuffer);
    vertexBuffer = 0;
}

Shape::Shape(Shape&& other) noexcept
    : vertexBuffer(other.vertexBuffer), vertexArrayObject(other.vertexArrayObject), textureHandle(other.textureHandle), primitivePattern(other.primitivePattern), vertexCount(other.vertexCount)
{
    other.vertexBuffer      = 0;
    other.vertexArrayObject = 0;
    other.textureHandle     = 0;
    other.vertexCount       = 0;
}

Shape& Shape::operator=(Shape&& other) noexcept
{
    std::swap(vertexBuffer, other.vertexBuffer);
    std::swap(vertexArrayObject, other.vertexArrayObject);
    std::swap(textureHandle, other.textureHandle);
    std::swap(primitivePattern, other.primitivePattern);
    std::swap(vertexCount, other.vertexCount);
    return *this;
}

void Shape::Draw() const noexcept
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureHandle);

    glBindVertexArray(vertexArrayObject);
    glDrawArrays(static_cast<GLenum>(primitivePattern), 0, vertexCount);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Shape::setup(PrimitivePattern pattern, OpenGL::Handle texture, std::span<const Vertex> vertices)
{
    primitivePattern = pattern;
    textureHandle    = texture;
    vertexCount      = static_cast<GLsizei>(vertices.size());

    // Create vertex buffer
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(Vertex) * vertices.size()), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //no index buffer this time
    
    // Create vertex array object
    glGenVertexArrays(1, &vertexArrayObject);
    glBindVertexArray(vertexArrayObject);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

    // Position attribute (location 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);

    // Texture coordinate attribute (location 1)
    glEnableVertexAttribArray(1);
    const ptrdiff_t texcoord_offset = 2 * sizeof(float);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(texcoord_offset));

    // Tint color attribute (location 2)
    glEnableVertexAttribArray(2);
    const ptrdiff_t tint_offset = offsetof(Vertex, tint);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), reinterpret_cast<void*>(tint_offset));

    // Unbind VAO and buffer
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
