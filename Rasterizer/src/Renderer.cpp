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
			
			ColorRGB barycentricColor{};

			//what is/can see triangle test 

			//float signedArea1{ Vector3::Cross(pointP - vertixesInScreenSpace[0].position,vertixesInScreenSpace[0].position - vertixesInScreenSpace[1].position).z };
			//float signedArea2{ Vector3::Cross(pointP - vertixesInScreenSpace[1].position,vertixesInScreenSpace[1].position - vertixesInScreenSpace[2].position).z };
			//float signedArea3{ Vector3::Cross(pointP - vertixesInScreenSpace[2].position,vertixesInScreenSpace[2].position - vertixesInScreenSpace[0].position).z };
			//float totalTriangleArea{ signedArea1 + signedArea2 + signedArea3 };

			//if (signedArea1 < 0 || signedArea2 < 0 || signedArea3 < 0) {
			//	continue; //not in triangle
			//} 
			//else {
			//	//is in triangle
			//	//calculating the final weights:
			//	float finalSignedArea1{ signedArea1 / totalTriangleArea };
			//	float finalSignedArea2{ signedArea2 / totalTriangleArea };
			//	float finalSignedArea3{ signedArea3 / totalTriangleArea };

			//	//interpolated color:
			//	finalColor = vertixesInScreenSpace[0].color * finalSignedArea1 + vertixesInScreenSpace[1].color * finalSignedArea2 + vertixesInScreenSpace[2].color * finalSignedArea3;
			//}

			//in barycentric coordinates
			float signedAreaWeightV0{ Vector3::Cross(vertixesInScreenSpace[2].position - vertixesInScreenSpace[1].position, pointP - vertixesInScreenSpace[1].position).z };
			float signedAreaWeightV1{ Vector3::Cross(vertixesInScreenSpace[1].position - vertixesInScreenSpace[0].position, pointP - vertixesInScreenSpace[0].position).z };
			float signedAreaWeightV2{ Vector3::Cross(vertixesInScreenSpace[0].position - vertixesInScreenSpace[2].position, pointP - vertixesInScreenSpace[2].position).z };

			float totalTriangleArea{ signedAreaWeightV0 + signedAreaWeightV1 + signedAreaWeightV2 };
			
			//if (totalTriangleArea == 1) finalColor = colors::Red;

			if (signedAreaWeightV0 < 0 || signedAreaWeightV1 < 0 || signedAreaWeightV2 < 0) {
				continue; //not in triangle
			}
			else {
				//is in triangle
					}

			//calculating the final weights:
			float finalSignedAreaWeightV0{ signedAreaWeightV0 / totalTriangleArea };
			float finalSignedAreaWeightV1{ signedAreaWeightV1 / totalTriangleArea };
			float finalSignedAreaWeightV2{ signedAreaWeightV2 / totalTriangleArea };

			//interpolated color:
			barycentricColor += vertixesInScreenSpace[0].color * finalSignedAreaWeightV0 + vertixesInScreenSpace[1].color * finalSignedAreaWeightV1 + vertixesInScreenSpace[2].color * finalSignedAreaWeightV2;



			//---------------

			/*float gradient = px / static_cast<float>(m_Width);
			gradient += py / static_cast<float>(m_Width);
			gradient /= 2.0f;*/

			finalColor = barycentricColor;

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
