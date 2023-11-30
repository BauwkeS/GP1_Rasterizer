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
	//define triangles 
	std::vector<Vertex> verticesWorld{
		//red one
		{{  0.f,2.f, 0.f }, {1, 0, 0}},
		{{  1.5f, -1.f, 0.f }, {1, 0, 0}},
		{{ -1.5f, -1.f, 0.f }, {1, 0, 0}},

		//original one
		{{0.f,4.f,2.f},{1,0,0}},
		{{3.f,-2.f,2.f},{0,1,0}},
		{{-3.f,-2.f,2.f},{0,0,1}},
	};
	
	size_t vecSize{ verticesWorld.size() };
	vertices_out.resize(vecSize);

	float aspectRatio{ m_Width / float(m_Height) }; //screen size ratio
	for (size_t i{}; i < vecSize; ++i) //set pixel from world to camera and adjust to camera stuff
	{
		Matrix ViewMatrix{ m_Camera.CalculateCameraToWorld() };
		ViewMatrix.Inverse();
		Vector3 transformedPoint = ViewMatrix.TransformPoint(verticesWorld[i].position);
		transformedPoint *= Vector3{ 1 / transformedPoint.z, 1 / transformedPoint.z, 1 }; 
		transformedPoint.x = transformedPoint.x / (aspectRatio * m_Camera.fov);
		transformedPoint.y = transformedPoint.y / (m_Camera.fov);

		transformedPoint.x = (transformedPoint.x + 1) * 0.5f * m_Width;
		transformedPoint.y = (1 - transformedPoint.y) * 0.5f * m_Height;
		transformedPoint.z = transformedPoint.z + 1;

		//vertices_out[i].position.x = (transformedPoint.x + 1) * 0.5f * m_Width;
		//vertices_out[i].position.y = (1 - transformedPoint.y) * 0.5f * m_Height;
		//vertices_out[i].position.z = transformedPoint.z + 1;

		vertices_out[i].position = transformedPoint;
		vertices_out[i].color = verticesWorld[i].color;
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

	m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f });
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
	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, std::numeric_limits<float>::max()); //buffer
	SDL_FillRect(m_pBackBuffer, &m_pBackBuffer->clip_rect, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100)); //clear screen

	std::vector<Vertex> vertixesInScreenSpace{};
	vertextTransformationFunction({}, vertixesInScreenSpace);

	//the points a -> atm triangle has
	constexpr int numVertices{ 3 };

	const int numTriangles{ static_cast<int>(vertixesInScreenSpace.size()) / numVertices };

	for (int triangleIndex{}; triangleIndex < numTriangles; triangleIndex++) //make triangles from all the vertexes you got
	{
		Vector3 vertexPos0{ vertixesInScreenSpace[triangleIndex * numVertices + 0].position };
		Vector3 vertexPos1{ vertixesInScreenSpace[triangleIndex * numVertices + 1].position };
		Vector3 vertexPos2{ vertixesInScreenSpace[triangleIndex * numVertices + 2].position };

		const float minX{ std::min(vertexPos0.x, std::min(vertexPos1.x, vertexPos2.x)) };
		const float maxX{ std::max(vertexPos0.x, std::max(vertexPos1.x, vertexPos2.x)) };

		const float minY{ std::min(vertexPos0.y, std::min(vertexPos1.y, vertexPos2.y)) };
		const float maxY{ std::max(vertexPos0.y, std::max(vertexPos1.y, vertexPos2.y)) };

		const int indexOffset{ numVertices * triangleIndex };
		const float totalTriangleArea{ Vector3::Cross(
			Vector3(vertixesInScreenSpace[indexOffset + 1].position - vertixesInScreenSpace[indexOffset].position),
			Vector3(vertixesInScreenSpace[indexOffset + 2].position - vertixesInScreenSpace[indexOffset].position)
			).z * 0.5f};

	//RENDER LOGIC
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			const Vector3 pointP{ px + 0.5f, py + 0.5f,1.f };

			if (pointP.x < minX || pointP.x > maxX ||
				pointP.y < minY || pointP.y > maxY)
			{
				continue;
			}

			bool inTriangle{ true }; //you finally made the bool to easy check
			float currentDepth{}; //to check with buffer
			ColorRGB barycentricColor{}; //color to put on triangle

			for (int vertexIndex{ 0 }; vertexIndex < numVertices; vertexIndex++)
			{
				//calculate if you are inside the triangle you are making
				const float crossResult{ Vector3::Cross(Vertex{vertixesInScreenSpace[(vertexIndex + 1) % numVertices + indexOffset]}.position
					- Vertex{vertixesInScreenSpace[vertexIndex + indexOffset]}.position,
					pointP - Vertex{vertixesInScreenSpace[vertexIndex + indexOffset]}.position).z };

				if (crossResult < 0)
				{
					inTriangle = false;
					break;
				}
				//add color and depth to the pixel
				barycentricColor += vertixesInScreenSpace[((vertexIndex + 2) % numVertices + indexOffset)].color * ((crossResult * 0.5f) / totalTriangleArea);
				currentDepth += vertixesInScreenSpace[((vertexIndex + 2) % numVertices + indexOffset)].position.z * ((crossResult * 0.5f) / totalTriangleArea);
			}

			if (!inTriangle)
				continue;

			const int depthBufferIndex{ px + (py * m_Width) };

			//check if what you are checking is in front of the one you are checking or not
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
