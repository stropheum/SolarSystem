#pragma once

#include "DrawableGameComponent.h"
#include "RenderStateHelper.h"
#include "DirectionalLight.h"
#include <DirectXMath.h>
#include <DirectXColors.h>

namespace Library
{
	class Mesh;
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
	class BlinnPhongDemo final : public Library::DrawableGameComponent
	{
		RTTI_DECLARATIONS(BlinnPhongDemo, Library::DrawableGameComponent)

	public:
		BlinnPhongDemo(Library::Game& game, const std::shared_ptr<Library::Camera>& camera);

		bool AnimationEnabled() const;
		void SetAnimationEnabled(bool enabled);

		virtual void Initialize() override;
		virtual void Update(const Library::GameTime& gameTime) override;
		virtual void Draw(const Library::GameTime& gameTime) override;

	private:
		struct VSCBufferPerObject
		{
			DirectX::XMFLOAT4X4 WorldViewProjection;
			DirectX::XMFLOAT4X4 World;

			VSCBufferPerObject() = default;
			VSCBufferPerObject(const DirectX::XMFLOAT4X4& wvp, const DirectX::XMFLOAT4X4& world) :
				WorldViewProjection(wvp), World(world) { }
		};

		struct PSCBufferPerFrame
		{
			DirectX::XMFLOAT3 CameraPosition;
			float Padding;
			DirectX::XMFLOAT4 AmbientColor;
			DirectX::XMFLOAT3 LightDirection;
			float Padding2;
			DirectX::XMFLOAT4 LightColor;

			PSCBufferPerFrame() :
				AmbientColor(DirectX::Colors::Black), LightDirection(0.0f, 0.0f, 1.0f), LightColor(DirectX::Colors::White) { }
		};

		struct PSCBufferPerObject
		{
			DirectX::XMFLOAT3 SpecularColor;
			float SpecularPower;

			PSCBufferPerObject() :
				SpecularColor(1.0f, 1.0f, 1.0f), SpecularPower(128.0f) { }
		};

		void CreateVertexBuffer(const Library::Mesh& mesh, ID3D11Buffer** vertexBuffer) const;
		void ToggleAnimation();
		void UpdateAmbientLight(const Library::GameTime& gameTime);
		void UpdateDirectionalLight(const Library::GameTime& gameTime);
		void UpdateSpecularLight(const Library::GameTime& gameTime);

		static const float ModelRotationRate;
		static const DirectX::XMFLOAT2 LightRotationRate;
		static const float LightModulationRate;

		DirectX::XMFLOAT4X4 mWorldMatrix;
		VSCBufferPerObject mVSCBufferPerObjectData;
		PSCBufferPerFrame mPSCBufferPerFrameData;
		PSCBufferPerObject mPSCBufferPerObjectData;
		Library::DirectionalLight mDirectionalLight;
		Library::RenderStateHelper mRenderStateHelper;
		Microsoft::WRL::ComPtr<ID3D11VertexShader> mVertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> mPixelShader;
		Microsoft::WRL::ComPtr<ID3D11InputLayout> mInputLayout;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mVertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mIndexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mVSCBufferPerObject;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mPSCBufferPerFrame;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mPSCBufferPerObject;		
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mColorAndSpecularMap;		
		std::unique_ptr<Library::ProxyModel> mProxyModel;
		Library::KeyboardComponent* mKeyboard;
		std::uint32_t mIndexCount;
		std::unique_ptr<DirectX::SpriteBatch> mSpriteBatch;
		std::unique_ptr<DirectX::SpriteFont> mSpriteFont;
		DirectX::XMFLOAT2 mTextPosition;
		bool mAnimationEnabled;
	};
}
