	//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Maths.h"
#include "Texture.h"
#include "Utils.h"

using namespace dae;

std::vector<Vertex> Renderer::ConvertToScreenSpaceVertex(const std::vector<Vertex>& NDCVVertices) const{
	std::vector<Vertex> newVertices{};
	for (int i = 0; i < NDCVVertices.size(); i++)
	{
		float ScreenSpaceVertexX = (NDCVVertices[i].position.x + 1) / 2 * m_Width;
		float ScreenSpaceVertexY = (1 - NDCVVertices[i].position.y) / 2 * m_Height;
		newVertices.push_back(Vertex{ { ScreenSpaceVertexX,ScreenSpaceVertexY,NDCVVertices[i].position.z}, {NDCVVertices[i].color}});
	}
	return newVertices;
}

void Renderer::vertextTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const {
	//define triangle
	std::vector<Vertex> verticesWorld{
		{{0.f,4.f,2.f},{1,0,0}},
		{{3.f,-2.f,2.f},{0,1,0}},
		{{-3.f,-2.f,2.f},{0,0,1}},
	};

	Matrix ViewMatrix{ m_Camera.CalculateCameraToWorld() };
	ViewMatrix.Inverse();
	
	for (int i = 0; i < verticesWorld.size(); i++)
	{
		Vector3 viewSpacePoint = ViewMatrix.TransformPoint(verticesWorld[i].position);

		float projectedVertexX = viewSpacePoint.x / viewSpacePoint.z;
		float projectedVertexY = viewSpacePoint.y / viewSpacePoint.z;

		float projectedVertexWithcameraX = projectedVertexX / (m_Width / m_Height * m_Camera.fov);
		float projectedVertexWithcameraY = projectedVertexY / m_Camera.fov;

		vertices_out.push_back(Vertex{ Vector3(projectedVertexWithcameraX, projectedVertexWithcameraY, 1.f), { verticesWorld[i].color } });
	}

	std::vector<Vertex> screenSpaceVertex = ConvertToScreenSpaceVertex(vertices_out);
	vertices_out.clear();

	for (int i = 0; i < screenSpaceVertex.size(); i++)
	{
		vertices_out.push_back(Vertex{ Vector3(screenSpaceVertex[i].position.x, screenSpaceVertex[i].position.y, screenSpaceVertex[i].position.z), { screenSpaceVertex[i].color } });
	}
}

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow)
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	//m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f });

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	m_pDepthBufferPixels = new float[m_Width * m_Height];
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);
}

void Renderer::Render()
{
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);
	ColorRGB finalColor{};
	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX); //set depth to max for all pixels
	SDL_FillRect(m_pBackBuffer, &m_pBackBuffer->clip_rect, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

	//add those points here
	//std::vector<Vector3> vertices_ndc{
	//	{0.f,0.5f,1.f},
	//	{.5f,-.5f,1.f},
	//	{-.5f,-.5f,1.f},
	//};
	std::vector<Vertex> vertixesInScreenSpace{};

	vertextTransformationFunction({}, vertixesInScreenSpace);


	std::vector<Vector2> pixels;
	std::vector<float> depthBuffer{90000000000000000.f};

	constexpr int numVertices{ 3 };

	const int numTriangles{ static_cast<int>(vertixesInScreenSpace.size()) / numVertices };

	for (int triangleIndex{}; triangleIndex < numTriangles; triangleIndex++)
	{
		Vector3 vertexPos0{ vertixesInScreenSpace[triangleIndex * numVertices + 0].position };
		Vector3 vertexPos1{ vertixesInScreenSpace[triangleIndex * numVertices + 1].position };
		Vector3 vertexPos2{ vertixesInScreenSpace[triangleIndex * numVertices + 2].position };

		const float minX{ std::min(vertexPos0.x, std::min(vertexPos1.x, vertexPos2.x)) };
		const float maxX{ std::max(vertexPos0.x, std::max(vertexPos1.x, vertexPos2.x)) };

		const float minY{ std::min(vertexPos0.y, std::min(vertexPos1.y, vertexPos2.y)) };
		const float maxY{ std::max(vertexPos0.y, std::max(vertexPos1.y, vertexPos2.y)) };

		const int indexOffset{ numVertices * triangleIndex };
		const Vector3 side1{ vertixesInScreenSpace[indexOffset + 1].position - vertixesInScreenSpace[indexOffset].position };
		const Vector3 side2{ vertixesInScreenSpace[indexOffset + 2].position - vertixesInScreenSpace[indexOffset].position };
		const float totalTriangleArea{ Vector3::Cross(side1, side2).z * 0.5f };

	//RENDER LOGIC
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			Vector3 pointP{ px + 0.5f, py + 0.5f,1.f };

			bool inTriangle{ true };
			float currentDepth{};
			ColorRGB barycentricColor{};
			for (int vertexIndex{ 0 }; vertexIndex < numVertices; vertexIndex++)
			{
				const float crossResult{ Vector3::Cross(Vertex{vertixesInScreenSpace[(vertexIndex + 1) % numVertices + indexOffset]}.position
					- Vertex{vertixesInScreenSpace[vertexIndex + indexOffset]}.position,
					pointP - Vertex{vertixesInScreenSpace[vertexIndex + indexOffset]}.position).z };

				if (crossResult < 0)
				{
					inTriangle = false;
					break;
				}

				barycentricColor += vertixesInScreenSpace[((vertexIndex + 2) % numVertices + indexOffset)].color * ((crossResult * 0.5f) / totalTriangleArea);
				currentDepth += vertixesInScreenSpace[((vertexIndex + 2) % numVertices + indexOffset)].position.z * ((crossResult * 0.5f) / totalTriangleArea);
			}

			if (!inTriangle)
				continue;

			const int depthBufferIndex{ px + (py * m_Width) };

			if (m_pDepthBufferPixels[depthBufferIndex] >= currentDepth)
			{
				m_pDepthBufferPixels[depthBufferIndex] = currentDepth;

				//Update Color in Buffer
				finalColor = barycentricColor;
				finalColor.MaxToOne();

				m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
					static_cast<uint8_t>(finalColor.r * 255),
					static_cast<uint8_t>(finalColor.g * 255),
					static_cast<uint8_t>(finalColor.b * 255));
			}

		}
		}
	}

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	//Todo > W1 Projection Stage
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
