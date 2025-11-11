/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include "IRenderer2D.hpp"
#include "Shader.hpp"
#include <array>
#include <vector>

/**
 *
 * one model,
 lots of instances
 sharing buffer static -> positions(because we use single quad!!)

 and each instance has their own dynamic buffer
 ->color
 ->texture
 ->model xform
 ->texcoords xform
 */

class InstancedRenderer2D : public IRenderer2D
{
public:
	InstancedRenderer2D(unsigned max_sprites = 10'000); // means max_instances

	void Init() override;
	void Shutdown() override;
	void BeginScene(std::span<const float, 9> ndc_matrix) override;
	void EndScene() override;
	void DrawQuad(std::span<const float, 9> transform, OpenGL::Handle texture, std::span<const float, 4> texture_coords_lbrt, std::span<const float, 4> tint_color) override;

private:
	struct QuadInstance //maybe we can make more compact? bit width, ...
	{
		/*float						 x = 0, y = 0;*/   // don't need for each instance anymore!!
		float						 trasnformrow0[3]; // instead having vertex for each instance, we have transform mat for each instance!
		float						 trasnformrow1[3];
		/*float						 s = 0, t = 0;*/ // don't need for each instance anymore!!
		float						 texScale[2];	 // instead having texcoord for each instance, we have transform mat of texcoord for each instance with compacted version
		float						 texOffset[2];
		std::array<unsigned char, 4> tint{};
		int							 textureIndex = 0;
	};

	std::vector<QuadInstance> instanceData{};
	OpenGL::CompiledShader	  shader;
	OpenGL::Handle			  fixedVertexBuffer = 0, instanceBuffer = 0, indexBuffer = 0, vertexArrayObject = 0;


	unsigned maxInstances = 0;

	unsigned instanceCount = 0; // just counting instanceCount instead using dataEnd

	// OpenGL::Handle theTexture = 0;
	std::vector<OpenGL::Handle> textureSlots;
	size_t						activeTextureSize = 0;

private:
	void flush(); // when quad amount is reached to max_quad
	void startBatch();
};
