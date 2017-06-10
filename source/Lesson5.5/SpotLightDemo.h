#pragma once

#include "DrawableGameComponent.h"
#include "RenderStateHelper.h"
#include "SpotLight.h"
#include <DirectXMath.h>
#include <DirectXColors.h>

namespace Library
{	
	class ProxyModel;
	class KeyboardComponent;
	struct VertexPositionTextureNormal;
}

namespace DirectX
{
	class SpriteBatch;
	class SpriteFont;
}

namespace Rendering
{
	class SpotLightDemo final : public Library::DrawableGameComponent
	{
		RTTI_DECLARATIONS(SpotLightDemo, Library::DrawableGameComponent)

	public:		
		SpotLightDemo(Library::Game& game, const std::shared_ptr<Library::Camera>& camera);

		virtual void Initialize() override;
		virtual void Update(const Library::GameTime& gameTime) override;
		virtual void Draw(const Library::GameTime& gameTime) override;

	private:
		struct VSCBufferPerFrame
		{
			DirectX::XMFLOAT3 LightPosition;
			float LightRadius;
			
			VSCBufferPerFrame() :
				LightPosition(Library::Vector3Helper::Zero), LightRadius(50.0f) { }
			VSCBufferPerFrame(const DirectX::XMFLOAT3 lightPosition, float lightRadius) :
				LightPosition(lightPosition), LightRadius(lightRadius) { }
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
			DirectX::XMFLOAT4 AmbientColor;
			DirectX::XMFLOAT4 LightColor;
			DirectX::XMFLOAT3 LightPosition;
			float SpotLightInnerAngle;
			float SpotLightOuterAngle;
			DirectX::XMFLOAT3 CameraPosition;
			DirectX::XMFLOAT3 LightLookAt;
			float Padding;

			PSCBufferPerFrame() :
				AmbientColor(DirectX::Colors::Black), LightColor(DirectX::Colors::White), LightPosition(Library::Vector3Helper::Zero),
				SpotLightInnerAngle(0.75f), SpotLightOuterAngle(0.25f), CameraPosition(Library::Vector3Helper::Zero), LightLookAt(Library::Vector3Helper::Forward)
			{
			}

			PSCBufferPerFrame(const DirectX::XMFLOAT4& ambientColor, const DirectX::XMFLOAT4& lightColor, const DirectX::XMFLOAT3& lightPosition, float spotLightInnerAngle, float spotLightOuterAngle, const DirectX::XMFLOAT3& cameraPosition, const DirectX::XMFLOAT3& lightLookAt) :
				AmbientColor(ambientColor), LightColor(lightColor), LightPosition(lightPosition),
				SpotLightInnerAngle(spotLightInnerAngle), SpotLightOuterAngle(spotLightOuterAngle),
				CameraPosition(cameraPosition), LightLookAt(lightLookAt)
			{
			}
		};

		struct PSCBufferPerObject
		{
			DirectX::XMFLOAT3 SpecularColor;
			float SpecularPower;

			PSCBufferPerObject() :
				SpecularColor(1.0f, 1.0f, 1.0f), SpecularPower(128.0f) { }

			PSCBufferPerObject(const DirectX::XMFLOAT3& specularColor, float specularPower) :
				SpecularColor(specularColor), SpecularPower(specularPower) { }
		};

		void CreateVertexBuffer(Library::VertexPositionTextureNormal* vertices, std::uint32_t vertexCount, ID3D11Buffer** vertexBuffer) const;
		void UpdateAmbientLight(const Library::GameTime& gameTime);
		void UpdateSpotLight(const Library::GameTime& gameTime);
		void UpdateSpecularLight(const Library::GameTime& gameTime);
				
		static const float ModelRotationRate;
		static const float LightModulationRate;
		static const float LightMovementRate;
		static const DirectX::XMFLOAT2 LightRotationRate;

		PSCBufferPerFrame mPSCBufferPerFrameData;
		DirectX::XMFLOAT4X4 mWorldMatrix;
		VSCBufferPerFrame mVSCBufferPerFrameData;
		VSCBufferPerObject mVSCBufferPerObjectData;
		PSCBufferPerObject mPSCBufferPerObjectData;
		Library::SpotLight mSpotLight;
		Library::RenderStateHelper mRenderStateHelper;
		Microsoft::WRL::ComPtr<ID3D11VertexShader> mVertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> mPixelShader;
		Microsoft::WRL::ComPtr<ID3D11InputLayout> mInputLayout;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mVertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mIndexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mVSCBufferPerFrame;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mVSCBufferPerObject;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mPSCBufferPerFrame;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mPSCBufferPerObject;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mColorTexture;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mSpecularMap;		
		std::unique_ptr<Library::ProxyModel> mProxyModel;
		Library::KeyboardComponent* mKeyboard;
		std::uint32_t mVertexCount;
		std::unique_ptr<DirectX::SpriteBatch> mSpriteBatch;
		std::unique_ptr<DirectX::SpriteFont> mSpriteFont;
		DirectX::XMFLOAT2 mTextPosition;
	};
}
