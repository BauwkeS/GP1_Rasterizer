	//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Maths.h"
#include "Texture.h"
#include "Utils.h"
#include "DataTypes.h"
#include <iostream>
#include <limits>
#include <execution>

using namespace dae;

//void dae::Renderer::VertexTransformationFunction(Mesh& mesh) const
//{
//	Matrix worldViewProjectionMatrix = mesh.worldMatrix * m_Camera.viewMatrix * m_Camera.projectionMatrix;
//	
//	mesh.vertices_out.clear();
//	mesh.vertices_out.reserve(mesh.vertices.size());
//	//just add the vertices here
//	for (int vertexIdx{}; vertexIdx < mesh.vertices.size(); vertexIdx++)
//	{
//		Vector4 vertexTotransform{ mesh.vertices[vertexIdx].position.x,mesh.vertices[vertexIdx].position.y,mesh.vertices[vertexIdx].position.z,1};
//		Vector4 vec{ worldViewProjectionMatrix.TransformPoint(vertexTotransform) };
//
//		vec.x /= vec.w;
//		vec.y /= vec.w;
//		vec.z /= vec.w;
//
//		//convert to screenspace from NDC
//		vec.x = ((vec.x + 1) / 2) * m_Width;
//		vec.y = ((1 - vec.y) / 2) * m_Height;
//		mesh.vertices_out.push_back(Vertex_Out{ vec,mesh.vertices[vertexIdx].color,mesh.vertices[vertexIdx].uv, mesh.vertices[vertexIdx].normal, mesh.vertices[vertexIdx].tangent});
//	}
//}

void dae::Renderer::VertexTransformationFunction(std::vector<Mesh>& meshes) const
{
	for (Mesh& mesh : meshes) 
	{
		Matrix worldViewProjectionMatrix = mesh.worldMatrix * m_Camera.viewMatrix * m_Camera.projectionMatrix;

		mesh.vertices_out.clear();
		mesh.vertices_out.reserve(mesh.vertices.size());
		//just add the vertices here
		for (int vertexIdx{}; vertexIdx < mesh.vertices.size(); vertexIdx++)
		{
			Vector4 vertexTotransform{ mesh.vertices[vertexIdx].position.x,mesh.vertices[vertexIdx].position.y,mesh.vertices[vertexIdx].position.z,1 };
			Vector4 vec{ worldViewProjectionMatrix.TransformPoint(vertexTotransform) };
			bool stayValid = true;

			vec.x /= vec.w;
			vec.y /= vec.w;
			vec.z /= vec.w;
			// Do frustum culling
			if (vec.z < 0.0f || vec.z > 1.0f
				|| vec.x < -1.0f || vec.x > 1.0f
				|| vec.y < -1.0f || vec.y > 1.0f)
				stayValid = false;

			vec.x = ((vec.x + 1) / 2) * m_Width;
			vec.y = ((1 - vec.y) / 2) * m_Height;
			mesh.vertices_out.push_back(Vertex_Out{ vec,mesh.vertices[vertexIdx].color,mesh.vertices[vertexIdx].uv, mesh.vertices[vertexIdx].normal, mesh.vertices[vertexIdx].tangent,stayValid });
		}
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
	for (int pixelIdx = 0; pixelIdx < m_Width * m_Height; pixelIdx++)
	{
		m_pDepthBufferPixels[pixelIdx] = std::numeric_limits<float>::max();
	}

	m_AspectRatio = float(m_Width) / float(m_Height);

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f },float(m_Width) / m_Height);

	m_MeshesWorld = {
		Mesh{
		{
			Vertex{{-3.f,  3.f, -2.f},  {colors::White}, {0.f,0.f}},
			Vertex{{ 0.f,  3.f, -2.f},  {colors::White}, {.5f,0.f}},
			Vertex{{ 3.f,  3.f, -2.f},  {colors::White}, {1.f,0.f}},
			Vertex{{-3.f,  0.f, -2.f},  {colors::White}, {0.f,.5f}},
			Vertex{{ 0.f,  0.f, -2.f},  {colors::White}, {.5f,.5f}},
			Vertex{{ 3.f,  0.f, -2.f},  {colors::White}, {1.f,.5f}},
			Vertex{{-3.f, -3.f, -2.f},  {colors::White}, {0.f,1.f}},
			Vertex{{ 0.f, -3.f, -2.f},  {colors::White}, {.5f,1.f}},
			Vertex{{ 3.f, -3.f, -2.f},  {colors::White}, {1.f,1.f}}
		},
		//triangle list
		/*{
			3, 0, 1,    1, 4, 3,    4, 1, 2,
			2, 5, 4,    6, 3, 4,    4, 7, 6,
			7, 4, 5,    5, 8, 7
		},
		PrimitiveTopology::TriangleList}*/
		//triangle strip
		{
			3, 0, 4, 1, 5, 2,
			2, 6,
			6, 3, 7, 4, 8, 5
		},
		PrimitiveTopology::TriangleStrip}
	};


	mp_Texture = Texture::LoadFromFile("C:/Users/bauwk/Documents/SCHOOL/GRAPHICSPROGRAMMING/GP1_Rasterizer/Rasterizer/Resources/uv_grid_2.png");
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
	delete mp_Texture;
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
	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
	SDL_FillRect(m_pBackBuffer, &m_pBackBuffer->clip_rect, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100)); //clear screen
	
	VertexTransformationFunction(m_MeshesWorld);

		for (Mesh& mesh : m_MeshesWorld)
		{
			int numTriangles{};
			switch (mesh.primitiveTopology)
			{
			case dae::PrimitiveTopology::TriangleList: //first one
				numTriangles = mesh.indices.size() / 3;
				break;
			case dae::PrimitiveTopology::TriangleStrip: //second one
				numTriangles = mesh.indices.size() - 2;
				break;
			}

			for (int indiceIdx = 0; indiceIdx < numTriangles; ++indiceIdx)
			{
				uint32_t indxVector0{ };
				uint32_t indxVector1{ };
				uint32_t indxVector2{ };
				switch (mesh.primitiveTopology)
				{
				case PrimitiveTopology::TriangleList:
					indxVector0 = mesh.indices[indiceIdx * 3];
					indxVector1 = mesh.indices[indiceIdx * 3 + 1];
					indxVector2 = mesh.indices[indiceIdx * 3 + 2];
					break;
				case PrimitiveTopology::TriangleStrip:
					indxVector0 = mesh.indices[indiceIdx];
					indxVector1 = mesh.indices[indiceIdx + 1];
					indxVector2 = mesh.indices[indiceIdx + 2];
					if (indiceIdx % 2 == 1)
					{
						std::swap(indxVector1, indxVector2); //make every other triangle rotate the other way
					}

					// not a triangle so skip
					if (indxVector0 == indxVector1 || indxVector2 == indxVector0 || indxVector1 == indxVector2)
						continue;
				}

				const Vertex_Out vertex0{ mesh.vertices_out[indxVector0] };
				const Vertex_Out vertex1{ mesh.vertices_out[indxVector1] };
				const Vertex_Out vertex2{ mesh.vertices_out[indxVector2] };

				const Vector4 vertex0Pos{ mesh.vertices_out[indxVector0].position };
				const Vector4 vertex1Pos{ mesh.vertices_out[indxVector1].position };
				const Vector4 vertex2Pos{ mesh.vertices_out[indxVector2].position };

				if (!vertex0.valid || !vertex1.valid || !vertex2.valid)
					continue;

				//Bouding Box ---------------
				const Vector3 edge10{ vertex1Pos - vertex0Pos };
				const Vector3 edge21{ vertex2Pos - vertex1Pos };
				const Vector3 edge02{ vertex0Pos - vertex2Pos };

				int minX{ int(std::min(vertex0Pos.x, std::min(vertex1Pos.x, vertex2Pos.x))) };
				int maxX{ int(std::max(vertex0Pos.x, std::max(vertex1Pos.x, vertex2Pos.x))) };
				
				int minY{ int(std::min(vertex0Pos.y, std::min(vertex1Pos.y, vertex2Pos.y))) };
				int maxY{ int(std::max(vertex0Pos.y, std::max(vertex1Pos.y, vertex2Pos.y))) };

				int buffer{ 2 };
				//clamp so it does not go out of bounds
				minX = Clamp(minX-buffer, 0, m_Width);
				maxX = Clamp(maxX+buffer, 0, m_Width);

				minY = Clamp(minY-buffer, 0, m_Height);
				maxY = Clamp(maxY+buffer, 0, m_Height);

				//---------------

				for (int px{ minX }; px < maxX; ++px)
				{
					if (px < 0) continue;

					for (int py{ minY }; py < maxY; ++py)
					{
						if (py < 0) continue;

						const Vector3 pointP{ px + 0.5f, py + 0.5f,0.f };

						const Vector3 signedAreaParallelogram12{ Vector3::Cross(edge21, pointP - vertex1Pos) };
						const Vector3 signedAreaParallelogram20{ Vector3::Cross(edge02, pointP - vertex2Pos) };
						const Vector3 signedAreaParallelogram01{ Vector3::Cross(edge10, pointP - vertex0Pos) };
						const float triangleArea = signedAreaParallelogram12.z + signedAreaParallelogram20.z + signedAreaParallelogram01.z;

						bool isInsideTriangle = true;
						isInsideTriangle &= signedAreaParallelogram01.z >= 0.0f;
						isInsideTriangle &= signedAreaParallelogram20.z >= 0.0f;
						isInsideTriangle &= signedAreaParallelogram12.z >= 0.0f;

						if (isInsideTriangle) {
							// weights
							const float weight0{ signedAreaParallelogram12.z / triangleArea };
							const float weight1{ signedAreaParallelogram20.z / triangleArea };
							const float weight2{ signedAreaParallelogram01.z / triangleArea };

							// check to seeif the weight is correct bc this breaks 24/7 pls
							assert((weight0 + weight1 + weight2) > 0.99f);
							assert((weight0 + weight1 + weight2) < 1.01f);


							// interpolated depth
							float currentDepth = 1 / ((weight0 / vertex0Pos.w) + (weight1 / vertex1Pos.w) + (weight2 / vertex2Pos.w));

							const int depthIndex{ px + (py * m_Width) };
							float min{ .985f };
							float max{ 1.f };
							float depthBuffer{ (currentDepth - min) * (max - min) };

							// Check the depth buffer
							if (currentDepth > m_pDepthBufferPixels[depthIndex])
								continue;
							Vector2 uvInterpolated = { (
								(vertex0.uv * weight0 / vertex0Pos.w) +
								(vertex1.uv * weight1 / vertex1Pos.w) +
								(vertex2.uv * weight2 / vertex2Pos.w)
							) * currentDepth };

							//ColorRGB barycentricColor = { m_Mesh.vertices_out[indxVector0].color * weightA + m_Mesh.vertices_out[indxVector1].color * weightB + m_Mesh.vertices_out[indxVector2].color * weightC };
							ColorRGB barycentricColor = mp_Texture->Sample(uvInterpolated);
							
							//ColorRGB barycentricColor = ColorRGB(depthBuffer, depthBuffer, depthBuffer);

							m_pDepthBufferPixels[depthIndex] = currentDepth;

							//Update Color in Buffer
							barycentricColor.MaxToOne();
							m_pBackBufferPixels[depthIndex] = SDL_MapRGB(m_pBackBuffer->format,
								static_cast<uint8_t>(barycentricColor.r * 255),
								static_cast<uint8_t>(barycentricColor.g * 255),
								static_cast<uint8_t>(barycentricColor.b * 255));
						}
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

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
