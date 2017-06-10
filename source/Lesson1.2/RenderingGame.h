#pragma once

#include "Game.h"
#include <windows.h>
#include <functional>

namespace Library
{
	class KeyboardComponent;
	class FpsComponent;	
}

namespace Rendering
{
    class RenderingGame : public Library::Game
    {
    public:
		RenderingGame(std::function<void*()> getWindowCallback, std::function<void(SIZE&)> getRenderTargetSizeCallback);

		virtual void Initialize() override;
		virtual void Update(const Library::GameTime& gameTime) override;
		virtual void Draw(const Library::GameTime& gameTime) override;

		void Exit();

    private:
		static const DirectX::XMVECTORF32 BackgroundColor;

		std::shared_ptr<Library::KeyboardComponent> mKeyboard;
		std::shared_ptr<Library::FpsComponent> mFpsComponent;
    };
}
