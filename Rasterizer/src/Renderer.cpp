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


void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const {
	//define triangles 
	std::vector<Vertex> verticesWorld{
		//red one
		{{-3.f,  3.f, -2.f},  {colors::White}},
		{ { 0.f,  3.f, -2.f},  {colors::White} },
		{ { 3.f,  3.f, -2.f},  {colors::White} },
		{ {-3.f,  0.f, -2.f},  {colors::White} },
		{ { 0.f,  0.f, -2.f},  {colors::White} },
		{ { 3.f,  0.f, -2.f},  {colors::White} },
		{ {-3.f, -3.f, -2.f},  {colors::White} },
		{ { 0.f, -3.f, -2.f},  {colors::White} },
		{ { 3.f, -3.f, -2.f},  {colors::White} }
	};
	//std::vector<Vertex> verticesWorld{
	//	//red one
	//	{{  0.f,2.f, 0.f }, {1, 0, 0}},
	//	{{  1.5f, -1.f, 0.f }, {1, 0, 0}},
	//	{{ -1.5f, -1.f, 0.f }, {1, 0, 0}},

	//	//original one
	//	{{0.f,4.f,2.f},{1,0,0}},
	//	{{3.f,-2.f,2.f},{0,1,0}},
	//	{{-3.f,-2.f,2.f},{0,0,1}},
	//};
	vertices_out.clear();
	vertices_out.reserve(vertices_in.size());
	for (const Vertex& vertex : vertices_in) //set pixel from world to camera and adjust to camera stuff
	{

		//float aspectRatio{ m_Width / float(m_Height) }; //screen size ratio		
		//
		//Vector3 transformedPoint = m_Camera.viewMatrix.TransformPoint(vertex.position);
		////transformedPoint *= Vector3{ 1 / transformedPoint.z, 1 / transformedPoint.z, 1 }; 
		//transformedPoint.x = (transformedPoint.x / transformedPoint.z) / (aspectRatio * m_Camera.fov);
		//transformedPoint.y = (transformedPoint.y / transformedPoint.z) / (m_Camera.fov);

		//transformedPoint.x = ((transformedPoint.x + 1) * 0.5f) * m_Width;
		//transformedPoint.y = ((1 - transformedPoint.y) * 0.5f) * m_Height;
		////transformedPoint.z = transformedPoint.z + 1;

		////vertices_out[i].position.x = (transformedPoint.x + 1) * 0.5f * m_Width;
		////vertices_out[i].position.y = (1 - transformedPoint.y) * 0.5f * m_Height;
		////vertices_out[i].position.z = transformedPoint.z + 1;

		////vertices_out[i].position = transformedPoint;
		////vertices_out[i].color = vertices_in[i].color;
		//vertices_out.push_back(Vertex{ vertex.position,vertex.color });
		////vertices_out[i].color = colors::White;
		Vector3 vec{ m_Camera.viewMatrix.TransformPoint(vertex.position) };
		const float aspectRatio{ float(m_Width) / float(m_Height) };
		vec.x = (vec.x / vec.z) / (m_AspectRatio * m_Camera.fov);
		vec.y = (vec.y / vec.z) / m_Camera.fov;

		vec.x = ((vec.x + 1) / 2) * m_Width;
		vec.y = ((1 - vec.y) / 2) * m_Height;
		vertices_out.push_back(Vertex{ vec,vertex.color });
	}
}

void dae::Renderer::VertexTransformationFunction(std::vector<Mesh>& meshes_World) const
{
	int meshIdx{};
	for (Mesh& mesh : meshes_World)
	{
		mesh.vertices_out.clear();
		int numTrigs{ int(mesh.indices.size()) / m_NumVertices };
		std::vector<Vertex_Out> meshVerts;
		for (int trigIdx = 0; trigIdx < numTrigs; trigIdx++)
		{
			int offset{ m_NumVertices * trigIdx };
			for (int indicesIdx = 0; indicesIdx < m_NumVertices; indicesIdx++)
			{
				mesh.vertices_out.push_back(VertexTransformationSingular(mesh.vertices[mesh.indices[offset + indicesIdx]]));
			}
		}
		meshIdx++;
	}
}
void Renderer::VertexTransformationFunctionList(Mesh& mesh) const {
	//what you had before just copy paste it bestie it was working
	mesh.vertices_out.clear();
	int numTrigs{ int(mesh.indices.size()) / m_NumVertices };
	std::vector<Vertex_Out> meshVerts;
	for (int trigIdx = 0; trigIdx < numTrigs; trigIdx++)
	{
		int offset{ m_NumVertices * trigIdx };
		for (int indicesIdx = 0; indicesIdx < m_NumVertices; indicesIdx++)
		{
			mesh.vertices_out.push_back(VertexTransformationSingular(mesh.vertices[mesh.indices[offset + indicesIdx]]));
		}
	}
}
void Renderer::VertexTransformationFunctionStrip(Mesh& mesh) const {
	mesh.vertices_out.clear();
	//just add the vertices here
	for (int vertexIdx{}; vertexIdx < mesh.vertices.size(); vertexIdx++)
	{
		mesh.vertices_out.push_back(VertexTransformationSingular(mesh.vertices[vertexIdx]));
	}
}

Vertex_Out dae::Renderer::VertexTransformationSingular(const dae::Vertex& vertexIn) const
{
	//normal
	/*Vector4 vertexTotransform{ vertexIn.position.x,vertexIn.position.y,vertexIn.position.z,0 };
	Vector4 vec{ m_Camera.viewMatrix.TransformPoint(vertexTotransform) };
	const float aspectRatio{ float(m_Width) / float(m_Height) };
	vec.x = (vec.x / vec.z) / (m_AspectRatio * m_Camera.fov);
	vec.y = (vec.y / vec.z) / m_Camera.fov;

	vec.x = ((vec.x + 1) / 2) * m_Width;
	vec.y = ((1 - vec.y) / 2) * m_Height;
	return Vertex_Out{ vec,vertexIn.color,vertexIn.uv };*/

	//wiht matrixes
	Vector4 vertexTotransform{ vertexIn.position.x,vertexIn.position.y,vertexIn.position.z,0 };
	Vector4 vec{ m_Camera.viewMatrix.TransformPoint(vertexTotransform) };
	const float aspectRatio{ float(m_Width) / float(m_Height) };
	const float fov{ m_Camera.fov };
	vec.x = (vec.x / vec.z) / (m_AspectRatio * m_Camera.fov);
	vec.y = (vec.y / vec.z) / m_Camera.fov;

	vec.x = ((vec.x + 1) / 2) * m_Width;
	vec.y = ((1 - vec.y) / 2) * m_Height;
	return Vertex_Out{ vec,vertexIn.color,vertexIn.uv };
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

	m_ListVertices.resize(m_NumVertices);

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f });

	m_MeshesWorld = {
		Mesh {
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
	for (Mesh& mesh : m_MeshesWorld)
	{
		mesh.vertices_out.clear();
	}

	//VertexTransformationFunction(m_MeshesWorld); -> old way of loading now
	
	
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);
	for (Mesh& mesh : m_MeshesWorld)
	{
		m_WorldMatrix = mesh.worldMatrix;
		switch (mesh.primitiveTopology)
		{
		case dae::PrimitiveTopology::TriangleList: //first one
			VertexTransformationFunctionList(mesh);
			RenderItems(mesh.vertices_out);
			break;
		case dae::PrimitiveTopology::TriangleStrip: //second one
			VertexTransformationFunctionStrip(mesh);
			RenderItemsStrip(mesh);
			break;
		}
	}

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void dae::Renderer::RenderItemsStrip(Mesh& mesh)
{
	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
	//std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, std::numeric_limits<float>::max()); //buffer
	SDL_FillRect(m_pBackBuffer, &m_pBackBuffer->clip_rect, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100)); //clear screen
	//std::vector<Vertex> vertixesInScreenSpace(m_MeshesWorld[0].vertices);
	//_MeshesWorld.vertices_out.clear();
	//VertexTransformationFunction(m_MeshesWorld.vertices, vertixesInScreenSpace);

	//the points a -> atm triangle has
	//const int numTriangles{ int(vertixesInScreenSpace.size()) / m_NumVertices };

	//for (int triangleIndex{}; triangleIndex < numTriangles; triangleIndex++) //make triangles from all the vertexes you got
	//{
	//	int indexOffset{ m_NumVertices * triangleIndex };
	//	Vector3 vec1 = (vertixesInScreenSpace[indexOffset].position - vertixesInScreenSpace[indexOffset + 1].position);
	//	Vector3 vec2 = Vector3(vertixesInScreenSpace[indexOffset].position - vertixesInScreenSpace[indexOffset + 2].position);
	//	float totalTriangleArea{ Vector3::Cross(vec1,vec2).z * 0.5f };

	//	BoundingBox boundingBox{};
	//	int boundingBoxWidth{};
	//	int boundingBoxHeight{};

	//	MakeBoundingBox(vertixesInScreenSpace, indexOffset, boundingBox, boundingBoxWidth, boundingBoxHeight);

	//	int pixels{ boundingBoxWidth * boundingBoxHeight };

	//	std::vector<int> pixleIndices{};

	//	pixleIndices.reserve(pixels);

	//	for (int index = 0; index < pixels; index++) pixleIndices.emplace_back(index);

	//	std::for_each(std::execution::par, pixleIndices.begin(), pixleIndices.end(), [&](int i) {
	//		RenderFunction(i, boundingBox, boundingBoxHeight, m_NumVertices, vertixesInScreenSpace, indexOffset, totalTriangleArea);
	//		}); //eyo you learned how to make for each functions look at you go -> I dont know if the help I asked they meant to use the for eahc here tho...

	//}

	for (int vertIdx = 0; vertIdx < mesh.indices.size(); vertIdx++) // you need to do this here bc you need to change your loop acc to slides
	{
		if ((vertIdx + 2) >= mesh.indices.size()) //higher than this is not a triangle anymore
			continue;

		int previousVecInt{ 0 }; //save this so you know which what you gonna make triangle
		bool invalidTrig{ false };
		for (int screenIdx{ 0 }; screenIdx < m_ListVertices.size(); screenIdx++)
		{
			int currentVert{ int(mesh.indices[(vertIdx) + screenIdx] )};
			m_ListVertices[screenIdx] = &mesh.vertices_out[currentVert];

			if (previousVecInt == currentVert && screenIdx > 0)
			{
				invalidTrig = true;
				break;
			}

			previousVecInt = currentVert;
		}
		if (invalidTrig)
			continue;
		if (vertIdx % 2 > 0)
		{
			//Considering if it’s an odd or even triangle if
			//using the triangle strip technique.Hint: odd or even ? -> modulo or bit masking
			std::swap(m_ListVertices[1], m_ListVertices[2]);
		}

		int indexOffset{ 0 };
		Vector3 vec1 = (m_ListVertices[indexOffset]->position - m_ListVertices[indexOffset + 1]->position);
		Vector3 vec2 = Vector3(m_ListVertices[indexOffset]->position - m_ListVertices[indexOffset + 2]->position);
		float totalTriangleArea{ Vector3::Cross(vec1,vec2).z * 0.5f };

		BoundingBox boundingBox{};
		int boundingBoxWidth{};
		int boundingBoxHeight{};

		MakeBoundingBox(m_ListVertices, indexOffset, boundingBox, boundingBoxWidth, boundingBoxHeight);

		int pixels{ boundingBoxWidth * boundingBoxHeight };

		std::vector<int> pixleIndices{};

		pixleIndices.reserve(pixels);

		for (int index = 0; index < pixels; index++) pixleIndices.emplace_back(index);

		std::for_each(std::execution::par, pixleIndices.begin(), pixleIndices.end(), [&](int i) {
			RenderFunction(i, boundingBox, boundingBoxHeight, m_NumVertices, m_ListVertices, indexOffset, totalTriangleArea);
			});
	}
}

void dae::Renderer::MakeBoundingBox(std::vector<Vertex_Out*>& vertixesInScreenSpace, int& indexOffset, BoundingBox& boudingBox, int& boundingBoxWidth, int& boundingBoxHeight) {
	/*	int minX{};
		int maxX{};
		int minY{};
		int maxY{};*/

		//for (int vertexId = 0; vertexId < numVertices; vertexId++) {
	Vector3 vertexPos0{ vertixesInScreenSpace[indexOffset]->position };
	Vector3 vertexPos1{ vertixesInScreenSpace[indexOffset + 1]->position };
	Vector3 vertexPos2{ vertixesInScreenSpace[indexOffset + 2]->position };

	int minX{ int(std::min(vertexPos0.x, std::min(vertexPos1.x, vertexPos2.x))) };
	int maxX{ int(std::max(vertexPos0.x, std::max(vertexPos1.x, vertexPos2.x))) };

	int minY{ int(std::min(vertexPos0.y, std::min(vertexPos1.y, vertexPos2.y))) };
	int maxY{ int(std::max(vertexPos0.y, std::max(vertexPos1.y, vertexPos2.y))) };
	//}

	/*if (minX < 0) minX = 0;
	else minX -= 1;
	if (minY < 0) minY = 0;
	else minY -= 1;

	if (maxX > m_Width) maxX = m_Width;
	else maxX += 1;
	if (maxY > m_Height) maxY = m_Height;
	else maxY += 1;*/

	minX = std::min(minX, m_Width - 1);
	minX = std::max(minX, 0);

	minY = std::min(minY, m_Height - 1);
	minY = std::max(minY, 0);

	maxX = std::min(maxX, m_Width - 1);
	maxX = std::max(maxX, 0);

	maxY = std::min(maxY, m_Height - 1);
	maxY = std::max(maxY, 0);

	int buffer{ 2 };
	boudingBox = BoundingBox{ minX, minY, maxX, maxY };
	boundingBoxWidth = int{ maxX - minX + buffer };
	boundingBoxHeight = int{ maxY - minY + buffer };
}

void dae::Renderer::MakeBoundingBox(std::vector<Vertex_Out>& vertixesInScreenSpace, int& indexOffset, BoundingBox& boudingBox, int& boundingBoxWidth, int& boundingBoxHeight) {
	/*	int minX{};
		int maxX{};
		int minY{};
		int maxY{};*/

		//for (int vertexId = 0; vertexId < numVertices; vertexId++) {
	Vector3 vertexPos0{ vertixesInScreenSpace[indexOffset].position };
	Vector3 vertexPos1{ vertixesInScreenSpace[indexOffset + 1].position };
	Vector3 vertexPos2{ vertixesInScreenSpace[indexOffset + 2].position };

	int minX{ int(std::min(vertexPos0.x, std::min(vertexPos1.x, vertexPos2.x))) };
	int maxX{ int(std::max(vertexPos0.x, std::max(vertexPos1.x, vertexPos2.x))) };

	int minY{ int(std::min(vertexPos0.y, std::min(vertexPos1.y, vertexPos2.y))) };
	int maxY{ int(std::max(vertexPos0.y, std::max(vertexPos1.y, vertexPos2.y))) };
	//}

	/*if (minX < 0) minX = 0;
	else minX -= 1;
	if (minY < 0) minY = 0;
	else minY -= 1;

	if (maxX > m_Width) maxX = m_Width;
	else maxX += 1;
	if (maxY > m_Height) maxY = m_Height;
	else maxY += 1;*/

	minX = std::min(minX, m_Width - 1);
	minX = std::max(minX, 0);

	minY = std::min(minY, m_Height - 1);
	minY = std::max(minY, 0);

	maxX = std::min(maxX, m_Width - 1);
	maxX = std::max(maxX, 0);

	maxY = std::min(maxY, m_Height - 1);
	maxY = std::max(maxY, 0);

	int buffer{ 2 };
	boudingBox = BoundingBox{ minX, minY, maxX, maxY };
	boundingBoxWidth = int{ maxX - minX + buffer };
	boundingBoxHeight = int{ maxY - minY + buffer };
}

void dae::Renderer::RenderItems(std::vector<Vertex_Out>& vertixesInScreenSpace)
{
	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
	//std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, std::numeric_limits<float>::max()); //buffer
	SDL_FillRect(m_pBackBuffer, &m_pBackBuffer->clip_rect, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100)); //clear screen

	//std::vector<Vertex> vertixesInScreenSpace(m_MeshesWorld[0].vertices);
	//_MeshesWorld.vertices_out.clear();
	//VertexTransformationFunction(m_MeshesWorld.vertices, vertixesInScreenSpace);

	//the points a -> atm triangle has


	const int numTriangles{ int(vertixesInScreenSpace.size()) / m_NumVertices };

	for (int triangleIndex{}; triangleIndex < numTriangles; triangleIndex++) //make triangles from all the vertexes you got
	{
		int indexOffset{ m_NumVertices * triangleIndex };
		Vector3 vec1 = (vertixesInScreenSpace[indexOffset].position - vertixesInScreenSpace[indexOffset + 1].position);
		Vector3 vec2 = Vector3(vertixesInScreenSpace[indexOffset].position - vertixesInScreenSpace[indexOffset + 2].position);
		float totalTriangleArea{ Vector3::Cross(vec1,vec2).z * 0.5f };

		/*	int minX{};
		int maxX{};
		int minY{};
		int maxY{};*/

		//for (int vertexId = 0; vertexId < numVertices; vertexId++) {
		Vector3 vertexPos0{ vertixesInScreenSpace[indexOffset].position };
		Vector3 vertexPos1{ vertixesInScreenSpace[indexOffset + 1].position };
		Vector3 vertexPos2{ vertixesInScreenSpace[indexOffset + 2].position };

		int minX{ int(std::min(vertexPos0.x, std::min(vertexPos1.x, vertexPos2.x))) };
		int maxX{ int(std::max(vertexPos0.x, std::max(vertexPos1.x, vertexPos2.x))) };

		int minY{ int(std::min(vertexPos0.y, std::min(vertexPos1.y, vertexPos2.y))) };
		int maxY{ int(std::max(vertexPos0.y, std::max(vertexPos1.y, vertexPos2.y))) };
		//}

		/*if (minX < 0) minX = 0;
		else minX -= 1;
		if (minY < 0) minY = 0;
		else minY -= 1;

		if (maxX > m_Width) maxX = m_Width;
		else maxX += 1;
		if (maxY > m_Height) maxY = m_Height;
		else maxY += 1;*/

		minX = std::min(minX, m_Width - 1);
		minX = std::max(minX, 0);

		minY = std::min(minY, m_Height - 1);
		minY = std::max(minY, 0);

		maxX = std::min(maxX, m_Width - 1);
		maxX = std::max(maxX, 0);

		maxY = std::min(maxY, m_Height - 1);
		maxY = std::max(maxY, 0);

		int buffer{ 2 };
		int boundingBoxWidth{ maxX - minX + buffer };
		int boundingBoxHeight{ maxY - minY + buffer };
		BoundingBox boundingBox{ minX, minY, maxX, maxY };

		int pixels{boundingBoxWidth * boundingBoxHeight};

		/*std::vector<int> pixleIndices{};

		pixleIndices.reserve(pixels);

		for (int index = 0; index < pixels; index++) pixleIndices.emplace_back(index);

		std::for_each(std::execution::par, pixleIndices.begin(), pixleIndices.end(), [&](int i) {
			RenderFunction(i, boundingBox, boundingBoxHeight, m_NumVertices, vertixesInScreenSpace, indexOffset, totalTriangleArea);
			});*/ //eyo you learned how to make for each functions look at you go -> I dont know if the healp I asked they meant to use the for eahc here tho...

		for (int pixelIdx{}; pixelIdx < pixels; pixelIdx++)
		{
			RenderFunction(pixelIdx, boundingBox, boundingBoxHeight, m_NumVertices, vertixesInScreenSpace, indexOffset, totalTriangleArea);
		}
	}
}

void Renderer::RenderFunction(int pixelIdx, dae::BoundingBox& boundingBox, int boundingBoxHeight, const int& numVertices, std::vector<Vertex_Out>& vertixesInScreenSpace, int indexOffset, float totalTriangleArea) const
{
	std::vector<Vertex_Out*> vertixesInScreenSpaceNew{};
	for (int i = 0; i < vertixesInScreenSpace.size(); i++)
	{
		vertixesInScreenSpaceNew.push_back(&vertixesInScreenSpace[i]);
	}
	RenderFunction(pixelIdx, boundingBox, boundingBoxHeight, numVertices, vertixesInScreenSpaceNew, indexOffset, totalTriangleArea);
}
void Renderer::RenderFunction(int pixelIdx, dae::BoundingBox& boundingBox, int boundingBoxHeight, const int& numVertices, std::vector<Vertex_Out*>& vertixesInScreenSpace, int indexOffset, float totalTriangleArea) const
{
	ColorRGB barycentricColor{}; //color to put on triangle
	bool inTriangle{ true }; //you finally made the bool to easy check
	float currentDepth{}; //to check with buffer
	Vector2 uvTexture{}; //uv to give pos to -> to give to texture to know which color it is
	//RENDER LOGIC
	//for (int px{ boundingBox.left }; px < boundingBox.right; ++px)
	//{
	//	if (px < 0) continue;

	//	for (int py{ boundingBox.bottom }; py < boundingBox.top; ++py)
	//	{
	int px{ boundingBox.left + (int(pixelIdx) / boundingBoxHeight) };
	int py{ boundingBox.bottom + (int(pixelIdx) % boundingBoxHeight) };
	const Vector3 pointP{ px + 0.5f, py + 0.5f,1.f };

	/*if (pointP.x < minX || pointP.x > maxX ||
	pointP.y < minY || pointP.y > maxY)
	{
	continue;
	}*/

	for (int vertexIndex{ 0 }; vertexIndex < numVertices; vertexIndex++)
	{
		//calculate if you are inside the triangle you are making
		const float crossResult{ Vector3::Cross(
			Vector3{ vertixesInScreenSpace[((vertexIndex + 1) % numVertices) + indexOffset]->position }
			- Vector3{ vertixesInScreenSpace[vertexIndex + indexOffset]->position },
			pointP - vertixesInScreenSpace[vertexIndex + indexOffset]->position)
			.z };

		if (crossResult < 0)
		{
			inTriangle = false;
			break;
		}

		//add color and depth to the pixel
		barycentricColor += vertixesInScreenSpace[((vertexIndex + 2) % numVertices + indexOffset)]->color * ((crossResult * 0.5f) / totalTriangleArea);
		currentDepth += 1/ vertixesInScreenSpace[((vertexIndex + 2) % numVertices + indexOffset)]->position.z * ((crossResult * 0.5f) / totalTriangleArea);
		//std::cout << "hello?\n";
		uvTexture += (vertixesInScreenSpace[((vertexIndex + 2) % numVertices)]->uv / vertixesInScreenSpace[((vertexIndex + 2) % numVertices)]->position.z) * ((crossResult * 0.5f) / totalTriangleArea);

	}
	if (inTriangle) {

		currentDepth = 1 / currentDepth;
		uvTexture *= currentDepth;

		int pixelIdx{ (px * m_Height) + py };

		if (m_pDepthBufferPixels[pixelIdx] >= currentDepth)
		{
			m_pDepthBufferPixels[pixelIdx] = currentDepth;

			barycentricColor = mp_Texture->Sample(uvTexture);
			barycentricColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pFrontBuffer->format,
				static_cast<uint8_t>(barycentricColor.r * 255),
				static_cast<uint8_t>(barycentricColor.g * 255),
				static_cast<uint8_t>(barycentricColor.b * 255));
		}
	}

	/*	if (!inTriangle)
		continue;*/



		//	}
		//}
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
