#include "Texture.h"
#include "Vector2.h"
#include <cassert>
#include <SDL_image.h>

namespace dae
{
	Texture::Texture(SDL_Surface* pSurface) :
		m_pSurface{ pSurface },
		m_pSurfacePixels{ (uint32_t*)pSurface->pixels }
	{
	}

	Texture::~Texture()
	{
		if (m_pSurface)	
		{
			SDL_FreeSurface(m_pSurface);
			m_pSurface = nullptr;
		}
	}

	Texture* Texture::LoadFromFile(const std::string& path)
	{
		//TODO
		//Load SDL_Surface using IMG_LOAD
		//Create & Return a new Texture Object (using SDL_Surface)
		SDL_Surface* newSurfaceFromFile{ IMG_Load(path.c_str()) };
		
		if (newSurfaceFromFile == nullptr) return nullptr;		
		
		return new Texture{ newSurfaceFromFile };	
	}

	ColorRGB Texture::Sample(const Vector2& uv) const
	{
		//TODO
		//Sample the correct texel for the given uv
		int x{ static_cast<int>(uv.x * m_pSurface->w) };
		int y{ static_cast<int>(uv.y * m_pSurface->h) };
		uint32_t pixelIdx{ static_cast<uint32_t>(x + (y * m_pSurface->w)) };
		uint8_t r{}, g{}, b{}; //don't ever do this with pointer types
		SDL_GetRGB(m_pSurfacePixels[pixelIdx], m_pSurface->format, &r, &g, &b);

		return { r / 255.f,g / 255.f,b / 255.f };
	}
}