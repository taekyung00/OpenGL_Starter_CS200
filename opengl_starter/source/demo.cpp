// completed_sdf_and_start_framebuffers
// make quad that fits to primitive(rect, circle, ...)
// and use another shader(SDF) to draw primitive directly
// and use transform matrix

#include "Path.hpp"
#include "Random.hpp"
#include "Shader.hpp"
#include <GL/glew.h>
#include <SDL.h>
#include <array> //feed array to vertex shader, and vertex shader do NDC
#include <imgui.h>
#include <stb_image.h>
#include <vector>

extern int gWidth;
extern int gHeight;

OpenGL::CompiledShader gShader;
// keep track of GPU resources by creating a handle
OpenGL::Handle         gVertexBuffer; // hold the unique set of vertices
OpenGL::Handle         gIndexBuffer;
// collect all together
OpenGL::Handle         gVertexArrayObject; // handle to our model data , everything that the model needs
GLsizei                gIndicesCount = 0;

// make helper func. to change scale and rot to fit primitives
float                gScaleX     = 128.f;
float                gScaleY     = 128.f;
float                gRotation   = 0.f;
float                gTranslateX = 0.f;
float                gTranslateY = 0.f;
int                  gShape      = 0; // 0 is circle, 1 is rect
float                gLineWidth  = 8.f;
std::array<float, 4> gFillColor  = { 1.f, 0.f, 0.f, 1.f };
std::array<float, 4> gLineColor  = { 0.f, 0.f, 0.f, 1.f };

void demo_setup()
{
    const std::filesystem::path vertex_file   = assets::locate_asset("Assets/shaders/sdf.vert");
    const std::filesystem::path fragment_file = assets::locate_asset("Assets/shaders/sdf.frag");
    gShader                                   = OpenGL::CreateShader(vertex_file, fragment_file);

    // vertex buffer that has 2D position adn some "texture coordinates"

    struct vertex
    {
        float x;
        float y;//shader does color things, so we don't need texture coord anymore
    };

    // just quad
    const vertex vertices[] = {
        // with s,t(texture space, texture coordinate)
        {
         -0.5,
         -0.5
         }, //  bottom left
        {
         +0.5,
         -0.5
         }, //  bottom right
        {
         +0.5,
         +0.5
         }, //  top right
        {
         -0.5,
         +0.5
         }  //  top left
    };

    // Triangle indices
    const unsigned short indices[] = { 0, 1, 2, 0, 2, 3 };
    gIndicesCount                  = static_cast<GLsizei>(std::ssize(indices)); // size for just size, ssize for signed size

    // buffer of vertex data
    glGenBuffers(1, &gVertexBuffer);              // generate buffers, set up to create many buffers all in one go, create a unique ID
    glBindBuffer(GL_ARRAY_BUFFER, gVertexBuffer); // feed this vertex buffer with my verticies data but before bind unique buffer, param : type of buffer, param : unique id
    glBufferData(
        GL_ARRAY_BUFFER, sizeof(vertices), vertices,
        GL_STATIC_DRAW);              // feed, param : type, param : size, param : real data(decay to pointer), param : static(not change)/dynamic(somtimes change) / stream(always change)
    glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind - good practice, 0 means nothing : no buffer

    // buffer of index data
    glGenBuffers(1, &gIndexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIndexBuffer); // opengl use word 'element' to specify indices
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // vertex array object - collection of the vertices and indices and description of how they're organized : model
    glGenVertexArrays(1, &gVertexArrayObject);
    glBindVertexArray(gVertexArrayObject);
    // associate our vertex&index buffers with this VAO
    glBindBuffer(GL_ARRAY_BUFFER, gVertexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIndexBuffer);

    // tell opengl how we organized the vertices for the vertex shader
    // to feed our data into the vertex shader
    // we have to attributes (location 0, location 1) -> we have to turn them on and describe them

    // describes our 2d position
    glEnableVertexAttribArray(0);                                             // turn on location 0
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), nullptr); // param : index(location), dimension(2d), type, should we normalize?(no, we hardcorded), stride: how many amount of jump
                                                                              // do we need to go next data, location of the very first bytes to be read : 0, but it takes void* so..
    glVertexAttribDivisor(0, 0); // called instancing..not now, param : index, how many instances of this mode need this value ; don't need this right now but use in assign


    // un-select VAO&buffers, unbind
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0); //**already unbind??
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

}
// Type aliases for OpenGL-compatible data formats
using mat3 = std::array<float, 9>; ///< 3x3 matrix in column-major order for OpenGL
using vec2 = std::array<float, 2>; ///< 2D vector for OpenGL uniform uploads

struct SDFTransform
{
    mat3 QuadTransform; ///< OpenGL transformation matrix for the rendering quad
    vec2 WorldSize;     ///< Original shape size in world coordinates
    vec2 QuadSize;      ///< Expanded quad size including line width padding
};

SDFTransform CalculateSDFTransform(const mat3& transform, float line_width) noexcept
{
    constexpr int  mat3_width = 3;
    constexpr auto mat3_index = [](int row, int col)
    {
        return col * mat3_width + row;
    };
    const auto  a = transform[mat3_index(0, 0)];
    const auto  b = transform[mat3_index(0, 1)];
    const auto  c = transform[mat3_index(1, 0)];
    const auto  d = transform[mat3_index(1, 1)];
    const vec2  world_size{ (std::sqrt(a * a + c * c)), (std::sqrt(b * b + d * d)) }; //rot and scale
    const float line_width_addition = std::max((line_width), 0.0f);
    const vec2  quad_size           = { world_size[0] + line_width_addition, world_size[1] + line_width_addition };

    const vec2 scale_up       = { quad_size[0] / world_size[0], quad_size[1] / world_size[1] }; //take the ratio of (world+line) / world
    mat3       quad_transform = transform;
    quad_transform[0] *= scale_up[0];
    quad_transform[1] *= scale_up[0];
    quad_transform[3] *= scale_up[1];
    quad_transform[4] *= scale_up[1];
    return { quad_transform, world_size, quad_size };
}

// Helper function to create a transformation matrix from scale, rotation, and translation
mat3 CreateTransformMatrix(float scale_x, float scale_y, float rotation_degrees, float translate_x, float translate_y)
{
    const float rad   = rotation_degrees * 3.14159265f / 180.0f;
    const float cos_r = std::cos(rad);
    const float sin_r = std::sin(rad);

    // Create combined scale, rotation, and translation matrix
    // Column-major order for OpenGL
    return mat3{
        scale_x * cos_r,  // m00
        scale_x * sin_r,  // m10
        0.0f,             // m20
        -scale_y * sin_r, // m01
        scale_y * cos_r,  // m11
        0.0f,             // m21
        translate_x,      // m02
        translate_y,      // m12
        1.0f              // m22
    };
}

void demo_draw()
{
    glClearColor(0.34f, 0.56f, 0.9f, 1.0f); // just 'set' window color
    glClear(GL_COLOR_BUFFER_BIT);           // actually clear

    glUseProgram(gShader.Shader);

    std::array<float, 9> to_ndc{
        2.0f / static_cast<float>(gWidth),
        0.0f,
        0.0f, // column 0
        0.0f,
        2.0f / static_cast<float>(gHeight),
        0.0f, // column 1
        0.0f,
        0.0f,
        1.0f // column 2
    };

    const auto           size = static_cast<float>(std::min(gWidth, gHeight));
    std::array<float, 9> model = CreateTransformMatrix(gScaleX,gScaleY,gRotation, gTranslateX,gTranslateY);

    const auto sdf_transform = CalculateSDFTransform(model,gLineWidth);

    //vertex shader
    glUniformMatrix3fv(gShader.UniformLocations.at("uToNDC"), 1, GL_FALSE, to_ndc.data());                      
    glUniformMatrix3fv(gShader.UniformLocations.at("uModel"), 1, GL_FALSE, sdf_transform.QuadTransform.data()); // use modified one!!            
    //let shader know width and height of quad, and we will make bottom-left (-w/2, -h/2), t-r(w/2, h/2)
    glUniform2f(gShader.UniformLocations.at("uSDFScale"), sdf_transform.QuadSize[0], sdf_transform.QuadSize[1]); //multiply this to position(-0.5 ~ 0.5 already)

    //frag shader(feed all things to shader)
    glUniform4fv(gShader.UniformLocations.at("uFillColor"), 1, gFillColor.data());
    glUniform4fv(gShader.UniformLocations.at("uLineColor"), 1, gLineColor.data());
    glUniform2fv(gShader.UniformLocations.at("uWorldSize"), 1, sdf_transform.WorldSize.data());
    glUniform1f(gShader.UniformLocations.at("uLineWidth"), gLineWidth);
    glUniform1i(gShader.UniformLocations.at("uShape"), gShape);

    // glUniformMatrix3fv(gShader.UniformLocations.at("uTexCoordTransform"), 1, GL_FALSE, texcoord_transform.data()); 
    // glUniform4f(gShader.UniformLocations.at("uTint"), 1.0f, 0.0f, 0.0f, 1.0f);
    glBindVertexArray(gVertexArrayObject);

    glDrawElements(GL_TRIANGLES, gIndicesCount, GL_UNSIGNED_SHORT, nullptr);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

void demo_imgui()
{
    ImGui::Begin("Demo Settings");
    ImGui::End();
}
