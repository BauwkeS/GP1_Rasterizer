#pragma once

#include <cstdint>
#include <vector>

#include "Camera.h"
#include "DataTypes.h"

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Texture;
	struct Mesh;
	struct Vertex;
	class Timer;
	class Scene;
	struct Vertex_Out;
	class Matrix;

	struct BoundingBox
	{
		int left;
		int bottom;
		int right;
		int top;
	};

	struct HitResult
	{
		bool didHit;
		ColorRGB color;
		Vector2 uv;
		double depth;
	};

	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(Timer* pTimer);
		void Render();

		bool SaveBufferToImage() const;

		void VertexTransformationFunction(std::vector<Mesh>& meshes) const;

		void ToggleRenderMode();
		void ToggleRotation() { m_RotationEnabled = !m_RotationEnabled; }
		void ToggleNormals();
		void ToggleShadingMode();

		ColorRGB PxelShading(Vertex_Out& vec);

	private:
		enum class RenderMode
		{
			FinalColor,
			DepthBuffer
		};
		enum class ShadingMode
		{
			Combined,
			ObservedArea,
			Diffuse,		
			Specular		
		};
		RenderMode m_CurrentRenderMode{ RenderMode::FinalColor };
		ShadingMode m_CurrentShadingMode{ ShadingMode::Combined };

		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		float* m_pDepthBufferPixels{};

		Camera m_Camera{};

		int m_Width{};
		int m_Height{};

		std::vector<Mesh> m_MeshesWorld{};
		//Mesh m_Mesh{};

		float m_AspectRatio{};

		const int m_NumVertices{ 3 };

		Texture* mp_Texture{};
		Texture* mp_Normal{};
		Texture* mp_Specular{};
		Texture* mp_Gloss{};


		bool m_NormalsEnabled{ true };

		float m_CurrentMeshRotation{ 0.0f };
		bool m_RotationEnabled{ true };
	};
}
