#pragma once

#include "DrawableGameComponent.h"
#include "RenderStateHelper.h"
#include <DirectXMath.h>

namespace Library
{
	class Mesh;
	class DirectionalLight;
	class ProxyModel;
	class KeyboardComponent;
}

namespace DirectX
{
	class SpriteBatch;
	class SpriteFont;
}

namespace Rendering
{
	class DiffuseLightingDemo final : public Library::DrawableGameComponent
	{
		RTTI_DECLARATIONS(DiffuseLightingDemo, Library::DrawableGameComponent)

	public:
		DiffuseLightingDemo(Library::Game& game, const std::shared_ptr<Library::Camera>& camera);		

		bool AnimationEnabled() const;
		void SetAnimationEnabled(bool enabled);

		virtual void Initialize() override;
		virtual void Update(const Library::GameTime& gameTime) override;
		virtual void Draw(const Library::GameTime& gameTime) override;

	private:
		struct VertexCBufferPerObject
		{
			DirectX::XMFLOAT4X4 WorldViewProjection;
			DirectX::XMFLOAT4X4 World;

			VertexCBufferPerObject() = default;
			VertexCBufferPerObject(const DirectX::XMFLOAT4X4& wvp, const DirectX::XMFLOAT4X4& world) : WorldViewProjection(wvp), World(world) { }
		};

		struct PixelCBufferPerFrame
		{
			DirectX::XMFLOAT4 AmbientColor;
			DirectX::XMFLOAT3 LightDirection;
			float Padding;
			DirectX::XMFLOAT4 LightColor;			

			PixelCBufferPerFrame() :
				AmbientColor(DirectX::Colors::Black), LightDirection(0.0f, 0.0f, 1.0f), LightColor(DirectX::Colors::White) { }

			PixelCBufferPerFrame(const DirectX::XMFLOAT4& ambientColor, const DirectX::XMFLOAT3& lightDirection, const DirectX::XMFLOAT4& lightColor) :
				AmbientColor(ambientColor), LightDirection(lightDirection), LightColor(lightColor) { }
		};

		void CreateVertexBuffer(const Library::Mesh& mesh, ID3D11Buffer** vertexBuffer) const;
		void ToggleAnimation();
		void UpdateAmbientLight(const Library::GameTime& gameTime);
		void UpdateDirectionalLight(const Library::GameTime& gameTime);

		static const float ModelRotationRate;
		static const DirectX::XMFLOAT2 LightRotationRate;

		VertexCBufferPerObject mVertexCBufferPerObjectData;
		DirectX::XMFLOAT4X4 mWorldMatrix;
		PixelCBufferPerFrame mPixelCBufferPerFrameData;		
		Library::RenderStateHelper mRenderStateHelper;
		Microsoft::WRL::ComPtr<ID3D11VertexShader> mVertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> mPixelShader;
		Microsoft::WRL::ComPtr<ID3D11InputLayout> mInputLayout;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mVertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mIndexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mVertexCBufferPerObject;		
		Microsoft::WRL::ComPtr<ID3D11Buffer> mVertexCBufferPerFrame;		
		Microsoft::WRL::ComPtr<ID3D11Buffer> mPixelCBufferPerFrame;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mColorTexture;
		std::uint32_t mIndexCount;
		std::unique_ptr<Library::DirectionalLight> mDirectionalLight;
		std::unique_ptr<Library::ProxyModel> mProxyModel;
		std::unique_ptr<DirectX::SpriteBatch> mSpriteBatch;
		std::unique_ptr<DirectX::SpriteFont> mSpriteFont;
		DirectX::XMFLOAT2 mTextPosition;
		Library::KeyboardComponent* mKeyboard;
		bool mAnimationEnabled;
	};
}