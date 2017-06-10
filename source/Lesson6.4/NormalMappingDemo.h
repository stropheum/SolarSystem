#pragma once

#include "DrawableGameComponent.h"
#include "RenderStateHelper.h"
#include "VectorHelper.h"
#include "DirectionalLight.h"
#include <DirectXMath.h>

namespace Library
{
	class ProxyModel;
	class KeyboardComponent;
	struct VertexPositionTextureNormalTangent;
}

namespace DirectX
{
	class SpriteBatch;
	class SpriteFont;
}

namespace Rendering
{
	class NormalMappingDemo final : public Library::DrawableGameComponent
	{
		RTTI_DECLARATIONS(NormalMappingDemo, Library::DrawableGameComponent)

	public:
		NormalMappingDemo(Library::Game& game, const std::shared_ptr<Library::Camera>& camera);		

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
				CameraPosition(Library::Vector3Helper::Zero), AmbientColor(Library::Vector4Helper::Zero),
				LightDirection(Library::Vector3Helper::Forward), LightColor(Library::Vector4Helper::One) { }

			PSCBufferPerFrame(const DirectX::XMFLOAT3& cameraPosition, const DirectX::XMFLOAT4& ambientColor, const DirectX::XMFLOAT3& lightDirection, const DirectX::XMFLOAT4& lightColor) :
				CameraPosition(cameraPosition), AmbientColor(ambientColor),
				LightDirection(lightDirection), LightColor(lightColor) { }
		};

		struct PSCBufferPerObject
		{
			DirectX::XMFLOAT3 SpecularColor;
			float SpecularPower;

			PSCBufferPerObject() :
				SpecularColor(Library::Vector3Helper::Zero), SpecularPower(128.0f) { }
		};

		void CreateVertexBuffer(Library::VertexPositionTextureNormalTangent* vertices, std::uint32_t vertexCount, ID3D11Buffer** vertexBuffer) const;
		void UpdateAmbientLight(const Library::GameTime& gameTime);
		void UpdateDirectionalLight(const Library::GameTime& gameTime);
		void UpdateSpecularLight(const Library::GameTime& gameTime);

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
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mColorTexture;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mActiveNormalMap;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mBlankNormalMap;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mRealNormalMap;
		std::unique_ptr<Library::ProxyModel> mProxyModel;
		Library::KeyboardComponent* mKeyboard;
		std::uint32_t mVertexCount;
		std::unique_ptr<DirectX::SpriteBatch> mSpriteBatch;
		std::unique_ptr<DirectX::SpriteFont> mSpriteFont;
		DirectX::XMFLOAT2 mTextPosition;
		bool mUseRealNormalMap;
	};
}
