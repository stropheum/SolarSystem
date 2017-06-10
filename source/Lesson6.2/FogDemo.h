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
	class FogDemo : public Library::DrawableGameComponent
	{
		RTTI_DECLARATIONS(FogDemo, Library::DrawableGameComponent)

	public:
		FogDemo(Library::Game& game, const std::shared_ptr<Library::Camera>& camera);

		bool AnimationEnabled() const;
		void SetAnimationEnabled(bool enabled);

		virtual void Initialize() override;
		virtual void Update(const Library::GameTime& gameTime) override;
		virtual void Draw(const Library::GameTime& gameTime) override;

	private:
		struct VSCBufferPerFrame
		{			
			DirectX::XMFLOAT3 CameraPosition;
			float FogStart;
			float FogRange;
			DirectX::XMFLOAT3 Padding;

			VSCBufferPerFrame() :
				CameraPosition(Library::Vector3Helper::Zero), FogStart(30.0f), FogRange(40.0f) { }
			VSCBufferPerFrame(const DirectX::XMFLOAT3& cameraPosition, float fogStart, float fogRange) :
				CameraPosition(cameraPosition), FogStart(fogStart), FogRange(fogRange) { }
		};

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
			DirectX::XMFLOAT4 FogColor;			

			PSCBufferPerFrame() :
				CameraPosition(Library::Vector3Helper::Zero), AmbientColor(Library::Vector4Helper::Zero), 
				LightDirection(Library::Vector3Helper::Forward), LightColor(Library::Vector4Helper::One),
				FogColor(DirectX::Colors::CornflowerBlue) { }

			PSCBufferPerFrame(const DirectX::XMFLOAT3& cameraPosition, const DirectX::XMFLOAT4& ambientColor, const DirectX::XMFLOAT3& lightDirection, const DirectX::XMFLOAT4& lightColor, const DirectX::XMFLOAT4& fogColor ) :
				CameraPosition(cameraPosition), AmbientColor(ambientColor),
				LightDirection(lightDirection), LightColor(lightColor),
				FogColor(fogColor) { }
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
		VSCBufferPerFrame mVSCBufferPerFrameData;
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
		Microsoft::WRL::ComPtr<ID3D11Buffer> mVSCBufferPerFrame;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mPSCBufferPerFrame;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mPSCBufferPerObject;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mColorTexture;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mSpecularMap;
		std::unique_ptr<Library::ProxyModel> mProxyModel;
		Library::KeyboardComponent* mKeyboard;
		std::uint32_t mIndexCount;
		std::unique_ptr<DirectX::SpriteBatch> mSpriteBatch;
		std::unique_ptr<DirectX::SpriteFont> mSpriteFont;
		DirectX::XMFLOAT2 mTextPosition;
		bool mAnimationEnabled;
	};
}
