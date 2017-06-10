#include "pch.h"

using namespace std;
using namespace Library;
using namespace DirectX;

namespace Rendering
{
	RTTI_DEFINITIONS(BlinnPhongDemo)

	const float BlinnPhongDemo::ModelRotationRate = XM_PI;
	const XMFLOAT2 BlinnPhongDemo::LightRotationRate = XMFLOAT2(XM_2PI, XM_2PI);
	const float BlinnPhongDemo::LightModulationRate = UCHAR_MAX;

	BlinnPhongDemo::BlinnPhongDemo(Game & game, const shared_ptr<Camera>& camera) :
		DrawableGameComponent(game, camera), mWorldMatrix(MatrixHelper::Identity), mDirectionalLight(game),
		mRenderStateHelper(game), mIndexCount(0), mTextPosition(0.0f, 40.0f), mAnimationEnabled(false)
	{
	}

	bool BlinnPhongDemo::AnimationEnabled() const
	{
		return mAnimationEnabled;
	}

	void BlinnPhongDemo::SetAnimationEnabled(bool enabled)
	{
		mAnimationEnabled = enabled;
	}

	void BlinnPhongDemo::Initialize()
	{
		// Load a compiled vertex shader
		vector<char> compiledVertexShader;
		Utility::LoadBinaryFile(L"Content\\Shaders\\BlinnPhongDemoVS.cso", compiledVertexShader);
		ThrowIfFailed(mGame->Direct3DDevice()->CreateVertexShader(&compiledVertexShader[0], compiledVertexShader.size(), nullptr, mVertexShader.ReleaseAndGetAddressOf()), "ID3D11Device::CreatedVertexShader() failed.");

		// Load a compiled pixel shader
		vector<char> compiledPixelShader;
		Utility::LoadBinaryFile(L"Content\\Shaders\\BlinnPhongDemoPS.cso", compiledPixelShader);
		ThrowIfFailed(mGame->Direct3DDevice()->CreatePixelShader(&compiledPixelShader[0], compiledPixelShader.size(), nullptr, mPixelShader.ReleaseAndGetAddressOf()), "ID3D11Device::CreatedPixelShader() failed.");

		// Create an input layout
		D3D11_INPUT_ELEMENT_DESC inputElementDescriptions[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		ThrowIfFailed(mGame->Direct3DDevice()->CreateInputLayout(inputElementDescriptions, ARRAYSIZE(inputElementDescriptions), &compiledVertexShader[0], compiledVertexShader.size(), mInputLayout.ReleaseAndGetAddressOf()), "ID3D11Device::CreateInputLayout() failed.");

		// Load the model
		Library::Model model("Content\\Models\\Sphere.obj.bin");

		// Create vertex and index buffers for the model
		Library::Mesh* mesh = model.Meshes().at(0).get();
		CreateVertexBuffer(*mesh, mVertexBuffer.ReleaseAndGetAddressOf());
		mesh->CreateIndexBuffer(*mGame->Direct3DDevice(), mIndexBuffer.ReleaseAndGetAddressOf());
		mIndexCount = static_cast<uint32_t>(mesh->Indices().size());

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
		wstring textureName = L"Content\\Textures\\Earthatday.dds";
		ThrowIfFailed(CreateDDSTextureFromFile(mGame->Direct3DDevice(), textureName.c_str(), nullptr, mColorAndSpecularMap.ReleaseAndGetAddressOf()), "CreateDDSTextureFromFile() failed.");

		// Create text rendering helpers
		mSpriteBatch = make_unique<SpriteBatch>(mGame->Direct3DDeviceContext());
		mSpriteFont = make_unique<SpriteFont>(mGame->Direct3DDevice(), L"Content\\Fonts\\Arial_14_Regular.spritefont");

		// Retrieve the keyboard service
		mKeyboard = reinterpret_cast<KeyboardComponent*>(mGame->Services().GetService(KeyboardComponent::TypeIdClass()));		

		mProxyModel = make_unique<ProxyModel>(*mGame, mCamera, "Content\\Models\\DirectionalLightProxy.obj.bin", 0.5f);
		mProxyModel->Initialize();
		mProxyModel->SetPosition(10.0f, 0.0, 0.0f);
		mProxyModel->ApplyRotation(XMMatrixRotationY(XM_PIDIV2));
		
		mPSCBufferPerFrameData.LightDirection = mDirectionalLight.DirectionToLight();

		// Update the pixel shader constant buffers
		mGame->Direct3DDeviceContext()->UpdateSubresource(mPSCBufferPerObject.Get(), 0, nullptr, &mPSCBufferPerObjectData, 0, 0);
	}

	void BlinnPhongDemo::Update(const GameTime& gameTime)
	{
		static float angle = 0.0f;

		if (mAnimationEnabled)
		{
			angle += gameTime.ElapsedGameTimeSeconds().count() * ModelRotationRate;
			XMStoreFloat4x4(&mWorldMatrix, XMMatrixRotationY(angle));
		}

		if (mKeyboard != nullptr)
		{
			if (mKeyboard->WasKeyPressedThisFrame(Keys::Space))
			{
				ToggleAnimation();
			}

			UpdateAmbientLight(gameTime);
			UpdateDirectionalLight(gameTime);
			UpdateSpecularLight(gameTime);
		}
		
		mProxyModel->Update(gameTime);
	}

	void BlinnPhongDemo::Draw(const GameTime& gameTime)
	{
		UNREFERENCED_PARAMETER(gameTime);

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

		direct3DDeviceContext->VSSetConstantBuffers(0, 1, mVSCBufferPerObject.GetAddressOf());

		mPSCBufferPerFrameData.CameraPosition = mCamera->Position();
		direct3DDeviceContext->UpdateSubresource(mPSCBufferPerFrame.Get(), 0, nullptr, &mPSCBufferPerFrameData, 0, 0);

		ID3D11Buffer* PSConstantBuffers[] = { mPSCBufferPerFrame.Get(), mPSCBufferPerObject.Get() };
		direct3DDeviceContext->PSSetConstantBuffers(0, ARRAYSIZE(PSConstantBuffers), PSConstantBuffers);

		direct3DDeviceContext->PSSetShaderResources(0, 1, mColorAndSpecularMap.GetAddressOf());
		direct3DDeviceContext->PSSetSamplers(0, 1, SamplerStates::TrilinearWrap.GetAddressOf());

		direct3DDeviceContext->DrawIndexed(mIndexCount, 0, 0);

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
		helpLabel << L"Toggle Grid (G)" << "\n";
		helpLabel << L"Toggle Animation (Space)" << "\n";
	
		mSpriteFont->DrawString(mSpriteBatch.get(), helpLabel.str().c_str(), mTextPosition);
		mSpriteBatch->End();
		mRenderStateHelper.RestoreAll();
	}

	void BlinnPhongDemo::CreateVertexBuffer(const Mesh& mesh, ID3D11Buffer** vertexBuffer) const
	{
		const vector<XMFLOAT3>& sourceVertices = mesh.Vertices();
		const vector<XMFLOAT3>& sourceNormals = mesh.Normals();
		const auto& sourceUVs = mesh.TextureCoordinates().at(0);

		vector<VertexPositionTextureNormal> vertices;
		vertices.reserve(sourceVertices.size());
		for (UINT i = 0; i < sourceVertices.size(); i++)
		{
			const XMFLOAT3& position = sourceVertices.at(i);
			const XMFLOAT3& uv = sourceUVs->at(i);
			const XMFLOAT3& normal = sourceNormals.at(i);

			vertices.push_back(VertexPositionTextureNormal(XMFLOAT4(position.x, position.y, position.z, 1.0f), XMFLOAT2(uv.x, uv.y), normal));
		}
		D3D11_BUFFER_DESC vertexBufferDesc = { 0 };
		vertexBufferDesc.ByteWidth = sizeof(VertexPositionTextureNormal) * static_cast<UINT>(vertices.size());
		vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA vertexSubResourceData = { 0 };
		vertexSubResourceData.pSysMem = &vertices[0];
		ThrowIfFailed(mGame->Direct3DDevice()->CreateBuffer(&vertexBufferDesc, &vertexSubResourceData, vertexBuffer), "ID3D11Device::CreateBuffer() failed.");
	}

	void BlinnPhongDemo::ToggleAnimation()
	{
		mAnimationEnabled = !mAnimationEnabled;
	}

	void BlinnPhongDemo::UpdateAmbientLight(const GameTime& gameTime)
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

	void BlinnPhongDemo::UpdateDirectionalLight(const GameTime& gameTime)
	{
		static float directionalIntensity = mPSCBufferPerFrameData.LightColor.x;

		assert(mKeyboard != nullptr);

		float elapsedTime = gameTime.ElapsedGameTimeSeconds().count();
		bool updateCBuffer = false;

		// Update directional light intensity		
		if (mKeyboard->IsKeyDown(Keys::Home) && directionalIntensity < 1.0f)
		{
			directionalIntensity += elapsedTime;
			directionalIntensity = min(directionalIntensity, 1.0f);

			mPSCBufferPerFrameData.LightColor = XMFLOAT4(directionalIntensity, directionalIntensity, directionalIntensity, 1.0f);
			mDirectionalLight.SetColor(mPSCBufferPerFrameData.LightColor);
			updateCBuffer = true;
		}
		else if (mKeyboard->IsKeyDown(Keys::End) && directionalIntensity > 0.0f)
		{
			directionalIntensity -= elapsedTime;
			directionalIntensity = max(directionalIntensity, 0.0f);

			mPSCBufferPerFrameData.LightColor = XMFLOAT4(directionalIntensity, directionalIntensity, directionalIntensity, 1.0f);
			mDirectionalLight.SetColor(mPSCBufferPerFrameData.LightColor);
			updateCBuffer = true;
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
			updateCBuffer = true;
		}

		if (updateCBuffer)
		{
			mGame->Direct3DDeviceContext()->UpdateSubresource(mPSCBufferPerFrame.Get(), 0, nullptr, &mPSCBufferPerFrameData, 0, 0);
		}
	}

	void BlinnPhongDemo::UpdateSpecularLight(const GameTime& gameTime)
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