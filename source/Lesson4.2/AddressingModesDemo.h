#pragma once

#include "DrawableGameComponent.h"

namespace Library
{
	class KeyboardComponent;
	struct VertexPositionTexture;
}

namespace Rendering
{
	class AddressingModesDemo final : public Library::DrawableGameComponent
	{
		RTTI_DECLARATIONS(AddressingModesDemo, Library::DrawableGameComponent)

	public:
		AddressingModesDemo(Library::Game& game, const std::shared_ptr<Library::Camera>& camera);

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

		enum class AddressingMode
		{
			Wrap = 0,
			Mirror,
			Clamp,
			Border,
			End
		};

		void CreateVertexBuffer(Library::VertexPositionTexture* vertices, std::uint32_t vertexCount, ID3D11Buffer** vertexBuffer) const;
		void CreateIndexBuffer(uint32_t* indices, uint32_t indexCount, ID3D11Buffer** indexBuffer) const;

		static const std::string AddressingModeDisplayNames[];

		DirectX::XMFLOAT4X4 mWorldMatrix;
		CBufferPerObject mCBufferPerObjectData;
		std::map<AddressingMode, Microsoft::WRL::ComPtr<ID3D11SamplerState>> mTextureSamplersByAddressingMode;
		Microsoft::WRL::ComPtr<ID3D11VertexShader> mVertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> mPixelShader;
		Microsoft::WRL::ComPtr<ID3D11InputLayout> mInputLayout;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mVertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mIndexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mCBufferPerObject;				
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mColorTexture;
		std::uint32_t mIndexCount;
		AddressingMode mActiveAddressingMode;		
		Library::KeyboardComponent* mKeyboard;
	};
}