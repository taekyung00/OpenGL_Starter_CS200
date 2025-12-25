#include "BatchRenderer2D.hpp"
#include "FPS.hpp"
#include "ImmediateRenderer2D.hpp"
#include "InstancedRenderer2D.hpp"
#include "Path.hpp"
#include "Random.hpp"
#include <GL/glew.h>
#include <SDL.h>
#include <array>
#include <imgui.h>
#include <memory>
#include <stb_image.h>
#include <vector>
#include <random>
#include <algorithm>

// Request high-performance GPU on systems with multiple GPUs (laptops with integrated + discrete)
// https://docs.nvidia.com/gameworks/content/technologies/desktop/optimus.htm
// https://gpuopen.com/learn/amdpowerxpressrequesthighperformance/
// https://gcc.gnu.org/wiki/Visibility

#if defined _WIN32 || defined __CYGWIN__
// Windows: Use __declspec(dllexport)
#	define GPU_EXPORT __declspec(dllexport)
#else
// Linux/Mac: Use visibility attribute
#	define GPU_EXPORT __attribute__((visibility("default")))
#endif

extern "C"
{
// NVIDIA Optimus: Request high-performance GPU
GPU_EXPORT unsigned long NvOptimusEnablement = 0x00000001;

// AMD PowerXpress: Request high-performance GPU
GPU_EXPORT int AmdPowerXpressRequestHighPerformance = 0x00000001;
}

// Simple vector types
struct vec2
{
	double x, y;
};

struct ivec2
{
	int x, y;
};

struct Camera
{
	// right, up, position
	vec2  right{ 1.0, 0.0 };
	vec2  up{ 0.0, 1.0 };
	// use angle to regenerate right and up vectors
	float angle = 0.f; // in radians
	vec2  position{ 0.0, 0.0 };
	bool  FirstPersonView = true;
} gCamera{};

struct Viewport
{
	int x, y;
	int width, height;
} gViewport1{ 0, 0, 800, 600 }/*, gViewport2{ 0, 300, 800, 300 }*/;

float gZoom = 1.0f;

extern int gWidth;
extern int gHeight;

// Robot sprite sheet constants
static constexpr ivec2 ROBOT_FRAME_SIZE{ 63, 127 };
static constexpr int   ROBOT_NUM_FRAMES = 5;
static constexpr int   ROBOT_VARIATIONS = 64;

// Robot instance data
struct Robot
{
	vec2  position;
	int	  frame;
	float depth = 0.f;
	float r, g, b; // tint color
	int	  variation;
};

std::vector<Robot>							 gRobots;
std::unique_ptr<IRenderer2D>				 gRenderer;
// OpenGL::Handle				 gRobotTexture = 0;
std::array<OpenGL::Handle, ROBOT_VARIATIONS> gRobotTextures{};
util::FPS									 gFPSTracker;
Uint32										 gLastTicks		  = 0;
bool										 gVSyncEnabled	  = true;
const char*									 gOpenGLRenderer  = nullptr;
int											 gMaxTextureUnits = 0;

enum class RendererType
{
	Immediate,
	Batch,
	Instanced
};
RendererType gCurrentRenderer = RendererType::Immediate;

// Forward declarations
Robot CreateRandomRobot();
void  SwitchRenderer(RendererType type);

void demo_setup()
{
	// Cache OpenGL renderer info
	gOpenGLRenderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &gMaxTextureUnits);

	// Initialize renderer
	gRenderer = std::make_unique<ImmediateRenderer2D>();
	gRenderer->Init();

	glGenTextures(ROBOT_VARIATIONS, gRobotTextures.data());
	for (int i = 0; i < ROBOT_VARIATIONS; ++i)
	{
		// Load robot texture
		std::ostringstream sout;
		sout << "Assets/variations/robot_var_" << std::setfill('0') << std::setw(2) << (i + 1) << ".png";
		const std::filesystem::path image_path = assets::locate_asset(sout.str());

		const bool FLIP = true;
		stbi_set_flip_vertically_on_load(FLIP);
		int		   w = 0, h = 0;
		const int  num_channels		  = 4;
		int		   files_num_channels = 0;
		const auto image_bytes		  = stbi_load(image_path.string().c_str(), &w, &h, &files_num_channels, num_channels);


		glBindTexture(GL_TEXTURE_2D, gRobotTextures[static_cast<size_t>(i)]);

		// Texture filtering
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // GL_LINEAR
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		// Texture wrapping
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // GL_CLAMP_TO_EDGE, GL_MIRRORED_REPEAT
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		constexpr int base_mipmap_level = 0;
		constexpr int zero_border		= 0;
		glTexImage2D(GL_TEXTURE_2D, base_mipmap_level, GL_RGBA8, w, h, zero_border, GL_RGBA, GL_UNSIGNED_BYTE, image_bytes);
		stbi_image_free(image_bytes);

		glBindTexture(GL_TEXTURE_2D, 0);
	}
	// Create random robots
	constexpr int NUM_ROBOTS = 20;
	gRobots.reserve(NUM_ROBOTS);
	for (int i = 0; i < NUM_ROBOTS; ++i)
	{
		gRobots.push_back(CreateRandomRobot());
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glDisable(GL_DEPTH_TEST);
	glEnable(GL_DEPTH_TEST);

	// Initialize VSync (adaptive vsync with fallback to regular vsync)
	// https://wiki.libsdl.org/SDL_GL_SetSwapInterval
	constexpr int ADAPTIVE_VSYNC = -1;
	constexpr int VSYNC			 = 1;
	if (const auto result = SDL_GL_SetSwapInterval(ADAPTIVE_VSYNC); result != 0)
	{
		SDL_GL_SetSwapInterval(VSYNC);
	}

	// Initialize FPS tracking
	gLastTicks = SDL_GetTicks();
}

void demo_draw()
{
	// Update FPS tracker
	const Uint32 currentTicks = SDL_GetTicks();
	const Uint32 deltaTicks	  = currentTicks - gLastTicks;
	const double deltaSeconds = deltaTicks / 1000.0;
	gLastTicks				  = currentTicks;
	gFPSTracker.Update(deltaSeconds);

	glClearColor(0.34f, 0.56f, 0.9f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear depth buffer too as 1.0
	const float right_x = (gCamera.FirstPersonView) ? static_cast<float>(gCamera.right.x) : 1.f;
	const float right_y = (gCamera.FirstPersonView) ? static_cast<float>(gCamera.right.y) : 0.f;
	const float up_x	= (gCamera.FirstPersonView) ? static_cast<float>(gCamera.up.x) : 0.f;
	const float up_y	= (gCamera.FirstPersonView) ? static_cast<float>(gCamera.up.y) : 1.f;
	const float view_tx = static_cast<float>(-(right_x * gCamera.position.x + right_y * gCamera.position.y)); // view translation x, -(right * position)
	const float view_ty = static_cast<float>(-(up_x * gCamera.position.x + up_y * gCamera.position.y));		  // view translation x, -(up * position)

	glViewport(gViewport1.x, gViewport1.y, gViewport1.width, gViewport1.height);

	// Create NDC transform matrix
	// std::array<float, 9> to_ndc{
	//	2.0f / static_cast<float>(gWidth),
	//	0.0f,
	//	0.0f, // column 0
	//	0.0f,
	//	2.0f / static_cast<float>(gHeight),
	//	0.0f, // column 1
	//	0.0f,
	//	0.0f,
	//	1.0f // column 2
	//};
	// if bottom left is 0,0 - translation position + 0.5 * (c_w,c_h)
	const float ndc1_scale_x = 2.f / (static_cast<float>(gViewport1.width) * gZoom);
	const float ndc1_scale_y = 2.f / (static_cast<float>(gViewport1.height) * gZoom); // make ndc scale depend on viewport size
	/*=============================================================================*/
	// but for me, it's better to apply zoom not here, but in right_x, right_y, up_x, up_y, view_tx, view_ty and don't think as denominate by zoom later.
	/*=============================================================================*/
	// for manually make view_ndc matrix

	// so far we just apply NDC transform, but we could add camera transform here
	// so far we treated camera as identity matrix
	// we will apply these two matrix -> to_ndc * camera globally to every drawing call
	// so camera matrix/NDC matrix are uniform matrix, whereas model_matrix is per-object matrix

	// M_{view_ndc} = NDC * View
	//  gl_Position = to_ndc * camera * model_matrix * vec3(position, 0.0, 1.0);


	std::array<float, 9> view_ndc1{
		ndc1_scale_x * right_x, ndc1_scale_y * up_x,	  0.f, // column 1
		ndc1_scale_x * right_y, ndc1_scale_y * up_y,	  0.f, // column 2
		ndc1_scale_x * view_tx, ndc1_scale_y * view_ty, 1.f, // column 3
	};

	gRenderer->BeginScene(view_ndc1);

	// Draw each robot
	for (const auto& robot : gRobots)
	{
		// Create transform matrix (scale by size and translate to position)
		const float width  = static_cast<float>(ROBOT_FRAME_SIZE.x);
		const float height = static_cast<float>(ROBOT_FRAME_SIZE.y);
		const float pos_x  = static_cast<float>(robot.position.x);
		const float pos_y  = static_cast<float>(robot.position.y);

		std::array<float, 9> transform{
			width, 0.0f,   0.0f, // column 0: scale X
			0.0f,  height, 0.0f, // column 1: scale Y
			pos_x, pos_y,  1.0f	 // column 2: translation
		};

		// Texture coordinates for sprite frame selection (left, bottom, right, top)
		// The sprite sheet is laid out horizontally with 5 frames
		const float frame_width = 1.0f / static_cast<float>(ROBOT_NUM_FRAMES);
		const float left		= frame_width * static_cast<float>(robot.frame);
		const float right		= left + frame_width;
		const float bottom		= 0.0f;
		const float top			= 1.0f;

		std::array<float, 4> texture_coords{ left, bottom, right, top };

		// Tint color
		std::array<float, 4> tint{ robot.r, robot.g, robot.b, 1.0f };

		gRenderer->DrawQuad(transform,robot.depth ,static_cast<OpenGL::Handle>(robot.variation), texture_coords, tint);
	}

	gRenderer->EndScene();

	///*====================================viewport2================================================*/

	//glViewport(gViewport2.x, gViewport2.y, gViewport2.width, gViewport2.height);

	//// Create NDC transform matrix
	//// std::array<float, 9> to_ndc{
	////	2.0f / static_cast<float>(gWidth),
	////	0.0f,
	////	0.0f, // column 0
	////	0.0f,
	////	2.0f / static_cast<float>(gHeight),
	////	0.0f, // column 1
	////	0.0f,
	////	0.0f,
	////	1.0f // column 2
	////};
	//// if bottom left is 0,0 - translation position + 0.5 * (c_w,c_h)
	//const float ndc2_scale_x = 2.f / (static_cast<float>(gViewport2.width) * gZoom);
	//const float ndc2_scale_y = 2.f / (static_cast<float>(gViewport2.height) * gZoom); // make ndc scale depend on viewport size
	///*=============================================================================*/
	//// but for me, it's better to apply zoom not here, but in right_x, right_y, up_x, up_y, view_tx, view_ty and don't think as denominate by zoom later.
	///*=============================================================================*/
	//// for manually make view_ndc matrix

	//// so far we just apply NDC transform, but we could add camera transform here
	//// so far we treated camera as identity matrix
	//// we will apply these two matrix -> to_ndc * camera globally to every drawing call
	//// so camera matrix/NDC matrix are uniform matrix, whereas model_matrix is per-object matrix

	//// M_{view_ndc} = NDC * View
	////  gl_Position = to_ndc * camera * model_matrix * vec3(position, 0.0, 1.0);


	//std::array<float, 9> view_ndc2{
	//	ndc2_scale_x * right_x, ndc2_scale_y * up_x,	  0.f, // column 1
	//	ndc2_scale_x * right_y, ndc2_scale_y * up_y,	  0.f, // column 2
	//	ndc2_scale_x * view_tx, ndc2_scale_y * view_ty, 1.f, // column 3
	//};

	//gRenderer->BeginScene(view_ndc2);

	//// Draw each robot
	//for (const auto& robot : gRobots)
	//{
	//	// Create transform matrix (scale by size and translate to position)
	//	const float width  = static_cast<float>(ROBOT_FRAME_SIZE.x);
	//	const float height = static_cast<float>(ROBOT_FRAME_SIZE.y);
	//	const float pos_x  = static_cast<float>(robot.position.x);
	//	const float pos_y  = static_cast<float>(robot.position.y);

	//	std::array<float, 9> transform{
	//		width, 0.0f,   0.0f, // column 0: scale X
	//		0.0f,  height, 0.0f, // column 1: scale Y
	//		pos_x, pos_y,  1.0f	 // column 2: translation
	//	};

	//	// Texture coordinates for sprite frame selection (left, bottom, right, top)
	//	// The sprite sheet is laid out horizontally with 5 frames
	//	const float frame_width = 1.0f / static_cast<float>(ROBOT_NUM_FRAMES);
	//	const float left		= frame_width * static_cast<float>(robot.frame);
	//	const float right		= left + frame_width;
	//	const float bottom		= 0.0f;
	//	const float top			= 1.0f;

	//	std::array<float, 4> texture_coords{ left, bottom, right, top };

	//	// Tint color
	//	std::array<float, 4> tint{ robot.r, robot.g, robot.b, 1.0f };

	//	gRenderer->DrawQuad(transform, static_cast<OpenGL::Handle>(robot.variation), texture_coords, tint);
	//}

	//gRenderer->EndScene();
	////===================================viewport2================================================*/
}

void demo_shutdown()
{
	glDeleteTextures(ROBOT_VARIATIONS, gRobotTextures.data());
	gRenderer->Shutdown();
	gRenderer.reset();
}

void demo_imgui()
{
	ImGui::Begin("Demo Settings");

	// Display FPS at the top
	ImGui::Text("FPS: %d", static_cast<int>(gFPSTracker));
	ImGui::Separator();

	// Display OpenGL renderer info
	if (gOpenGLRenderer)
	{
		ImGui::Text("OpenGL Renderer: %s", gOpenGLRenderer);
		ImGui::Text("Max Texture Units: %d", gMaxTextureUnits);
		ImGui::Separator();
	}

	// Renderer selection
	ImGui::Text("Renderer:");
	if (ImGui::RadioButton("Immediate", gCurrentRenderer == RendererType::Immediate))
	{
		SwitchRenderer(RendererType::Immediate);
	}
	ImGui::SameLine();

	if (ImGui::RadioButton("Batch", gCurrentRenderer == RendererType::Batch))
	{
		SwitchRenderer(RendererType::Batch);
	}

	ImGui::SameLine();
	if (ImGui::RadioButton("Instanced", gCurrentRenderer == RendererType::Instanced))
	{
		SwitchRenderer(RendererType::Instanced);
	}
	ImGui::Separator();

	// VSync toggle
	if (ImGui::Checkbox("VSync", &gVSyncEnabled))
	{
		// https://wiki.libsdl.org/SDL_GL_SetSwapInterval
		constexpr int ADAPTIVE_VSYNC = -1;
		constexpr int VSYNC			 = 1;
		constexpr int NO_VSYNC		 = 0;

		if (gVSyncEnabled)
		{
			// Try adaptive vsync first, fall back to regular vsync
			if (const auto result = SDL_GL_SetSwapInterval(ADAPTIVE_VSYNC); result != 0)
			{
				SDL_GL_SetSwapInterval(VSYNC);
			}
		}
		else
		{
			SDL_GL_SetSwapInterval(NO_VSYNC);
		}
	}
	ImGui::Separator();

	// Display current robot count
	ImGui::Text("Current Robot Count: %zu", gRobots.size());
	ImGui::Separator();

	// Amounts for adding/removing
	constexpr int	 amounts[]	= { 1, 10, 100, 1000, 10000, 100000 };
	constexpr size_t MAX_ROBOTS = 1000000; // Sanity limit to prevent crashes

	// Add robots buttons
	ImGui::Text("Add Robots:");
	for (int amount : amounts)
	{
		// Disable button if it would exceed max limit
		const bool can_add = (gRobots.size() + static_cast<size_t>(amount)) <= MAX_ROBOTS;
		if (!can_add)
		{
			ImGui::BeginDisabled();
		}

		if (ImGui::Button((std::string("+") + std::to_string(amount)).c_str()))
		{
			const size_t new_count = std::min(gRobots.size() + static_cast<size_t>(amount), MAX_ROBOTS);
			const size_t to_add	   = new_count - gRobots.size();

			if (to_add > 0)
			{
				gRobots.reserve(new_count);
				for (size_t i = 0; i < to_add; ++i)
				{
					gRobots.push_back(CreateRandomRobot());
				}
			}
		}

		if (!can_add)
		{
			ImGui::EndDisabled();
		}
		ImGui::SameLine();
	}
	ImGui::NewLine();

	// Remove robots buttons
	ImGui::Text("Remove Robots:");
	for (int amount : amounts)
	{
		// Disable button if there are no robots to remove
		const bool can_remove = !gRobots.empty();
		if (!can_remove)
		{
			ImGui::BeginDisabled();
		}

		if (ImGui::Button((std::string("-") + std::to_string(amount)).c_str()))
		{
			const size_t to_remove = std::min(static_cast<size_t>(amount), gRobots.size());
			if (to_remove > 0)
			{
				gRobots.resize(gRobots.size() - to_remove);
			}
		}

		if (!can_remove)
		{
			ImGui::EndDisabled();
		}
		ImGui::SameLine();
	}
	ImGui::NewLine();

	ImGui::Separator();

	// Clear all button
	const bool has_robots = !gRobots.empty();
	if (!has_robots)
	{
		ImGui::BeginDisabled();
	}

	if (ImGui::Button("Clear All"))
	{
		gRobots.clear();
	}

	if (!has_robots)
	{
		ImGui::EndDisabled();
	}

	// Show warning when approaching limit
	if (gRobots.size() > static_cast<size_t>(MAX_ROBOTS * 0.8))
	{
		ImGui::Separator();
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Warning: Approaching maximum robot limit!");
	}

	ImGui::SeparatorText("Camera Controls");
	if (ImGui::ArrowButton("Left", ImGuiDir_Left))
	{
		gCamera.position.x -= 10.0;
	}
	ImGui::SameLine();
	if (ImGui::ArrowButton("Right", ImGuiDir_Right))
	{
		gCamera.position.x += 10.0;
	}


	if (ImGui::ArrowButton("Up", ImGuiDir_Up))
	{
		gCamera.position.y += 10.0;
	}
	ImGui::SameLine();
	if (ImGui::ArrowButton("Down", ImGuiDir_Down))
	{
		gCamera.position.y -= 10.0;
	}

	ImGui::Checkbox("First Person View", &gCamera.FirstPersonView);

	if (ImGui::SliderAngle("Camera Angle", &gCamera.angle))
	{
		const float cos_angle = std::cos(gCamera.angle);
		const float sin_angle = std::sin(gCamera.angle);
		gCamera.right.x		  = cos_angle;
		gCamera.right.y		  = sin_angle;
		gCamera.up.x		  = -sin_angle;
		gCamera.up.y		  = cos_angle;
	}
	// 200% zoom in -> 0.5 scale
	float percent = 100.f / gZoom;
	ImGui::SliderFloat("Zoom", &percent, 20.f, 400.f);
	gZoom = 100.f / percent;

	ImGui::SeparatorText("Viewport1 Settings");
	ImGui::InputInt("X1", &gViewport1.x);
	ImGui::InputInt("Y1", &gViewport1.y);
	ImGui::InputInt("Width1", &gViewport1.width);
	ImGui::InputInt("Height1", &gViewport1.height);

	//ImGui::SeparatorText("Viewport2 Settings");
	//ImGui::InputInt("X2", &gViewport2.x);
	//ImGui::InputInt("Y2", &gViewport2.y);
	//ImGui::InputInt("Width2", &gViewport2.width);
	//ImGui::InputInt("Height2", &gViewport2.height);

	ImGui::SeparatorText("Depth Settings");

	if (ImGui::Button("Sort as Painters Algorithm"))
	{
		std::sort(gRobots.begin(), gRobots.end(), [](const Robot& left, const Robot& right) {
			return left.depth > right.depth; 
		});
	}

	if (ImGui::Button("Sort as Front to Back"))
	{
		std::sort(
			gRobots.begin(), gRobots.end(),
			[](const Robot& left, const Robot& right)
			{
				return left.depth < right.depth; // then smaller depth drawn first, and frag of larger depth gonna be skipped over by depth test, and hopefully save effort of fragment shader
			});
	}

	if (ImGui::Button("Sort Randomly"))
	{
		std::random_device rd;
		std::mt19937	   g(rd());
		std::shuffle(gRobots.begin(), gRobots.end(), g);
	}

	ImGui::End();
}

// Helper function to create a random robot
Robot CreateRandomRobot()
{
	Robot		robot;
	const float half_width	= static_cast<float>(gWidth) / 2.0f;
	const float half_height = static_cast<float>(gHeight) / 2.0f;
	robot.position.x		= static_cast<double>(util::random(-half_width, half_width));
	robot.position.y		= static_cast<double>(util::random(-half_height, half_height));
	robot.depth				= util::random(-1.0f, 1.0f);
	robot.frame				= util::random(ROBOT_NUM_FRAMES);
	if (util::random(0.0f, 1.0f) < 0.85f)
	{
		robot.r = robot.g = robot.b = 1.0f;
	}
	else
	{
		robot.r = util::random(0.5f, 1.0f);
		robot.g = util::random(0.6f, 1.0f);
		robot.b = util::random(0.45f, 1.0f);
	}
	robot.variation = static_cast<int>(gRobotTextures[static_cast<OpenGL::Handle>(util::random(ROBOT_VARIATIONS))]);
	return robot;
}

// Helper function to switch between renderers
void SwitchRenderer(RendererType type)
{
	if (gCurrentRenderer == type)
		return; // Already using this renderer

	// Shutdown current renderer
	if (gRenderer)
	{
		gRenderer->Shutdown();
		gRenderer.reset();
	}

	// Create and initialize new renderer
	gCurrentRenderer = type;
	switch (type)
	{
		case RendererType::Immediate: gRenderer = std::make_unique<ImmediateRenderer2D>(); break;
		case RendererType::Batch: gRenderer = std::make_unique<BatchRenderer2D>(); break;
		case RendererType::Instanced: gRenderer = std::make_unique<InstancedRenderer2D>(); break;
		default: gRenderer = std::make_unique<ImmediateRenderer2D>(); break;
	}

	if (gRenderer)
	{
		gRenderer->Init();
	}
}
