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

		void VertexTransformationFunction(Mesh& mesh) const;
		/*void VertexTransformationFunctionList(Mesh& mesh) const;
		void VertexTransformationFunctionStrip(Mesh& mesh) const;
		Vertex_Out VertexTransformationSingular(const dae::Vertex& vertexIn, Mesh& mesh) const;
		void RenderFunction(int pixelIdx, dae::BoundingBox& boundingBox, int boundingBoxHeight, const int& numVertices, std::vector<Vertex_Out>& vertixesInScreenSpace, int indexOffset, float totalTriangleArea) const;
		void RenderFunction(int pixelIdx, dae::BoundingBox& boundingBox, int boundingBoxHeight, const int& numVertices, std::vector<Vertex_Out*>& vertixesInScreenSpace, int indexOffset, float totalTriangleArea) const;
		void RenderItems(std::vector<Vertex_Out>& vertixesInScreenSpace);
		void RenderItemsStrip(Mesh& mesh);	*/

		void MakeBoundingBox(std::vector<Vertex_Out>& vertixesInScreenSpace, int& indexOffset, BoundingBox& boudingBox, int& boundingBoxWidth, int& boundingBoxHeight);
		void MakeBoundingBox(std::vector<Vertex_Out*>& vertixesInScreenSpace, int& indexOffset, BoundingBox& boudingBox, int& boundingBoxWidth, int& boundingBoxHeight);
		
	private:
		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		float* m_pDepthBufferPixels{};

		Camera m_Camera{};

		int m_Width{};
		int m_Height{};

		//std::vector<Mesh> m_MeshesWorld{};
		Mesh m_Mesh{};
		std::vector<Vertex_Out*> m_ListVertices;
		//Mesh m_MeshesWorld2{};	

		float m_AspectRatio{};

		const int m_NumVertices{ 3 };

		Texture* mp_Texture{};

		//Matrix m_WorldViewProjectionMatrix{};
		//Matrix m_WorldMatrix{};
		//Matrix m_ViewMatrix{};
		//Matrix m_ProjectionMatrix{};
	};
}
