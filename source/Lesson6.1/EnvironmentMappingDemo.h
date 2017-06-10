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
	class EnvironmentMappingDemo : public Library::DrawableGameComponent
	{
		RTTI_DECLARATIONS(EnvironmentMappingDemo, Library::DrawableGameComponent)

	public:		
		EnvironmentMappingDemo(Library::Game& game, const std::shared_ptr<Library::Camera>& camera);

		virtual void Initialize() override;
		virtual void Update(const Library::GameTime& gameTime) override;
		virtual void Draw(const Library::GameTime& gameTime) override;

	private:
		struct VSCBufferPerFrame
		{
			DirectX::XMFLOAT3 CameraPosition;
			float Padding;

			VSCBufferPerFrame() :
				CameraPosition(Library::Vector3Helper::Zero) { }
			VSCBufferPerFrame(const DirectX::XMFLOAT3& cameraPosition) :
				CameraPosition(cameraPosition) { }
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
			DirectX::XMFLOAT4 EnvColor;

			PSCBufferPerFrame() :
				AmbientColor(Library::Vector4Helper::One), EnvColor(Library::Vector4Helper::One) { }
			PSCBufferPerFrame(const DirectX::XMFLOAT4& ambientColor, const DirectX::XMFLOAT4& envColor) :
				AmbientColor(ambientColor), EnvColor(envColor) { }
		};

		struct PSCBufferPerObject
		{
			float ReflectionAmount;
			DirectX::XMFLOAT3 Padding;

			PSCBufferPerObject() : ReflectionAmount(0.9f) { }
			PSCBufferPerObject(float reflectionAmount) : ReflectionAmount(reflectionAmount) { }
		};

		void CreateVertexBuffer(const Library::Mesh& mesh, ID3D11Buffer** vertexBuffer) const;
				
		DirectX::XMFLOAT4X4 mWorldMatrix;
		PSCBufferPerFrame mPSCBufferPerFrameData;		
		VSCBufferPerFrame mVSCBufferPerFrameData;
		VSCBufferPerObject mVSCBufferPerObjectData;
		PSCBufferPerObject mPSCBufferPerObjectData;
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
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mEnvironmentMap;
		Library::KeyboardComponent* mKeyboard;
		std::uint32_t mIndexCount;
		std::unique_ptr<DirectX::SpriteBatch> mSpriteBatch;
		std::unique_ptr<DirectX::SpriteFont> mSpriteFont;
		DirectX::XMFLOAT2 mTextPosition;
	};
}
