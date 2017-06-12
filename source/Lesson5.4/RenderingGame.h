#pragma once

#include "Game.h"
#include "RenderStateHelper.h"
#include <windows.h>
#include <functional>
#include "CelestialBody.h"

namespace Library
{
	class KeyboardComponent;
	class MouseComponent;
	class GamePadComponent;
	class FpsComponent;
	class Camera;
	class Grid;
}

namespace Rendering
{
	class PointLightDemo;

	class RenderingGame final : public Library::Game
	{
	public:
		RenderingGame(std::function<void*()> getWindowCallback, std::function<void(SIZE&)> getRenderTargetSizeCallback);

		virtual void Initialize() override;
		virtual void Update(const Library::GameTime& gameTime) override;
		virtual void Draw(const Library::GameTime& gameTime) override;
		virtual void Shutdown() override;

		void Exit();

	private:
		static const DirectX::XMVECTORF32 BackgroundColor;

		Library::RenderStateHelper mRenderStateHelper;
		std::shared_ptr<Library::KeyboardComponent> mKeyboard;
		std::shared_ptr<Library::MouseComponent> mMouse;
		std::shared_ptr<Library::GamePadComponent> mGamePad;
		std::shared_ptr<Library::FpsComponent> mFpsComponent;
		std::shared_ptr<Library::Camera> mCamera;
		std::shared_ptr<Library::Grid> mGrid;
		std::shared_ptr<PointLightDemo> mPointLightDemo;

		std::shared_ptr<CelestialBody> mMercury;
		std::shared_ptr<CelestialBody> mVenus;
		std::shared_ptr<CelestialBody> mEarth;
		std::shared_ptr<CelestialBody> mMars;
		std::shared_ptr<CelestialBody> mJupiter;
		std::shared_ptr<CelestialBody> mSaturn;
		std::shared_ptr<CelestialBody> mUranus;
		std::shared_ptr<CelestialBody> mNeptune;
		std::shared_ptr<CelestialBody> mPluto;
	};
}
