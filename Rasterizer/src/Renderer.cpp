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


			/*Normals in NDC on the other hand make no sense… We are interested in normals in world
				space when we do lighting calculations.This means, in the vertex transformation we
				multiply our normals with the World matrix, NOT the WorldViewProjection matrix*/
			Vector3 newNormal { mesh.worldMatrix.TransformVector(mesh.vertices[vertexIdx].normal).Normalized()};
			Vector3 newTangent{ mesh.worldMatrix.TransformVector(mesh.vertices[vertexIdx].tangent).Normalized() };
			Vector3 vecPosition{ mesh.worldMatrix.TransformPoint(mesh.vertices[vertexIdx].position) };


			vec.x = ((vec.x + 1) / 2) * m_Width;
			vec.y = ((1 - vec.y) / 2) * m_Height;
			mesh.vertices_out.push_back(Vertex_Out{ vec,
				mesh.vertices[vertexIdx].color,
				mesh.vertices[vertexIdx].uv,
				newNormal,
				newTangent,
				{vecPosition - m_Camera.origin},
				stayValid });
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
	m_Camera.Initialize(60.f, { .0f,5.0f,-64.f },float(m_Width) / m_Height);

	//m_MeshesWorld = {
	//	Mesh{
	//	{
	//		Vertex{{-3.f,  3.f, -2.f},  {colors::White}, {0.f,0.f}},
	//		Vertex{{ 0.f,  3.f, -2.f},  {colors::White}, {.5f,0.f}},
	//		Vertex{{ 3.f,  3.f, -2.f},  {colors::White}, {1.f,0.f}},
	//		Vertex{{-3.f,  0.f, -2.f},  {colors::White}, {0.f,.5f}},
	//		Vertex{{ 0.f,  0.f, -2.f},  {colors::White}, {.5f,.5f}},
	//		Vertex{{ 3.f,  0.f, -2.f},  {colors::White}, {1.f,.5f}},
	//		Vertex{{-3.f, -3.f, -2.f},  {colors::White}, {0.f,1.f}},
	//		Vertex{{ 0.f, -3.f, -2.f},  {colors::White}, {.5f,1.f}},
	//		Vertex{{ 3.f, -3.f, -2.f},  {colors::White}, {1.f,1.f}}
	//	},
	//	//triangle list
	//	/*{
	//		3, 0, 1,    1, 4, 3,    4, 1, 2,
	//		2, 5, 4,    6, 3, 4,    4, 7, 6,
	//		7, 4, 5,    5, 8, 7
	//	},
	//	PrimitiveTopology::TriangleList}*/
	//	//triangle strip
	//	{
	//		3, 0, 4, 1, 5, 2,
	//		2, 6,
	//		6, 3, 7, 4, 8, 5
	//	},
	//	PrimitiveTopology::TriangleStrip}
	//};

	//mp_Texture = Texture::LoadFromFile("C:/Users/bauwk/Documents/SCHOOL/GRAPHICSPROGRAMMING/GP1_Rasterizer/Rasterizer/Resources/uv_grid_2.png");
	mp_Texture = Texture::LoadFromFile("Resources/vehicle_diffuse.png");
	mp_Normal = Texture::LoadFromFile("Resources/vehicle_normal.png");
	mp_Specular = Texture::LoadFromFile("Resources/vehicle_specular.png");
	mp_Gloss = Texture::LoadFromFile("Resources/vehicle_gloss.png");

	Mesh& mesh = m_MeshesWorld.emplace_back(Mesh{});
	Utils::ParseOBJ("Resources/vehicle.obj", mesh.vertices, mesh.indices);
	mesh.primitiveTopology = PrimitiveTopology::TriangleList;
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
	delete mp_Texture;
	delete mp_Normal;
	delete mp_Specular;
	delete mp_Gloss;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);

	//rotate that stuff
	if (m_RotationEnabled)
	{
		m_CurrentMeshRotation += 0.5f * pTimer->GetElapsed();
		for (Mesh& mesh : m_MeshesWorld)
		{
			Matrix translationMatrix = Matrix::CreateTranslation(0.f, 0.f, 50.f);
			Matrix rotationMatrix = Matrix::CreateRotationY(m_CurrentMeshRotation);
			mesh.worldMatrix = rotationMatrix * translationMatrix;
		}
	}
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
							

							// Check the depth buffer
							if (currentDepth > m_pDepthBufferPixels[depthIndex])
								continue;

							//look at what mode and make either color or go shade it bestie
							ColorRGB barycentricColor{};
							switch (m_CurrentRenderMode)
							{
								case dae::Renderer::RenderMode::FinalColor:
								{
									//SETUP FOR PIXELSHADING MKE ALL YOUR STUFF:--------------

							//POS
									float wInterpolated = currentDepth; //just for eadability
									float zInterpolated = 1 / ((weight0 / vertex0Pos.z) + (weight1 / vertex1Pos.z) + (weight2 / vertex2Pos.z));

									//UV
									Vector2 uvInterpolated = { (
										(vertex0.uv * weight0 / vertex0Pos.w) +
										(vertex1.uv * weight1 / vertex1Pos.w) +
										(vertex2.uv * weight2 / vertex2Pos.w)
									) * currentDepth };

									//COLOR
									ColorRGB colorInterpolated = { (
										(vertex0.color * weight0 / vertex0Pos.w) +
										(vertex1.color * weight1 / vertex1Pos.w) +
										(vertex2.color * weight2 / vertex2Pos.w)
									) * currentDepth };

									//NORMAL
									Vector3 normalInterpolated = { (
										(vertex0.normal * weight0 / vertex0Pos.w) +
										(vertex1.normal * weight1 / vertex1Pos.w) +
										(vertex2.normal * weight2 / vertex2Pos.w)
									) * currentDepth };
									normalInterpolated.Normalize(); //in slides it says you need to mak sure it is normalized pls do

									//TANGENT
									Vector3 tangentInterpolated = { (
										(vertex0.tangent * weight0 / vertex0Pos.w) +
										(vertex1.tangent * weight1 / vertex1Pos.w) +
										(vertex2.tangent * weight2 / vertex2Pos.w)
									) * currentDepth };
									tangentInterpolated.Normalize();

									//VIEWDIRECTION
									Vector3 viewDirectionInterpolated = { (
										(vertex0.viewDirection * weight0 / vertex0Pos.w) +
										(vertex1.viewDirection * weight1 / vertex1Pos.w) +
										(vertex2.viewDirection * weight2 / vertex2Pos.w)
									) * currentDepth };
									viewDirectionInterpolated.Normalize();

									//--------------------------

									//the pixel you are on right now to shade with all the interpolated calc you just did
									Vertex_Out vertex_OutPixelshading{
									Vector4{pointP.x,pointP.y,zInterpolated,wInterpolated},
									colorInterpolated,
									uvInterpolated,
									normalInterpolated,tangentInterpolated,
									viewDirectionInterpolated };




									//barycentricColor = mp_Texture->Sample(uvInterpolated); old news we cool now
									barycentricColor = PxelShading(vertex_OutPixelshading);
									break;
								}
								case dae::Renderer::RenderMode::DepthBuffer:
								{
									//buffer
									float min{ .985f };
									float max{ 1.f };
									float depthBuffer{ (currentDepth - min) * (max - min) };

									barycentricColor = ColorRGB(depthBuffer, depthBuffer, depthBuffer);
									break;
								}
							}

							//ColorRGB barycentricColor = { m_Mesh.vertices_out[indxVector0].color * weightA + m_Mesh.vertices_out[indxVector1].color * weightB + m_Mesh.vertices_out[indxVector2].color * weightC };
						
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


ColorRGB Renderer::PxelShading(Vertex_Out& vec) 
{
	//things we got from the docu
	const Vector3 lightDirection{ .577f, -.577f, .577f };
	const float lightIntensity{ 7.f };

	const ColorRGB lightColor{ 1, 1, 1 };
	const ColorRGB ambientColor{ .03f, .03f, .03f };

	const float shininess{ 25.0f }; 

	//all my uv's from my Textures for readability
	const ColorRGB diffuseColorSample{ mp_Texture->Sample(vec.uv) };
	const ColorRGB specularColorSample{ mp_Specular->Sample(vec.uv) };
	const ColorRGB normalColorSample{ mp_Normal->Sample(vec.uv) };
	const ColorRGB glossinessColorSample{ mp_Gloss->Sample(vec.uv) };

	//tangent space want "Implement tangents":)

	const Vector3 binormal{ Vector3::Cross(vec.normal, vec.tangent) };
	const Matrix tangentSpaceAxis{ Matrix{ vec.tangent, binormal, vec.normal, Vector3::Zero } };

	//normals:
	Vector3 currentNormal{};
	if (m_NormalsEnabled) {
		const Vector3 tangentNormal{ normalColorSample.r * 2.0f - 1.0f, normalColorSample.g * 2.0f - 1.0f, normalColorSample.b * 2.0f - 1.0f };
		currentNormal = { tangentSpaceAxis.TransformVector(tangentNormal.Normalized()).Normalized() };
	}
	else {
		currentNormal = vec.normal;
	}

	//observed area:
	const float observedArea{ Vector3::Dot(currentNormal, -lightDirection) };

	if (observedArea < 0.0f) //if here is nothing return nothing; you got...  nothing:)
		return { 0,0,0 };

	//get that lambert from before
	const ColorRGB lambertDiffuse{ (1.0f * diffuseColorSample) / PI };

	//get that other old phong that was actually fun
	const Vector3 reflect{ lightDirection - (2.0f * Vector3::Dot(currentNormal, lightDirection) * currentNormal) };
	const float RdotV{ std::max(0.0f, Vector3::Dot(reflect, -vec.viewDirection)) };
	const ColorRGB phongSpecular{ specularColorSample * powf(RdotV, glossinessColorSample.r * shininess) };
	
	switch (m_CurrentShadingMode)
	{
	case dae::Renderer::ShadingMode::Combined:
		//get everything in there
		return (((lightColor * lightIntensity) * lambertDiffuse) + phongSpecular + ambientColor) * observedArea;
		break;
	case dae::Renderer::ShadingMode::ObservedArea:
		//only the observed area you had calc before
		return ColorRGB{ observedArea, observedArea, observedArea };
		break;
	case dae::Renderer::ShadingMode::Diffuse:
		//lambert lives here
		return (lightColor * lightIntensity) * lambertDiffuse * observedArea;
		break;
	case dae::Renderer::ShadingMode::Specular:
		//PHOOOOOOONG
		return phongSpecular;
		break;
	default:
		return { 0.f, 0.f, 0.5f }; // give something back to live
		break;
	}
}



bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}

void dae::Renderer::ToggleRenderMode()
{
	//love %
	int cntRenderModes = 2;
	m_CurrentRenderMode = RenderMode(((int)m_CurrentRenderMode + 1) % cntRenderModes);

}
void dae::Renderer::ToggleNormals()
{
	m_NormalsEnabled = !m_NormalsEnabled;
}

void dae::Renderer::ToggleShadingMode()
{
	//cycle session, just give the next one
	switch (m_CurrentShadingMode)
	{
	case dae::Renderer::ShadingMode::Combined:
		m_CurrentShadingMode = ShadingMode::ObservedArea;
		break;
	case dae::Renderer::ShadingMode::ObservedArea:
		m_CurrentShadingMode = ShadingMode::Diffuse;
		break;
	case dae::Renderer::ShadingMode::Diffuse:
		m_CurrentShadingMode = ShadingMode::Specular;
		break;
	case dae::Renderer::ShadingMode::Specular:
		m_CurrentShadingMode = ShadingMode::Combined;
		break;
	}
}