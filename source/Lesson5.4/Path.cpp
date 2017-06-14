#include "pch.h"
#include "Path.h"
#include "CelestialBody.h"

using namespace DirectX;

namespace Library
{
	RTTI_DEFINITIONS(Path)

	const UINT Path::DefaultSize = 360;
	const UINT Path::DefaultScale = 16;
	const XMFLOAT4 Path::DefaultColor = XMFLOAT4(0.0f, 0.5f, 0.5f, 0.5f);

	Path::Path(Game& game, const std::shared_ptr<Camera>& camera, const float& radius)
		: DrawableGameComponent(game, camera), mVertexShader(nullptr), mPixelShader(nullptr), mInputLayout(nullptr), mVertexBuffer(nullptr),
		mVertexCBufferPerObject(nullptr), mVertexCBufferPerObjectData(),
		mPosition(Vector3Helper::Zero), mSize(DefaultSize), mScale(DefaultScale), mColor(DefaultColor), mWorldMatrix(MatrixHelper::Identity), mRadius(radius * CelestialBody::OrbitalScale)
	{
	}

	Path::Path(Game& game, const std::shared_ptr<Camera>& camera, const float& radius, UINT size, UINT scale, const XMFLOAT4& color)
		: DrawableGameComponent(game, camera), mVertexShader(nullptr), mPixelShader(nullptr), mInputLayout(nullptr), mVertexBuffer(nullptr),
		mVertexCBufferPerObject(nullptr), mVertexCBufferPerObjectData(),
		mPosition(Vector3Helper::Zero), mSize(size), mScale(scale), mColor(color), mWorldMatrix(MatrixHelper::Identity), mRadius(radius * CelestialBody::OrbitalScale)
	{
	}

	const XMFLOAT3& Path::Position() const
	{
		return mPosition;
	}

	const XMFLOAT4 Path::Color() const
	{
		return mColor;
	}

	const UINT Path::Size() const
	{
		return mSize;
	}

	const UINT Path::Scale() const
	{
		return mScale;
	}

	void Path::SetPosition(const XMFLOAT3& position)
	{
		mPosition = position;

		XMMATRIX translation = XMMatrixTranslation(mPosition.x, mPosition.y, mPosition.z);
		XMStoreFloat4x4(&mWorldMatrix, translation);
	}

	void Path::SetPosition(float x, float y, float z)
	{
		mPosition.x = x;
		mPosition.y = y;
		mPosition.z = z;

		XMMATRIX translation = XMMatrixTranslation(mPosition.x, mPosition.y, mPosition.z);
		XMStoreFloat4x4(&mWorldMatrix, translation);
	}

	void Path::SetColor(const XMFLOAT4& color)
	{
		mColor = color;
		InitializePath();
	}

	void Path::SetSize(UINT size)
	{
		mSize = size;
		InitializePath();
	}

	void Path::SetScale(UINT scale)
	{
		mScale = scale;
		InitializePath();
	}

	void Path::Initialize()
	{
		// Load a compiled vertex shader
		std::vector<char> compiledVertexShader;
		Utility::LoadBinaryFile(L"Content\\Shaders\\BasicVS.cso", compiledVertexShader);
		ThrowIfFailed(mGame->Direct3DDevice()->CreateVertexShader(&compiledVertexShader[0], compiledVertexShader.size(), nullptr, mVertexShader.GetAddressOf()), "ID3D11Device::CreatedVertexShader() failed.");

		// Load a compiled pixel shader
		std::vector<char> compiledPixelShader;
		Utility::LoadBinaryFile(L"Content\\Shaders\\BasicPS.cso", compiledPixelShader);
		ThrowIfFailed(mGame->Direct3DDevice()->CreatePixelShader(&compiledPixelShader[0], compiledPixelShader.size(), nullptr, mPixelShader.GetAddressOf()), "ID3D11Device::CreatedPixelShader() failed.");

		// Create an input layout
		D3D11_INPUT_ELEMENT_DESC inputElementDescriptions[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};

		ThrowIfFailed(mGame->Direct3DDevice()->CreateInputLayout(inputElementDescriptions, ARRAYSIZE(inputElementDescriptions), &compiledVertexShader[0], compiledVertexShader.size(), mInputLayout.GetAddressOf()), "ID3D11Device::CreateInputLayout() failed.");

		// Create constant buffers
		D3D11_BUFFER_DESC constantBufferDesc = { 0 };
		constantBufferDesc.ByteWidth = sizeof(VertexCBufferPerObject);
		constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		ThrowIfFailed(mGame->Direct3DDevice()->CreateBuffer(&constantBufferDesc, nullptr, mVertexCBufferPerObject.GetAddressOf()), "ID3D11Device::CreateBuffer() failed.");

		InitializePath();
	}

	void Path::Draw(const GameTime& gameTime)
	{
		UNREFERENCED_PARAMETER(gameTime);

		ID3D11DeviceContext* direct3DDeviceContext = mGame->Direct3DDeviceContext();
		direct3DDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
		direct3DDeviceContext->IASetInputLayout(mInputLayout.Get());

		UINT stride = sizeof(VertexPositionColor);
		UINT offset = 0;
		direct3DDeviceContext->IASetVertexBuffers(0, 1, mVertexBuffer.GetAddressOf(), &stride, &offset);

		direct3DDeviceContext->VSSetShader(mVertexShader.Get(), nullptr, 0);
		direct3DDeviceContext->PSSetShader(mPixelShader.Get(), nullptr, 0);

		XMMATRIX worldMatrix = XMLoadFloat4x4(&mWorldMatrix);
		XMMATRIX wvp = worldMatrix * mCamera->ViewProjectionMatrix();
		XMStoreFloat4x4(&mVertexCBufferPerObjectData.WorldViewProjection, XMMatrixTranspose(wvp));

		direct3DDeviceContext->UpdateSubresource(mVertexCBufferPerObject.Get(), 0, nullptr, &mVertexCBufferPerObjectData, 0, 0);
		direct3DDeviceContext->VSSetConstantBuffers(0, 1, mVertexCBufferPerObject.GetAddressOf());

		direct3DDeviceContext->Draw(mSize, 0);
	}

	void Path::InitializePath()
	{
		mVertexBuffer.Reset();

		ID3D11Device* direct3DDevice = GetGame()->Direct3DDevice();
		int length = mSize;
		int size = sizeof(VertexPositionColor) * length;
		std::unique_ptr<VertexPositionColor> vertexData(new VertexPositionColor[length]);
		VertexPositionColor* vertices = vertexData.get();

		for (unsigned int i = 0, j = 0; i < mSize; i++, j = i)
		{
			float positionX = mRadius * cos(2 * XM_PI * i / mSize);
			float positionY = mRadius * sin(2 * XM_PI * i / mSize);
			vertices[i] = VertexPositionColor(XMFLOAT4(positionX, 0.0f, positionY, 1.0f), mColor);
		}

		D3D11_BUFFER_DESC vertexBufferDesc = { 0 };
		vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
		vertexBufferDesc.ByteWidth = size;
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA vertexSubResourceData = { 0 };
		vertexSubResourceData.pSysMem = vertices;

		ThrowIfFailed(direct3DDevice->CreateBuffer(&vertexBufferDesc, &vertexSubResourceData, mVertexBuffer.GetAddressOf()), "ID3D11Device::CreateBuffer() failed");
	}
}