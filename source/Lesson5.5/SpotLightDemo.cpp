#include "pch.h"

using namespace std;
using namespace Library;
using namespace DirectX;

namespace Rendering
{
	RTTI_DEFINITIONS(SpotLightDemo)

	const float SpotLightDemo::ModelRotationRate = XM_PI;
	const float SpotLightDemo::LightModulationRate = UCHAR_MAX;
	const float SpotLightDemo::LightMovementRate = 10.0f;
	const XMFLOAT2 SpotLightDemo::LightRotationRate = XMFLOAT2(XM_2PI, XM_2PI);

	SpotLightDemo::SpotLightDemo(Game& game, const shared_ptr<Camera>& camera) :
		DrawableGameComponent(game, camera),
		mWorldMatrix(MatrixHelper::Identity), mSpotLight(game, XMFLOAT3(0.0f, 5.0f, 20.0f), 50.0f),
		mRenderStateHelper(game), mVertexCount(0), mTextPosition(0.0f, 40.0f)
	{
	}

	void SpotLightDemo::Initialize()
	{
		// Load a compiled vertex shader
		vector<char> compiledVertexShader;
		Utility::LoadBinaryFile(L"Content\\Shaders\\SpotLightDemoVS.cso", compiledVertexShader);
		ThrowIfFailed(mGame->Direct3DDevice()->CreateVertexShader(&compiledVertexShader[0], compiledVertexShader.size(), nullptr, mVertexShader.ReleaseAndGetAddressOf()), "ID3D11Device::CreatedVertexShader() failed.");

		// Load a compiled pixel shader
		vector<char> compiledPixelShader;
		Utility::LoadBinaryFile(L"Content\\Shaders\\SpotLightDemoPS.cso", compiledPixelShader);
		ThrowIfFailed(mGame->Direct3DDevice()->CreatePixelShader(&compiledPixelShader[0], compiledPixelShader.size(), nullptr, mPixelShader.ReleaseAndGetAddressOf()), "ID3D11Device::CreatedPixelShader() failed.");

		// Create an input layout
		D3D11_INPUT_ELEMENT_DESC inputElementDescriptions[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		ThrowIfFailed(mGame->Direct3DDevice()->CreateInputLayout(inputElementDescriptions, ARRAYSIZE(inputElementDescriptions), &compiledVertexShader[0], compiledVertexShader.size(), mInputLayout.ReleaseAndGetAddressOf()), "ID3D11Device::CreateInputLayout() failed.");

		// Create a vertex buffer
		VertexPositionTextureNormal vertices[] =
		{
			VertexPositionTextureNormal(XMFLOAT4(-0.5f, 0.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 1.0f), Vector3Helper::Backward),
			VertexPositionTextureNormal(XMFLOAT4(-0.5f, 1.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 0.0f), Vector3Helper::Backward),
			VertexPositionTextureNormal(XMFLOAT4(0.5f, 1.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 0.0f), Vector3Helper::Backward),

			VertexPositionTextureNormal(XMFLOAT4(-0.5f, 0.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 1.0f), Vector3Helper::Backward),
			VertexPositionTextureNormal(XMFLOAT4(0.5f, 1.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 0.0f), Vector3Helper::Backward),
			VertexPositionTextureNormal(XMFLOAT4(0.5f, 0.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 1.0f), Vector3Helper::Backward),
		};

		mVertexCount = static_cast<uint32_t>(ARRAYSIZE(vertices));
		CreateVertexBuffer(vertices, mVertexCount, &mVertexBuffer);

		// Create constant buffers
		D3D11_BUFFER_DESC constantBufferDesc = { 0 };
		constantBufferDesc.ByteWidth = sizeof(VSCBufferPerFrame);
		constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		ThrowIfFailed(mGame->Direct3DDevice()->CreateBuffer(&constantBufferDesc, nullptr, mVSCBufferPerFrame.ReleaseAndGetAddressOf()), "ID3D11Device::CreateBuffer() failed.");

		constantBufferDesc.ByteWidth = sizeof(VSCBufferPerObject);
		ThrowIfFailed(mGame->Direct3DDevice()->CreateBuffer(&constantBufferDesc, nullptr, mVSCBufferPerObject.ReleaseAndGetAddressOf()), "ID3D11Device::CreateBuffer() failed.");

		constantBufferDesc.ByteWidth = sizeof(PSCBufferPerFrame);
		ThrowIfFailed(mGame->Direct3DDevice()->CreateBuffer(&constantBufferDesc, nullptr, mPSCBufferPerFrame.ReleaseAndGetAddressOf()), "ID3D11Device::CreateBuffer() failed.");

		constantBufferDesc.ByteWidth = sizeof(PSCBufferPerObject);
		ThrowIfFailed(mGame->Direct3DDevice()->CreateBuffer(&constantBufferDesc, nullptr, mPSCBufferPerObject.ReleaseAndGetAddressOf()), "ID3D11Device::CreateBuffer() failed.");

		// Load textures for the color and specular maps
		wstring textureName = L"Content\\Textures\\Checkerboard.png";
		ThrowIfFailed(CreateWICTextureFromFile(mGame->Direct3DDevice(), textureName.c_str(), nullptr, mColorTexture.ReleaseAndGetAddressOf()), "CreateDDSTextureFromFile() failed.");

		textureName = L"Content\\Textures\\CheckerboardSpecularMap.png";
		ThrowIfFailed(CreateWICTextureFromFile(mGame->Direct3DDevice(), textureName.c_str(), nullptr, mSpecularMap.ReleaseAndGetAddressOf()), "CreateWICTextureFromFile() failed.");

		// Create text rendering helpers
		mSpriteBatch = make_unique<SpriteBatch>(mGame->Direct3DDeviceContext());
		mSpriteFont = make_unique<SpriteFont>(mGame->Direct3DDevice(), L"Content\\Fonts\\Arial_14_Regular.spritefont");

		// Retrieve the keyboard service
		mKeyboard = reinterpret_cast<KeyboardComponent*>(mGame->Services().GetService(KeyboardComponent::TypeIdClass()));

		// Setup the spot light
		mVSCBufferPerFrameData.LightPosition = mSpotLight.Position();
		mVSCBufferPerFrameData.LightRadius = mSpotLight.Radius();		
		mPSCBufferPerFrameData.LightColor = ColorHelper::ToFloat4(mSpotLight.Color(), true);
		mPSCBufferPerFrameData.LightPosition = mSpotLight.Position();
		mPSCBufferPerFrameData.SpotLightInnerAngle = mSpotLight.InnerAngle();
		mPSCBufferPerFrameData.SpotLightOuterAngle = mSpotLight.OuterAngle();
		mPSCBufferPerFrameData.CameraPosition = mCamera->Position();
		mPSCBufferPerFrameData.LightLookAt = mSpotLight.DirectionToLight();
		mGame->Direct3DDeviceContext()->UpdateSubresource(mVSCBufferPerFrame.Get(), 0, nullptr, &mVSCBufferPerFrameData, 0, 0);
		mGame->Direct3DDeviceContext()->UpdateSubresource(mPSCBufferPerObject.Get(), 0, nullptr, &mPSCBufferPerObjectData, 0, 0);

		// Load a proxy model for the point light
		mProxyModel = make_unique<ProxyModel>(*mGame, mCamera, "Content\\Models\\SpotLightProxy.obj.bin", 0.5f);
		mProxyModel->Initialize();
		mProxyModel->SetPosition(mSpotLight.Position());
		mProxyModel->ApplyRotation(XMMatrixRotationX(XM_PIDIV2));

		XMStoreFloat4x4(&mWorldMatrix, XMMatrixScaling(10.0f, 10.0f, 10.0f));
	}

	void SpotLightDemo::Update(const GameTime& gameTime)
	{
		if (mKeyboard != nullptr)
		{
			UpdateAmbientLight(gameTime);
			UpdateSpotLight(gameTime);
			UpdateSpecularLight(gameTime);
		}

		mProxyModel->Update(gameTime);
	}

	void SpotLightDemo::Draw(const GameTime& gameTime)
	{
		ID3D11DeviceContext* direct3DDeviceContext = mGame->Direct3DDeviceContext();
		direct3DDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		direct3DDeviceContext->IASetInputLayout(mInputLayout.Get());

		UINT stride = sizeof(VertexPositionTextureNormal);
		UINT offset = 0;
		direct3DDeviceContext->IASetVertexBuffers(0, 1, mVertexBuffer.GetAddressOf(), &stride, &offset);
		direct3DDeviceContext->IASetIndexBuffer(mIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

		direct3DDeviceContext->VSSetShader(mVertexShader.Get(), nullptr, 0);
		direct3DDeviceContext->PSSetShader(mPixelShader.Get(), nullptr, 0);

		XMMATRIX worldMatrix = XMLoadFloat4x4(&mWorldMatrix);
		XMMATRIX wvp = worldMatrix * mCamera->ViewProjectionMatrix();
		wvp = XMMatrixTranspose(wvp);
		XMStoreFloat4x4(&mVSCBufferPerObjectData.WorldViewProjection, wvp);
		XMStoreFloat4x4(&mVSCBufferPerObjectData.World, XMMatrixTranspose(worldMatrix));
		direct3DDeviceContext->UpdateSubresource(mVSCBufferPerObject.Get(), 0, nullptr, &mVSCBufferPerObjectData, 0, 0);	

		ID3D11Buffer* VSConstantBuffers[] = { mVSCBufferPerFrame.Get(), mVSCBufferPerObject.Get() };
		direct3DDeviceContext->VSSetConstantBuffers(0, ARRAYSIZE(VSConstantBuffers), VSConstantBuffers);

		mPSCBufferPerFrameData.CameraPosition = mCamera->Position();
		direct3DDeviceContext->UpdateSubresource(mPSCBufferPerFrame.Get(), 0, nullptr, &mPSCBufferPerFrameData, 0, 0);

		ID3D11Buffer* PSConstantBuffers[] = { mPSCBufferPerFrame.Get(), mPSCBufferPerObject.Get() };
		direct3DDeviceContext->PSSetConstantBuffers(0, ARRAYSIZE(PSConstantBuffers), PSConstantBuffers);

		ID3D11ShaderResourceView* PSShaderResources[] = { mColorTexture.Get(), mSpecularMap.Get() };
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
		helpLabel << L"Spot Light Intensity (+Home/-End): " << mPSCBufferPerFrameData.LightColor.x << "\n";
		helpLabel << L"Move Spot Light (8/2, 4/6, 3/9)\n";
		helpLabel << L"Rotate Spot Light (Arrow Keys)\n";
		helpLabel << L"Spot Light Radius (+B/-N): " << mSpotLight.Radius() << "\n";
		helpLabel << L"Spot Light Inner Angle(+Z/-X): " << mSpotLight.InnerAngle() << "\n";
		helpLabel << L"Spot Light Outer Angle(+C/-V): " << mSpotLight.OuterAngle() << "\n";
		helpLabel << L"Toggle Grid (G)";
	
		mSpriteFont->DrawString(mSpriteBatch.get(), helpLabel.str().c_str(), mTextPosition);
		mSpriteBatch->End();
		mRenderStateHelper.RestoreAll();
	}

	void SpotLightDemo::CreateVertexBuffer(VertexPositionTextureNormal* vertices, uint32_t vertexCount, ID3D11Buffer** vertexBuffer) const
	{
		D3D11_BUFFER_DESC vertexBufferDesc = { 0 };
		vertexBufferDesc.ByteWidth = sizeof(VertexPositionTextureNormal) * vertexCount;
		vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA vertexSubResourceData = { 0 };
		vertexSubResourceData.pSysMem = &vertices[0];
		ThrowIfFailed(mGame->Direct3DDevice()->CreateBuffer(&vertexBufferDesc, &vertexSubResourceData, vertexBuffer), "ID3D11Device::CreateBuffer() failed.");
	}

	void SpotLightDemo::UpdateAmbientLight(const GameTime& gameTime)
	{
		static float ambientIntensity = mPSCBufferPerFrameData.AmbientColor.x;

		assert(mKeyboard != nullptr);

		if (mKeyboard->IsKeyDown(Keys::PageUp) && ambientIntensity < 1.0f)
		{
			ambientIntensity += gameTime.ElapsedGameTimeSeconds().count();
			ambientIntensity = min(ambientIntensity, 1.0f);

			mPSCBufferPerFrameData.AmbientColor = XMFLOAT4(ambientIntensity, ambientIntensity, ambientIntensity, 1.0f);
			mGame->Direct3DDeviceContext()->UpdateSubresource(mPSCBufferPerFrame.Get(), 0, nullptr, &mPSCBufferPerFrameData, 0, 0);
		}
		else if (mKeyboard->IsKeyDown(Keys::PageDown) && ambientIntensity > 0.0f)
		{
			ambientIntensity -= gameTime.ElapsedGameTimeSeconds().count();
			ambientIntensity = max(ambientIntensity, 0.0f);

			mPSCBufferPerFrameData.AmbientColor = XMFLOAT4(ambientIntensity, ambientIntensity, ambientIntensity, 1.0f);
			mGame->Direct3DDeviceContext()->UpdateSubresource(mPSCBufferPerFrame.Get(), 0, nullptr, &mPSCBufferPerFrameData, 0, 0);
		}
	}

	void SpotLightDemo::UpdateSpotLight(const GameTime& gameTime)
	{
		static float lightIntensity = mPSCBufferPerFrameData.LightColor.x;

		assert(mKeyboard != nullptr);

		float elapsedTime = gameTime.ElapsedGameTimeSeconds().count();
		bool updateCBuffer = false;

		// Update spot light intensity
		if (mKeyboard->IsKeyDown(Keys::Home) && lightIntensity < 1.0f)
		{
			lightIntensity += elapsedTime;
			lightIntensity = min(lightIntensity, 1.0f);

			mPSCBufferPerFrameData.LightColor = XMFLOAT4(lightIntensity, lightIntensity, lightIntensity, 1.0f);
			mSpotLight.SetColor(mPSCBufferPerFrameData.LightColor);
			updateCBuffer = true;
		}
		else if (mKeyboard->IsKeyDown(Keys::End) && lightIntensity > 0.0f)
		{
			lightIntensity -= elapsedTime;
			lightIntensity = max(lightIntensity, 0.0f);

			mPSCBufferPerFrameData.LightColor = XMFLOAT4(lightIntensity, lightIntensity, lightIntensity, 1.0f);
			mSpotLight.SetColor(mPSCBufferPerFrameData.LightColor);
			updateCBuffer = true;
		}

		// Move spot light
		XMFLOAT3 movementAmount = Vector3Helper::Zero;
		if (mKeyboard != nullptr)
		{
			if (mKeyboard->IsKeyDown(Keys::NumPad4))
			{
				movementAmount.x = -1.0f;
			}

			if (mKeyboard->IsKeyDown(Keys::NumPad6))
			{
				movementAmount.x = 1.0f;
			}

			if (mKeyboard->IsKeyDown(Keys::NumPad9))
			{
				movementAmount.y = 1.0f;
			}

			if (mKeyboard->IsKeyDown(Keys::NumPad3))
			{
				movementAmount.y = -1.0f;
			}

			if (mKeyboard->IsKeyDown(Keys::NumPad8))
			{
				movementAmount.z = -1.0f;
			}

			if (mKeyboard->IsKeyDown(Keys::NumPad2))
			{
				movementAmount.z = 1.0f;
			}
		}

		if (movementAmount.x != 0.0f || movementAmount.y != 0.0f || movementAmount.z != 0.0f)
		{
			XMVECTOR movement = XMLoadFloat3(&movementAmount) * LightMovementRate * elapsedTime;
			mSpotLight.SetPosition(mSpotLight.PositionVector() + movement);
			mProxyModel->SetPosition(mSpotLight.Position());
			mVSCBufferPerFrameData.LightPosition = mSpotLight.Position();
			mPSCBufferPerFrameData.LightPosition = mSpotLight.Position();
			updateCBuffer = true;
		}

		// Rotate spot light
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
			lightRotationMatrix *= XMMatrixRotationAxis(mSpotLight.RightVector(), rotationAmount.y);
		}

		if (rotationAmount.x != 0.0f || rotationAmount.y != 0.0f)
		{
			mSpotLight.ApplyRotation(lightRotationMatrix);
			mProxyModel->ApplyRotation(lightRotationMatrix);
			mPSCBufferPerFrameData.LightLookAt = mSpotLight.DirectionToLight();
			updateCBuffer = true;
		}

		// Update the light's radius
		if (mKeyboard->IsKeyDown(Keys::V))
		{
			float radius = mSpotLight.Radius() + LightModulationRate * elapsedTime;
			mSpotLight.SetRadius(radius);
			mVSCBufferPerFrameData.LightRadius = mSpotLight.Radius();
			updateCBuffer = true;
		}

		if (mKeyboard->IsKeyDown(Keys::B))
		{
			float radius = mSpotLight.Radius() - LightModulationRate * elapsedTime;
			radius = max(radius, 0.0f);
			mSpotLight.SetRadius(radius);
			mVSCBufferPerFrameData.LightRadius = mSpotLight.Radius();
			updateCBuffer = true;
		}
		
		// Update inner and outer angles
		static float innerAngle = mSpotLight.InnerAngle();
		if (mKeyboard->IsKeyDown(Keys::Z) && innerAngle < 1.0f)
		{
			innerAngle += elapsedTime;
			innerAngle = min(innerAngle, 1.0f);

			mSpotLight.SetInnerAngle(innerAngle);
			mPSCBufferPerFrameData.SpotLightInnerAngle = mSpotLight.InnerAngle();
			updateCBuffer = true;
		}
		if (mKeyboard->IsKeyDown(Keys::X) && innerAngle > 0.5f)
		{
			innerAngle -= elapsedTime;
			innerAngle = max(innerAngle, 0.5f);

			mSpotLight.SetInnerAngle(innerAngle);
			mPSCBufferPerFrameData.SpotLightInnerAngle = mSpotLight.InnerAngle();
			updateCBuffer = true;
		}

		static float outerAngle = mSpotLight.OuterAngle();
		if (mKeyboard->IsKeyDown(Keys::C) && outerAngle < 0.5f)
		{
			outerAngle += elapsedTime;
			outerAngle = min(outerAngle, 0.5f);

			mSpotLight.SetOuterAngle(outerAngle);
			mPSCBufferPerFrameData.SpotLightOuterAngle = mSpotLight.OuterAngle();
			updateCBuffer = true;
		}
		if (mKeyboard->IsKeyDown(Keys::V) && outerAngle > 0.0f)
		{
			outerAngle -= elapsedTime;
			outerAngle = max(outerAngle, 0.0f);

			mSpotLight.SetOuterAngle(outerAngle);
			mPSCBufferPerFrameData.SpotLightOuterAngle = mSpotLight.OuterAngle();
			updateCBuffer = true;
		}

		if (updateCBuffer)
		{
			mGame->Direct3DDeviceContext()->UpdateSubresource(mVSCBufferPerFrame.Get(), 0, nullptr, &mVSCBufferPerFrameData, 0, 0);
			mGame->Direct3DDeviceContext()->UpdateSubresource(mPSCBufferPerFrame.Get(), 0, nullptr, &mPSCBufferPerFrameData, 0, 0);
		}
	}

	void SpotLightDemo::UpdateSpecularLight(const GameTime& gameTime)
	{
		static float specularIntensity = mPSCBufferPerObjectData.SpecularColor.x;

		assert(mKeyboard != nullptr);

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