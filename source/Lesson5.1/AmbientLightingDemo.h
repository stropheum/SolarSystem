#pragma once

#include "DrawableGameComponent.h"
#include "RenderStateHelper.h"
#include <DirectXMath.h>

namespace Library
{
	class Mesh;
	class KeyboardComponent;
}

namespace DirectX
{
	class SpriteBatch;
	class SpriteFont;
}

namespace Rendering
{
	class AmbientLightingDemo final : public Library::DrawableGameComponent
	{
		RTTI_DECLARATIONS(AmbientLightingDemo, DrawableGameComponent)

	public:
		AmbientLightingDemo(Library::Game& game, const std::shared_ptr<Library::Camera>& camera);

		virtual void Initialize() override;
		virtual void Update(const Library::GameTime& gameTime) override;
		virtual void Draw(const Library::GameTime& gameTime) override;

	private:
		struct CBufferPerObject
		{
			DirectX::XMFLOAT4X4 WorldViewProjection;

			CBufferPerObject() = default;
			CBufferPerObject(const DirectX::XMFLOAT4X4& wvp) : WorldViewProjection(wvp) { }
		};

		struct CBufferPerFrame
		{
			DirectX::XMFLOAT4 AmbientColor; // Constant buffers must have a byte width in multiples of 16

			CBufferPerFrame() : AmbientColor(1.0f, 1.0f, 1.0f, 1.0f) { }
			CBufferPerFrame(const DirectX::XMFLOAT4& ambientColor) : AmbientColor(ambientColor) { }
		};

		void CreateVertexBuffer(const Library::Mesh& mesh, ID3D11Buffer** vertexBuffer) const;
		void UpdateAmbientLight(const Library::GameTime& gameTime);

		DirectX::XMFLOAT4X4 mWorldMatrix;
		CBufferPerObject mCBufferPerObjectData;
		CBufferPerFrame mCBufferPerFrameData;
		Library::RenderStateHelper mRenderStateHelper;
		Microsoft::WRL::ComPtr<ID3D11VertexShader> mVertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> mPixelShader;
		Microsoft::WRL::ComPtr<ID3D11InputLayout> mInputLayout;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mVertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mIndexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mCBufferPerObject;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mCBufferPerFrame;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mColorTexture;
		std::uint32_t mIndexCount;		
		std::unique_ptr<DirectX::SpriteBatch> mSpriteBatch;
		std::unique_ptr<DirectX::SpriteFont> mSpriteFont;
		DirectX::XMFLOAT2 mTextPosition;
		Library::KeyboardComponent* mKeyboard;
	};
}
