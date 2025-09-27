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
OpenGL::Handle         gDuckTexture  = 0;

void demo_setup()
{
    const std::filesystem::path vertex_file   = assets::locate_asset("Assets/shaders/basic.vert");
    const std::filesystem::path fragment_file = assets::locate_asset("Assets/shaders/basic.frag");
    gShader                                   = OpenGL::CreateShader(vertex_file, fragment_file);

    // vertex buffer that has 2D position adn some "texture coordinates"

    struct vertex
    {
        float x;
        float y;
        float s;
        float t;
    };

    // just quad
    const vertex vertices[] = {
        // with s,t(texture space, texture coordinate)
        {
         -0.5,
         -0.5,
         0, 0,
         }, //  bottom left
        {
         +0.5,
         -0.5,
         1, 0,
         }, //  bottom right
        {
         +0.5,
         +0.5,
         1, 1,
         }, //  top right
        {
         -0.5,
         +0.5,
         0, 1,
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

    // describes our rgb color
    glEnableVertexAttribArray(1);                                                                     // turn on location 1
    ptrdiff_t offset = 2 * sizeof(float);                                                             // because {x,y,*r*,g,b} -> need 2 offset!
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), reinterpret_cast<void*>(offset)); // now we need just 2
    glVertexAttribDivisor(1, 0);

    // un-select VAO&buffers, unbind
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0); //**already unbind??
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // texture create
    // load color values
    const std::filesystem::path image_path = assets::locate_asset("Assets/robot.png");

    // we may need to flip because of order of row of color
    const bool FLIP = true;
    stbi_set_flip_vertically_on_load(FLIP);

    int           w = 0, h = 0;
    constexpr int num_channels       = 4;                                                                                 // rgba
    int           files_num_channels = 0;                                                                                 // to here
    const auto    image_bytes        = stbi_load(image_path.string().c_str(), &w, &h, &files_num_channels, num_channels); // loading, use dynamic memory so we need free

    // copy the color values to the GPU as a texture


    glGenTextures(1, &gDuckTexture);
    glBindTexture(GL_TEXTURE_2D, gDuckTexture);

    // properties
    //- how it's filtered : go to nearest pixel / linear and blur
    //- how handle outside of 0..1 : wrapping / clamp
    //- mip mapping(useful to 3D graphics) : a way to have smaller versions of texture
    //-> if 3D is really far away.., and we just need color of it, we dont need high res source image
    // so we use low res image

    // filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // param2 : minifying the image filtering , param3 : we can gl_linear
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // param2 : magnifying the image filtering

    // wrapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // GL_CLAMP_TO_EDGE/GL_MIRRORED_REPEAT
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // GL_CLAMP_TO_EDGE/GL_MIRRORED_REPEAT

    // actually feed image_bytes to OpenGL
    constexpr int base_mipmap_level = 0; // just bare level, we don't care
    constexpr int zero_border       = 0;
    glTexImage2D(GL_TEXTURE_2D, base_mipmap_level, GL_RGBA8, w, h, zero_border, GL_RGBA, GL_UNSIGNED_BYTE, image_bytes); // feed
    // unload,free
    stbi_image_free(image_bytes);    // free first, cause we don't need it
    glBindTexture(GL_TEXTURE_2D, 0); // we got color from texture, so we don't need it anymore


    // and we can draw in draw func..
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

    const auto size =static_cast<float>( std::min(gWidth, gHeight));
    std::array<float, 9> model{ size, 0.0f, 0.0f, 0.0f, size, 0.0f, 0.0f, 0.0f, 1.0f };
    std::array<float, 9> texcoord_transform{ 63.f/315.f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };//to shrink tex_coord to just one frame
    glUniformMatrix3fv(gShader.UniformLocations.at("uToNDC"), 1, GL_FALSE, to_ndc.data()); // bind matrices first, ndc matrix has to be uniform because it doesn't change
    glUniformMatrix3fv(gShader.UniformLocations.at("uModel"), 1, GL_FALSE, model.data());  // bind matrices first, ndc matrix has to be uniform because it doesn't change
    glUniformMatrix3fv(gShader.UniformLocations.at("uTexCoordTransform"), 1, GL_FALSE, texcoord_transform.data());  // bind matrices first, ndc matrix has to be uniform because it doesn't change
    glUniform4f(gShader.UniformLocations.at("uTint"),1.0f, 0.0f, 0.0f, 1.0f);
    glBindVertexArray(gVertexArrayObject);
    //also use sampler, it is also uniform!!
    glActiveTexture(0); // which texture slot you want to use, 0 is first
    glBindTexture(GL_TEXTURE_2D, gDuckTexture);//now activated, so we put real texture
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
