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
		int x{ int(uv.x * m_pSurface->w) };
		int y{ int(uv.y * m_pSurface->h) };
		Uint32 pixel{ Uint32(x + (y * m_pSurface->w)) };
		Uint8 r{};
		Uint8 g{};
		Uint8 b{};
		SDL_GetRGB(m_pSurfacePixels[pixel], m_pSurface->format, &r, &g, &b);

		return { r / 255.f,g / 255.f,b / 255.f };
	}
}