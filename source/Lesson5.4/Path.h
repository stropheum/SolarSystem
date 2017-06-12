#pragma once

#include "DrawableGameComponent.h"
#include <wrl.h>
#include <d3d11_2.h>
#include <DirectXMath.h>

namespace Library
{
	class Path final : public DrawableGameComponent
	{
		RTTI_DECLARATIONS(Path, DrawableGameComponent)

	public:
		Path(Game& game, const std::shared_ptr<Camera>& camera, const float& radius);
		Path(Game& game, const std::shared_ptr<Camera>& camera, const float& radius, UINT size, UINT scale, const DirectX::XMFLOAT4& color);
		
		Path() = delete;
		Path(const Path&) = delete;
		Path& operator=(const Path&) = delete;

		const DirectX::XMFLOAT3& Position() const;
		const DirectX::XMFLOAT4 Color() const;
		const UINT Size() const;
		const UINT Scale() const;

		void SetPosition(const DirectX::XMFLOAT3& position);
		void SetPosition(float x, float y, float z);
		void SetColor(const DirectX::XMFLOAT4& color);
		void SetSize(UINT size);
		void SetScale(UINT scale);

		virtual void Initialize() override;
		virtual void Draw(const GameTime& gameTime) override;

	private:
		struct VertexCBufferPerObject
		{
			DirectX::XMFLOAT4X4 WorldViewProjection;

			VertexCBufferPerObject() { }
			VertexCBufferPerObject(const DirectX::XMFLOAT4X4& wvp) : WorldViewProjection(wvp) { }
		};

		void InitializePath();

		static const UINT DefaultSize;
		static const UINT DefaultScale;
		static const DirectX::XMFLOAT4 DefaultColor;

		Microsoft::WRL::ComPtr<ID3D11VertexShader> mVertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> mPixelShader;
		Microsoft::WRL::ComPtr<ID3D11InputLayout> mInputLayout;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mVertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mVertexCBufferPerObject;
		VertexCBufferPerObject mVertexCBufferPerObjectData;

		DirectX::XMFLOAT3 mPosition;
		UINT mSize;
		UINT mScale;
		float mRadius;
		DirectX::XMFLOAT4 mColor;
		DirectX::XMFLOAT4X4 mWorldMatrix;
	};
}