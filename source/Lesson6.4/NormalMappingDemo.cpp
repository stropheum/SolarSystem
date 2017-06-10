#include "pch.h"

using namespace std;
using namespace Library;
using namespace DirectX;

namespace Rendering
{
	RTTI_DEFINITIONS(NormalMappingDemo)

	const XMFLOAT2 NormalMappingDemo::LightRotationRate = XMFLOAT2(XM_2PI, XM_2PI);
	const float NormalMappingDemo::LightModulationRate = UCHAR_MAX;

	NormalMappingDemo::NormalMappingDemo(Game& game, const shared_ptr<Camera>& camera) :
		DrawableGameComponent(game, camera), mWorldMatrix(MatrixHelper::Identity), mDirectionalLight(game),
		mRenderStateHelper(game), mVertexCount(0), mTextPosition(0.0f, 40.0f), mUseRealNormalMap(true)
	{
	}

	void NormalMappingDemo::Initialize()
	{
		// Load a compiled vertex shader
		vector<char> compiledVertexShader;
		Utility::LoadBinaryFile(L"Content\\Shaders\\NormalMappingDemoVS.cso", compiledVertexShader);
		ThrowIfFailed(mGame->Direct3DDevice()->CreateVertexShader(&compiledVertexShader[0], compiledVertexShader.size(), nullptr, mVertexShader.ReleaseAndGetAddressOf()), "ID3D11Device::CreatedVertexShader() failed.");

		// Load a compiled pixel shader
		vector<char> compiledPixelShader;
		Utility::LoadBinaryFile(L"Content\\Shaders\\NormalMappingDemoPS.cso", compiledPixelShader);
		ThrowIfFailed(mGame->Direct3DDevice()->CreatePixelShader(&compiledPixelShader[0], compiledPixelShader.size(), nullptr, mPixelShader.ReleaseAndGetAddressOf()), "ID3D11Device::CreatedPixelShader() failed.");

		// Create an input layout
		D3D11_INPUT_ELEMENT_DESC inputElementDescriptions[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};

		ThrowIfFailed(mGame->Direct3DDevice()->CreateInputLayout(inputElementDescriptions, ARRAYSIZE(inputElementDescriptions), &compiledVertexShader[0], compiledVertexShader.size(), mInputLayout.ReleaseAndGetAddressOf()), "ID3D11Device::CreateInputLayout() failed.");

		// Create a vertex buffer
		VertexPositionTextureNormalTangent vertices[] =
		{
			VertexPositionTextureNormalTangent(XMFLOAT4(-0.5f, 0.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 1.0f), Vector3Helper::Backward, Vector3Helper::Right),
			VertexPositionTextureNormalTangent(XMFLOAT4(-0.5f, 1.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 0.0f), Vector3Helper::Backward, Vector3Helper::Right),
			VertexPositionTextureNormalTangent(XMFLOAT4(0.5f, 1.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 0.0f), Vector3Helper::Backward, Vector3Helper::Right),

			VertexPositionTextureNormalTangent(XMFLOAT4(-0.5f, 0.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 1.0f), Vector3Helper::Backward, Vector3Helper::Right),
			VertexPositionTextureNormalTangent(XMFLOAT4(0.5f, 1.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 0.0f), Vector3Helper::Backward, Vector3Helper::Right),
			VertexPositionTextureNormalTangent(XMFLOAT4(0.5f, 0.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 1.0f), Vector3Helper::Backward, Vector3Helper::Right),
		};

		mVertexCount = static_cast<uint32_t>(ARRAYSIZE(vertices));
		CreateVertexBuffer(vertices, mVertexCount, &mVertexBuffer);

		// Create constant buffers
		D3D11_BUFFER_DESC constantBufferDesc = { 0 };
		constantBufferDesc.ByteWidth = sizeof(VSCBufferPerObject);
		constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;	
		ThrowIfFailed(mGame->Direct3DDevice()->CreateBuffer(&constantBufferDesc, nullptr, mVSCBufferPerObject.ReleaseAndGetAddressOf()), "ID3D11Device::CreateBuffer() failed.");

		constantBufferDesc.ByteWidth = sizeof(PSCBufferPerFrame);
		ThrowIfFailed(mGame->Direct3DDevice()->CreateBuffer(&constantBufferDesc, nullptr, mPSCBufferPerFrame.ReleaseAndGetAddressOf()), "ID3D11Device::CreateBuffer() failed.");

		constantBufferDesc.ByteWidth = sizeof(PSCBufferPerObject);
		ThrowIfFailed(mGame->Direct3DDevice()->CreateBuffer(&constantBufferDesc, nullptr, mPSCBufferPerObject.ReleaseAndGetAddressOf()), "ID3D11Device::CreateBuffer() failed.");

		// Load a texture
		std::wstring textureName = L"Content\\Textures\\Blocks_COLOR_RGB.png";
		ThrowIfFailed(DirectX::CreateWICTextureFromFile(mGame->Direct3DDevice(), mGame->Direct3DDeviceContext(), textureName.c_str(), nullptr, mColorTexture.ReleaseAndGetAddressOf()), "CreateWICTextureFromFile() failed.");

		textureName = L"Content\\Textures\\Blocks_NORM.png";
		ThrowIfFailed(DirectX::CreateWICTextureFromFile(mGame->Direct3DDevice(), mGame->Direct3DDeviceContext(), textureName.c_str(), nullptr, mRealNormalMap.ReleaseAndGetAddressOf()), "CreateWICTextureFromFile() failed.");

		textureName = L"Content\\Textures\\DefaultNormalMap.png";
		ThrowIfFailed(DirectX::CreateWICTextureFromFile(mGame->Direct3DDevice(), mGame->Direct3DDeviceContext(), textureName.c_str(), nullptr, mBlankNormalMap.ReleaseAndGetAddressOf()), "CreateWICTextureFromFile() failed.");

		mActiveNormalMap = mRealNormalMap;

		// Create text rendering helpers
		mSpriteBatch = std::make_unique<SpriteBatch>(mGame->Direct3DDeviceContext());
		mSpriteFont = std::make_unique<SpriteFont>(mGame->Direct3DDevice(), L"Content\\Fonts\\Arial_14_Regular.spritefont");

		// Retrieve the keyboard service
		mKeyboard = reinterpret_cast<KeyboardComponent*>(mGame->Services().GetService(KeyboardComponent::TypeIdClass()));

		// Setup a proxy model for the directional light
		mProxyModel = make_unique<ProxyModel>(*mGame, mCamera, "Content\\Models\\DirectionalLightProxy.obj.bin", 0.5f);
		mProxyModel->Initialize();
		mProxyModel->SetPosition(10.0f, 0.0, 0.0f);
		mProxyModel->ApplyRotation(XMMatrixRotationY(XM_PIDIV2));

		mPSCBufferPerFrameData.LightDirection = mDirectionalLight.DirectionToLight();

		// Update the pixel shader per-object constant buffer
		mGame->Direct3DDeviceContext()->UpdateSubresource(mPSCBufferPerObject.Get(), 0, nullptr, &mPSCBufferPerObjectData, 0, 0);

		XMStoreFloat4x4(&mWorldMatrix, XMMatrixScaling(10.0f, 10.0f, 10.0f));
	}

	void NormalMappingDemo::Update(const GameTime& gameTime)
	{
		if (mKeyboard != nullptr)
		{
			if (mKeyboard->WasKeyPressedThisFrame(Keys::Space))
			{
				mUseRealNormalMap = !mUseRealNormalMap;
				mActiveNormalMap = (mUseRealNormalMap ? mRealNormalMap : mBlankNormalMap);
			}

			UpdateAmbientLight(gameTime);
			UpdateDirectionalLight(gameTime);
			UpdateSpecularLight(gameTime);
		}

		mProxyModel->Update(gameTime);
	}

	void NormalMappingDemo::Draw(const GameTime& gameTime)
	{
		ID3D11DeviceContext* direct3DDeviceContext = mGame->Direct3DDeviceContext();
		direct3DDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		direct3DDeviceContext->IASetInputLayout(mInputLayout.Get());

		UINT stride = sizeof(VertexPositionTextureNormalTangent);
		UINT offset = 0;
		direct3DDeviceContext->IASetVertexBuffers(0, 1, mVertexBuffer.GetAddressOf(), &stride, &offset);		

		direct3DDeviceContext->VSSetShader(mVertexShader.Get(), nullptr, 0);
		direct3DDeviceContext->PSSetShader(mPixelShader.Get(), nullptr, 0);

		XMMATRIX worldMatrix = XMLoadFloat4x4(&mWorldMatrix);
		XMMATRIX wvp = worldMatrix * mCamera->ViewProjectionMatrix();
		XMStoreFloat4x4(&mVSCBufferPerObjectData.WorldViewProjection, XMMatrixTranspose(wvp));
		XMStoreFloat4x4(&mVSCBufferPerObjectData.World, XMMatrixTranspose(worldMatrix));
		direct3DDeviceContext->UpdateSubresource(mVSCBufferPerObject.Get(), 0, nullptr, &mVSCBufferPerObjectData, 0, 0);

		direct3DDeviceContext->VSSetConstantBuffers(0, 1, mVSCBufferPerObject.GetAddressOf());

		mPSCBufferPerFrameData.CameraPosition = mCamera->Position();
		direct3DDeviceContext->UpdateSubresource(mPSCBufferPerFrame.Get(), 0, nullptr, &mPSCBufferPerFrameData, 0, 0);

		direct3DDeviceContext->VSSetConstantBuffers(0, 1, mVSCBufferPerObject.GetAddressOf());

		ID3D11Buffer* PSConstantBuffers[] = { mPSCBufferPerFrame.Get(), mPSCBufferPerObject.Get() };
		direct3DDeviceContext->PSSetConstantBuffers(0, ARRAYSIZE(PSConstantBuffers), PSConstantBuffers);
		
		ID3D11ShaderResourceView* PSShaderResources[] = { mColorTexture.Get(), mActiveNormalMap.Get() };
		direct3DDeviceContext->PSSetShaderResources(0, ARRAYSIZE(PSShaderResources), PSShaderResources);
		direct3DDeviceContext->PSSetSamplers(0, 1, SamplerStates::TrilinearWrap.GetAddressOf());

		direct3DDeviceContext->Draw(mVertexCount, 0);

		mProxyModel->Draw(gameTime);

		// Draw help text
		mRenderStateHelper.SaveAll();
		mSpriteBatch->Begin();

		wostringstream helpLabel;
		helpLabel << "Ambient Intensity (+PgUp/-PgDn): " << mPSCBufferPerFrameData.AmbientColor.x << "\n";
		helpLabel << L"Specular Intensity (+Insert/-Delete): " << mPSCBufferPerObjectData.SpecularColor.x << "\n";
		helpLabel << L"Specular Power (+O/-P): " << mPSCBufferPerObjectData.SpecularPower << "\n";
		helpLabel << L"Directional Light Intensity (+Home/-End): " << mPSCBufferPerFrameData.LightColor.x << "\n";
		helpLabel << L"Rotate Directional Light (Arrow Keys)" << "\n";		
		helpLabel << L"Use Normal Map (Space): " << (mUseRealNormalMap ? "Yes" : "No") << "\n";
		helpLabel << L"Toggle Grid (G)";
	
		mSpriteFont->DrawString(mSpriteBatch.get(), helpLabel.str().c_str(), mTextPosition);
		mSpriteBatch->End();
		mRenderStateHelper.RestoreAll();
	}

	void NormalMappingDemo::CreateVertexBuffer(VertexPositionTextureNormalTangent* vertices, uint32_t vertexCount, ID3D11Buffer** vertexBuffer) const
	{
		D3D11_BUFFER_DESC vertexBufferDesc = { 0 };
		vertexBufferDesc.ByteWidth = sizeof(VertexPositionTextureNormalTangent) * vertexCount;
		vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA vertexSubResourceData = { 0 };
		vertexSubResourceData.pSysMem = vertices;
		ThrowIfFailed(mGame->Direct3DDevice()->CreateBuffer(&vertexBufferDesc, &vertexSubResourceData, vertexBuffer), "ID3D11Device::CreateBuffer() failed.");
	}

	void NormalMappingDemo::UpdateAmbientLight(const GameTime& gameTime)
	{
		static float ambientIntensity = mPSCBufferPerFrameData.AmbientColor.x;

		assert(mKeyboard != nullptr);

		if (mKeyboard->IsKeyDown(Keys::PageUp) && ambientIntensity < 1.0f)
		{
			ambientIntensity += gameTime.ElapsedGameTimeSeconds().count();
			ambientIntensity = min(ambientIntensity, 1.0f);

			// No need to update the associated cbuffer here, as it is updated each frame within Draw()
			mPSCBufferPerFrameData.AmbientColor = XMFLOAT4(ambientIntensity, ambientIntensity, ambientIntensity, 1.0f);
		}
		else if (mKeyboard->IsKeyDown(Keys::PageDown) && ambientIntensity > 0.0f)
		{
			ambientIntensity -= gameTime.ElapsedGameTimeSeconds().count();
			ambientIntensity = max(ambientIntensity, 0.0f);

			mPSCBufferPerFrameData.AmbientColor = XMFLOAT4(ambientIntensity, ambientIntensity, ambientIntensity, 1.0f);
		}
	}

	void NormalMappingDemo::UpdateDirectionalLight(const GameTime& gameTime)
	{
		static float directionalIntensity = mPSCBufferPerFrameData.LightColor.x;

		assert(mKeyboard != nullptr);

		float elapsedTime = gameTime.ElapsedGameTimeSeconds().count();

		// Update directional light intensity		
		if (mKeyboard->IsKeyDown(Keys::Home) && directionalIntensity < 1.0f)
		{
			directionalIntensity += elapsedTime;
			directionalIntensity = min(directionalIntensity, 1.0f);

			mPSCBufferPerFrameData.LightColor = XMFLOAT4(directionalIntensity, directionalIntensity, directionalIntensity, 1.0f);
			mDirectionalLight.SetColor(mPSCBufferPerFrameData.LightColor);
		}
		else if (mKeyboard->IsKeyDown(Keys::End) && directionalIntensity > 0.0f)
		{
			directionalIntensity -= elapsedTime;
			directionalIntensity = max(directionalIntensity, 0.0f);

			mPSCBufferPerFrameData.LightColor = XMFLOAT4(directionalIntensity, directionalIntensity, directionalIntensity, 1.0f);
			mDirectionalLight.SetColor(mPSCBufferPerFrameData.LightColor);
		}

		// Rotate directional light
		XMFLOAT2 rotationAmount = Vector2Helper::Zero;
		if (mKeyboard->IsKeyDown(Keys::Left))
		{
			rotationAmount.x += LightRotationRate.x * elapsedTime;
		}
		if (mKeyboard->IsKeyDown(Keys::Right))
		{
			rotationAmount.x -= LightRotationRate.x * elapsedTime;
		}
		if (mKeyboard->IsKeyDown(Keys::Up))
		{
			rotationAmount.y += LightRotationRate.y * elapsedTime;
		}
		if (mKeyboard->IsKeyDown(Keys::Down))
		{
			rotationAmount.y -= LightRotationRate.y * elapsedTime;
		}

		XMMATRIX lightRotationMatrix = XMMatrixIdentity();
		if (rotationAmount.x != 0)
		{
			lightRotationMatrix = XMMatrixRotationY(rotationAmount.x);
		}

		if (rotationAmount.y != 0)
		{
			lightRotationMatrix *= XMMatrixRotationAxis(mDirectionalLight.RightVector(), rotationAmount.y);
		}

		if (rotationAmount.x != 0.0f || rotationAmount.y != 0.0f)
		{
			mDirectionalLight.ApplyRotation(lightRotationMatrix);
			mProxyModel->ApplyRotation(lightRotationMatrix);
			mPSCBufferPerFrameData.LightDirection = mDirectionalLight.DirectionToLight();
		}

		// No need to update the associated cbuffer here, as it is updated each frame within Draw()
	}

	void NormalMappingDemo::UpdateSpecularLight(const GameTime& gameTime)
	{
		static float specularIntensity = mPSCBufferPerObjectData.SpecularColor.x;
		bool updateCBuffer = false;

		if (mKeyboard->IsKeyDown(Keys::Insert) && specularIntensity < 1.0f)
		{
			specularIntensity += gameTime.ElapsedGameTimeSeconds().count();
			specularIntensity = min(specularIntensity, 1.0f);

			mPSCBufferPerObjectData.SpecularColor = XMFLOAT3(specularIntensity, specularIntensity, specularIntensity);
			updateCBuffer = true;
		}

		if (mKeyboard->IsKeyDown(Keys::Delete) && specularIntensity > 0.0f)
		{
			specularIntensity -= gameTime.ElapsedGameTimeSeconds().count();
			specularIntensity = max(specularIntensity, 0.0f);

			mPSCBufferPerObjectData.SpecularColor = XMFLOAT3(specularIntensity, specularIntensity, specularIntensity);
			updateCBuffer = true;
		}

		static float specularPower = mPSCBufferPerObjectData.SpecularPower;

		if (mKeyboard->IsKeyDown(Keys::O) && specularPower < UCHAR_MAX)
		{
			specularPower += LightModulationRate * gameTime.ElapsedGameTimeSeconds().count();
			specularPower = min(specularPower, static_cast<float>(UCHAR_MAX));

			mPSCBufferPerObjectData.SpecularPower = specularPower;
			updateCBuffer = true;
		}

		if (mKeyboard->IsKeyDown(Keys::P) && specularPower > 1.0f)
		{
			specularPower -= LightModulationRate * gameTime.ElapsedGameTimeSeconds().count();
			specularPower = max(specularPower, 1.0f);

			mPSCBufferPerObjectData.SpecularPower = specularPower;
			updateCBuffer = true;
		}

		if (updateCBuffer)
		{
			mGame->Direct3DDeviceContext()->UpdateSubresource(mPSCBufferPerObject.Get(), 0, nullptr, &mPSCBufferPerObjectData, 0, 0);
		}
	}
}