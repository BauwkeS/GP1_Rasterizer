	//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Maths.h"
#include "Texture.h"
#include "Utils.h"

using namespace dae;

std::vector<Vector2> Renderer::ConvertToScreenSpaceVertex(const std::vector<Vertex>& NDCVVertices) const{
	std::vector<Vector2> newVertices{};
	for (int i = 0; i < NDCVVertices.size(); i++)
	{
		float ScreenSpaceVertexX = (NDCVVertices[i].position.x + 1) / 2 * m_Width;
		float ScreenSpaceVertexY = (1 - NDCVVertices[i].position.y) / 2 * m_Height;
		newVertices.push_back({ ScreenSpaceVertexX,ScreenSpaceVertexY });
	}
	return newVertices;
}

void Renderer::vertextTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const {
	//define triangle
	std::vector<Vertex> verticesWorld{
		{{0.f,2.f,0.f}},
		{{1.f,0.f,0.f}},
		{{-1.f,0.f,0.f}},
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

		vertices_out.push_back(Vertex{ Vector3(projectedVertexWithcameraX, projectedVertexWithcameraY, 1.f) });
	}

	std::vector<Vector2> screenSpaceVertex = ConvertToScreenSpaceVertex(vertices_out);
	vertices_out.clear();

	for (int i = 0; i < screenSpaceVertex.size(); i++)
	{
		vertices_out.push_back(Vertex{ Vector3(screenSpaceVertex[i].x, screenSpaceVertex[i].y, 0.f) });
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
}

Renderer::~Renderer()
{
	//delete[] m_pDepthBufferPixels;
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
	//std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX); //set depth to max for all pixels
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

	//RENDER LOGIC
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			Vector3 pointP{ px + 0.5f, py + 0.5f,1.f };
			
		
			float signedArea1{ Vector3::Cross(pointP - vertixesInScreenSpace[0].position,vertixesInScreenSpace[0].position - vertixesInScreenSpace[1].position).z };
			float signedArea2{ Vector3::Cross(pointP - vertixesInScreenSpace[1].position,vertixesInScreenSpace[1].position - vertixesInScreenSpace[2].position).z };
			float signedArea3{ Vector3::Cross(pointP - vertixesInScreenSpace[2].position,vertixesInScreenSpace[2].position - vertixesInScreenSpace[0].position).z };

			if (signedArea1 < 0 || signedArea2 < 0 || signedArea3 < 0) {
				continue;
			} 

			/*float gradient = px / static_cast<float>(m_Width);
			gradient += py / static_cast<float>(m_Width);
			gradient /= 2.0f;*/

			finalColor = colors::White;

			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
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
