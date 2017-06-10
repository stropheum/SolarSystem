#pragma once

#include "DrawableGameComponent.h"

namespace Library
{
	class KeyboardComponent;
	struct VertexPositionTexture;
}

namespace Rendering
{
	class FilteringModesDemo final : public Library::DrawableGameComponent
	{
		RTTI_DECLARATIONS(FilteringModesDemo, Library::DrawableGameComponent)

	public:
		FilteringModesDemo(Library::Game& game, const std::shared_ptr<Library::Camera>& camera);

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

		enum class FilteringMode
		{
			Point = 0,
			TriLinear,
			Anisotropic,
			End
		};

		void CreateVertexBuffer(Library::VertexPositionTexture* vertices, std::uint32_t vertexCount, ID3D11Buffer** vertexBuffer) const;
		void CreateIndexBuffer(std::uint32_t* indices, std::uint32_t indexCount, ID3D11Buffer** indexBuffer) const;

		static const std::string FilteringModeDisplayNames[];

		DirectX::XMFLOAT4X4 mWorldMatrix;
		CBufferPerObject mCBufferPerObjectData;
		std::map<FilteringMode, Microsoft::WRL::ComPtr<ID3D11SamplerState>> mTextureSamplersByFilteringMode;
		Microsoft::WRL::ComPtr<ID3D11VertexShader> mVertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> mPixelShader;
		Microsoft::WRL::ComPtr<ID3D11InputLayout> mInputLayout;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mVertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mIndexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mCBufferPerObject;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mColorTexture;
		std::uint32_t mIndexCount;
		FilteringMode mActiveFilteringMode;		
		Library::KeyboardComponent* mKeyboard;
	};
}
