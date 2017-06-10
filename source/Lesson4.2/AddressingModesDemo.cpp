#include "pch.h"

using namespace std;
using namespace Library;
using namespace DirectX;

namespace Rendering
{
	RTTI_DEFINITIONS(AddressingModesDemo)

	const string AddressingModesDemo::AddressingModeDisplayNames[] =
	{
		"Wrap",
		"Mirror",
		"Clamp",
		"Border"
	};

	AddressingModesDemo::AddressingModesDemo(Game& game, const shared_ptr<Camera>& camera) : 
		DrawableGameComponent(game, camera),		
		mWorldMatrix(MatrixHelper::Identity), mIndexCount(0), mActiveAddressingMode(AddressingMode::Wrap)
	{
	}

	void AddressingModesDemo::Initialize()
	{
		// Load a compiled vertex shader
		vector<char> compiledVertexShader;
		Utility::LoadBinaryFile(L"Content\\Shaders\\AddressingModesDemoVS.cso", compiledVertexShader);		
		ThrowIfFailed(mGame->Direct3DDevice()->CreateVertexShader(&compiledVertexShader[0], compiledVertexShader.size(), nullptr, mVertexShader.ReleaseAndGetAddressOf()), "ID3D11Device::CreatedVertexShader() failed.");

		// Load a compiled pixel shader
		vector<char> compiledPixelShader;
		Utility::LoadBinaryFile(L"Content\\Shaders\\AddressingModesDemoPS.cso", compiledPixelShader);
		ThrowIfFailed(mGame->Direct3DDevice()->CreatePixelShader(&compiledPixelShader[0], compiledPixelShader.size(), nullptr, mPixelShader.ReleaseAndGetAddressOf()), "ID3D11Device::CreatedPixelShader() failed.");

		// Create an input layout
		D3D11_INPUT_ELEMENT_DESC inputElementDescriptions[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};

		ThrowIfFailed(mGame->Direct3DDevice()->CreateInputLayout(inputElementDescriptions, ARRAYSIZE(inputElementDescriptions), &compiledVertexShader[0], compiledVertexShader.size(), mInputLayout.ReleaseAndGetAddressOf()), "ID3D11Device::CreateInputLayout() failed.");
				
		const float size = 10.0f;
		const float halfSize = size / 2.0f;

		// Create a vertex buffer
		VertexPositionTexture vertices[] =
		{
			VertexPositionTexture(XMFLOAT4(-halfSize, 1.0f, 0.0, 1.0f), XMFLOAT2(0.0f, 3.0f)),
			VertexPositionTexture(XMFLOAT4(-halfSize, size + 1.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 0.0f)),
			VertexPositionTexture(XMFLOAT4(halfSize, size + 1.0f, 0.0f, 1.0f), XMFLOAT2(3.0f, 0.0f)),
			VertexPositionTexture(XMFLOAT4(halfSize, 1.0f, 0.0f, 1.0f), XMFLOAT2(3.0f, 3.0f))
		};

		CreateVertexBuffer(vertices, ARRAYSIZE(vertices), mVertexBuffer.ReleaseAndGetAddressOf());

		// Create an index buffer
		uint32_t indices[] =
		{
			0, 1, 2,
			0, 2, 3
		};

		mIndexCount = ARRAYSIZE(indices);
		CreateIndexBuffer(indices, mIndexCount, mIndexBuffer.ReleaseAndGetAddressOf());

		// Create constant buffers
		D3D11_BUFFER_DESC constantBufferDesc = { 0 };
		constantBufferDesc.ByteWidth = sizeof(CBufferPerObject);
		constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		ThrowIfFailed(mGame->Direct3DDevice()->CreateBuffer(&constantBufferDesc, nullptr, mCBufferPerObject.ReleaseAndGetAddressOf()), "ID3D11Device::CreateBuffer() failed.");

		// Load a texture
		wstring textureName = L"Content\\Textures\\Cover.jpg";
		ThrowIfFailed(DirectX::CreateWICTextureFromFile(mGame->Direct3DDevice(), mGame->Direct3DDeviceContext(), textureName.c_str(), nullptr, mColorTexture.ReleaseAndGetAddressOf()), "CreateWICTextureFromFile() failed.");

		// Create texture samplers
		for (AddressingMode mode = AddressingMode(0); mode < AddressingMode::End; mode = AddressingMode(static_cast<int>(mode) + 1))
		{
			D3D11_SAMPLER_DESC samplerDesc;
			ZeroMemory(&samplerDesc, sizeof(samplerDesc));
			samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;

			switch (mode)
			{
			case AddressingMode::Wrap:
				samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
				samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
				samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
				break;

			case AddressingMode::Mirror:
				samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
				samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
				samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
				break;

			case AddressingMode::Clamp:
				samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
				samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
				samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
				break;

			case AddressingMode::Border:
				samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
				samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
				samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
				memcpy(samplerDesc.BorderColor, &Colors::Blue[0], sizeof(XMVECTORF32));
				break;

			default:
				throw GameException("Unsupported texture addressing mode.");
			}

			ThrowIfFailed(mGame->Direct3DDevice()->CreateSamplerState(&samplerDesc, mTextureSamplersByAddressingMode[mode].ReleaseAndGetAddressOf()), "ID3D11Device::CreateSamplerState() failed.");
		}

		mKeyboard = reinterpret_cast<KeyboardComponent*>(mGame->Services().GetService(KeyboardComponent::TypeIdClass()));
	}

	void AddressingModesDemo::Update(const GameTime& gameTime)
	{
		UNREFERENCED_PARAMETER(gameTime);

		if (mKeyboard != nullptr)
		{
			if (mKeyboard->WasKeyPressedThisFrame(Keys::Space))
			{
				AddressingMode activeMode = AddressingMode(static_cast<int>(mActiveAddressingMode) + 1);
				if (activeMode >= AddressingMode::End)
				{
					activeMode = AddressingMode(0);
				}

				mActiveAddressingMode = activeMode;
			}
		}
	}

	void AddressingModesDemo::Draw(const GameTime& gameTime)
	{
		UNREFERENCED_PARAMETER(gameTime);

		ID3D11DeviceContext* direct3DDeviceContext = mGame->Direct3DDeviceContext();
		direct3DDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		direct3DDeviceContext->IASetInputLayout(mInputLayout.Get());

		UINT stride = sizeof(VertexPositionTexture);
		UINT offset = 0;
		direct3DDeviceContext->IASetVertexBuffers(0, 1, mVertexBuffer.GetAddressOf(), &stride, &offset);
		direct3DDeviceContext->IASetIndexBuffer(mIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
		direct3DDeviceContext->VSSetShader(mVertexShader.Get(), nullptr, 0);
		direct3DDeviceContext->PSSetShader(mPixelShader.Get(), nullptr, 0);

		XMMATRIX worldMatrix = XMLoadFloat4x4(&mWorldMatrix);
		XMMATRIX wvp = worldMatrix * mCamera->ViewProjectionMatrix();
		wvp = XMMatrixTranspose(wvp);
		XMStoreFloat4x4(&mCBufferPerObjectData.WorldViewProjection, wvp);

		direct3DDeviceContext->UpdateSubresource(mCBufferPerObject.Get(), 0, nullptr, &mCBufferPerObjectData, 0, 0);
		direct3DDeviceContext->VSSetConstantBuffers(0, 1, mCBufferPerObject.GetAddressOf());
		direct3DDeviceContext->PSSetShaderResources(0, 1, mColorTexture.GetAddressOf());
		direct3DDeviceContext->PSSetSamplers(0, 1, mTextureSamplersByAddressingMode[mActiveAddressingMode].GetAddressOf());

		direct3DDeviceContext->DrawIndexed(mIndexCount, 0, 0);
	}

	void AddressingModesDemo::CreateVertexBuffer(VertexPositionTexture* vertices, uint32_t vertexCount, ID3D11Buffer** vertexBuffer) const
	{
		D3D11_BUFFER_DESC vertexBufferDesc = { 0 };
		vertexBufferDesc.ByteWidth = sizeof(VertexPositionTexture) * vertexCount;
		vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA vertexSubResourceData = { 0 };
		vertexSubResourceData.pSysMem = vertices;
		ThrowIfFailed(mGame->Direct3DDevice()->CreateBuffer(&vertexBufferDesc, &vertexSubResourceData, vertexBuffer), "ID3D11Device::CreateBuffer() failed.");
	}

	void AddressingModesDemo::CreateIndexBuffer(uint32_t* indices, uint32_t indexCount, ID3D11Buffer** indexBuffer) const
	{
		D3D11_BUFFER_DESC indexBufferDesc = { 0 };
		indexBufferDesc.ByteWidth = sizeof(uint32_t) * indexCount;
		indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
		indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

		D3D11_SUBRESOURCE_DATA indexSubResourceData = { 0 };
		indexSubResourceData.pSysMem = indices;
		ThrowIfFailed(mGame->Direct3DDevice()->CreateBuffer(&indexBufferDesc, &indexSubResourceData, indexBuffer), "ID3D11Device::CreateBuffer() failed.");
	}
}