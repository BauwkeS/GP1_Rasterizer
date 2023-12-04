#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Math.h"
#include "Timer.h"
#include "Vector3.h"
#include "Matrix.h"
#include "MathHelpers.h"

namespace dae
{
	struct Camera
	{
		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle) :
			origin{ _origin },
			fovAngle{ _fovAngle }
		{
		}


		Vector3 origin{};
		const float movementMouseSpeed{ 1.f };
		const float movementKeySpeed{ 5.f };
		float fovAngle{ 90.f };
		float fov{ tanf((fovAngle * TO_RADIANS) / 2.f) };

		Vector3 forward{ Vector3::UnitZ };
		Vector3 up{ Vector3::UnitY };
		Vector3 right{ Vector3::UnitX };

		float totalPitch{};
		float totalYaw{};

		Matrix invViewMatrix{};
		Matrix viewMatrix{};

		void Initialize(float _fovAngle = 90.f, Vector3 _origin = { 0.f,0.f,0.f })
		{
			fovAngle = _fovAngle;
			fov = tanf((fovAngle * TO_RADIANS) / 2.f);

			origin = _origin;
		}

		void CalculateViewMatrix()
		{
			//TODO W1
			right = Vector3::Cross(Vector3::UnitY, forward).Normalized();
			up = Vector3::Cross(forward, right).Normalized();

			viewMatrix = Matrix{ {right,0},{up,0},{forward,0},{origin,1} }.Inverse();

			//ONB => invViewMatrix
			//Inverse(ONB) => ViewMatrix

			//ViewMatrix => Matrix::CreateLookAtLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixlookatlh
		}

		void CalculateProjectionMatrix()
		{
			//TODO W3

			//ProjectionMatrix => Matrix::CreatePerspectiveFovLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovlh
		}


		void Update(Timer* pTimer)
		{	
			const float deltaTime = pTimer->GetElapsed();

			//Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);


			//Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

			//todo: W2
			//assert(false && "Not Implemented Yet");
			//keyboard things
			if (pKeyboardState[SDL_SCANCODE_W]) {//1
				origin.x += forward.x * movementKeySpeed * deltaTime;
				origin.z += forward.z * movementKeySpeed * deltaTime;
			}
			if (pKeyboardState[SDL_SCANCODE_S]) {//2
				origin.x -= forward.x * movementKeySpeed * deltaTime;
				origin.z -= forward.z * movementKeySpeed * deltaTime;
			}
			if (pKeyboardState[SDL_SCANCODE_A]) {//3
				origin.x -= forward.z*movementKeySpeed*deltaTime;
				origin.z += forward.x*movementKeySpeed*deltaTime;
			}
			if (pKeyboardState[SDL_SCANCODE_D]) {//4
				origin.x += forward.z * movementKeySpeed * deltaTime;
				origin.z -= forward.x * movementKeySpeed * deltaTime;
			}

			//mouse things
			if (mouseState == SDL_BUTTON_RMASK) {
				//1 & 2
				totalYaw += mouseX*deltaTime*movementMouseSpeed;
				totalPitch -= mouseY*deltaTime*movementMouseSpeed;
			}
			if (mouseState == SDL_BUTTON_LEFT) {
				//3 & 4
				totalYaw += mouseX * deltaTime * movementMouseSpeed;
				origin.x += (mouseY * forward.x) * deltaTime * movementMouseSpeed;
				origin.z += (mouseY * forward.z) * deltaTime * movementMouseSpeed;
			}
			if (mouseState == SDL_BUTTON_X2) {	
				//5
				origin.y += (mouseY * up.y) * deltaTime * movementMouseSpeed;
			}

			const Matrix rotationMatrix{ Matrix::CreateRotation(TO_RADIANS * totalPitch,TO_RADIANS * totalYaw,0) };

			forward = rotationMatrix.TransformVector(Vector3::UnitZ);
			forward.Normalize();
			right = rotationMatrix.TransformVector(Vector3::UnitX);
			right.Normalize();

			CalculateViewMatrix();
			CalculateProjectionMatrix();
		}
		

	};
}
