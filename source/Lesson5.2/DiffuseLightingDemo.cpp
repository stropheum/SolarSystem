#include "pch.h"

using namespace std;
using namespace Library;
using namespace DirectX;

namespace Rendering
{
	RTTI_DEFINITIONS(DiffuseLightingDemo)

	const float DiffuseLightingDemo::ModelRotationRate = XM_PI;
	const XMFLOAT2 DiffuseLightingDemo::LightRotationRate = XMFLOAT2(XM_2PI, XM_2PI);

	DiffuseLightingDemo::DiffuseLightingDemo(Game& game, const shared_ptr<Camera>& camera) :
		DrawableGameComponent(game, camera),
		mWorldMatrix(MatrixHelper::Identity), mIndexCount(0),
		mRenderStateHelper(game), mTextPosition(0.0f, 40.0f), mAnimationEnabled(true)
	{
	}

	bool DiffuseLightingDemo::AnimationEnabled() const
	{
		return mAnimationEnabled;
	}

	void DiffuseLightingDemo::SetAnimationEnabled(bool enabled)
	{
		mAnimationEnabled = enabled;
	}

	void DiffuseLightingDemo::Initialize()
	{
		// Load a compiled vertex shader
		vector<char> compiledVertexShader;
		Utility::LoadBinaryFile(L"Content\\Shaders\\DiffuseLightingVS.cso", compiledVertexShader);		
		ThrowIfFailed(mGame->Direct3DDevice()->CreateVertexShader(&compiledVertexShader[0], compiledVertexShader.size(), nullptr, mVertexShader.ReleaseAndGetAddressOf()), "ID3D11Device::CreatedVertexShader() failed.");

		// Load a compiled pixel shader
		vector<char> compiledPixelShader;
		Utility::LoadBinaryFile(L"Content\\Shaders\\DiffuseLightingPS.cso", compiledPixelShader);
		ThrowIfFailed(mGame->Direct3DDevice()->CreatePixelShader(&compiledPixelShader[0], compiledPixelShader.size(), nullptr, mPixelShader.ReleaseAndGetAddressOf()), "ID3D11Device::CreatedPixelShader() failed.");

		// Create an input layout
		D3D11_INPUT_ELEMENT_DESC inputElementDescriptions[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
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
		constantBufferDesc.ByteWidth = sizeof(VertexCBufferPerObject);
		constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		ThrowIfFailed(mGame->Direct3DDevice()->CreateBuffer(&constantBufferDesc, nullptr, mVertexCBufferPerObject.ReleaseAndGetAddressOf()), "ID3D11Device::CreateBuffer() failed.");

		constantBufferDesc.ByteWidth = sizeof(PixelCBufferPerFrame);
		ThrowIfFailed(mGame->Direct3DDevice()->CreateBuffer(&constantBufferDesc, nullptr, mPixelCBufferPerFrame.ReleaseAndGetAddressOf()), "ID3D11Device::CreateBuffer() failed.");

		// Initialize cbuffers with default data
		mGame->Direct3DDeviceContext()->UpdateSubresource(mPixelCBufferPerFrame.Get(), 0, nullptr, &mPixelCBufferPerFrameData, 0, 0);

		// Load a texture
		wstring textureName = L"Content\\Textures\\EarthComposite.dds";
		ThrowIfFailed(CreateDDSTextureFromFile(mGame->Direct3DDevice(), textureName.c_str(), nullptr, mColorTexture.ReleaseAndGetAddressOf()), "CreateDDSTextureFromFile() failed.");

		// Create text rendering helpers
		mSpriteBatch = make_unique<SpriteBatch>(mGame->Direct3DDeviceContext());
		mSpriteFont = make_unique<SpriteFont>(mGame->Direct3DDevice(), L"Content\\Fonts\\Arial_14_Regular.spritefont");

		// Retrieve the keyboard service
		mKeyboard = reinterpret_cast<KeyboardComponent*>(mGame->Services().GetService(KeyboardComponent::TypeIdClass()));

		mProxyModel = make_unique<ProxyModel>(*mGame, mCamera, "Content\\Models\\DirectionalLightProxy.obj.bin", 0.5f);
		mProxyModel->Initialize();
		mProxyModel->SetPosition(10.0f, 0.0, 0.0f);
		mProxyModel->ApplyRotation(XMMatrixRotationY(XM_PIDIV2));

		mDirectionalLight = make_unique<DirectionalLight>(*mGame);
		mPixelCBufferPerFrameData.LightDirection = mDirectionalLight->DirectionToLight();
	}

	void DiffuseLightingDemo::Update(const GameTime& gameTime)
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
		}

		mProxyModel->Update(gameTime);
	}

	void DiffuseLightingDemo::Draw(const GameTime& gameTime)
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
		XMStoreFloat4x4(&mVertexCBufferPerObjectData.WorldViewProjection, wvp);
		XMStoreFloat4x4(&mVertexCBufferPerObjectData.World, XMMatrixTranspose(worldMatrix));

		direct3DDeviceContext->UpdateSubresource(mVertexCBufferPerObject.Get(), 0, nullptr, &mVertexCBufferPerObjectData, 0, 0);
		direct3DDeviceContext->VSSetConstantBuffers(0, 1, mVertexCBufferPerObject.GetAddressOf());
		direct3DDeviceContext->PSSetConstantBuffers(0, 1, mPixelCBufferPerFrame.GetAddressOf());

		direct3DDeviceContext->PSSetShaderResources(0, 1, mColorTexture.GetAddressOf());
		direct3DDeviceContext->PSSetSamplers(0, 1, SamplerStates::TrilinearWrap.GetAddressOf());

		direct3DDeviceContext->DrawIndexed(mIndexCount, 0, 0);

		mProxyModel->Draw(gameTime);

		// Draw help text
		mRenderStateHelper.SaveAll();
		mSpriteBatch->Begin();
		
		wostringstream helpLabel;
		helpLabel << "Ambient Intensity (+PgUp/-PgDn): " << mPixelCBufferPerFrameData.AmbientColor.x << "\n";
		helpLabel << L"Directional Light Intensity (+Home/-End): " << mPixelCBufferPerFrameData.LightColor.x << "\n";
		helpLabel << L"Rotate Directional Light (Arrow Keys)" << "\n";
		helpLabel << L"Toggle Grid (G)" << "\n";
		helpLabel << L"Toggle Animation (Space)" << "\n";
	
		mSpriteFont->DrawString(mSpriteBatch.get(), helpLabel.str().c_str(), mTextPosition);
		mSpriteBatch->End();
		mRenderStateHelper.RestoreAll();
	}

	void DiffuseLightingDemo::CreateVertexBuffer(const Mesh& mesh, ID3D11Buffer** vertexBuffer) const
	{
		const vector<XMFLOAT3>& sourceVertices = mesh.Vertices();
		const vector<XMFLOAT3>* const textureCoordinates = mesh.TextureCoordinates().at(0);
		assert(textureCoordinates->size() == sourceVertices.size());
		const vector<XMFLOAT3>& normals = mesh.Normals();
		assert(normals.size() == sourceVertices.size());

		vector<VertexPositionTextureNormal> vertices;
		vertices.reserve(sourceVertices.size());
		for (size_t i = 0; i < sourceVertices.size(); i++)
		{
			const XMFLOAT3& position = sourceVertices.at(i);
			const XMFLOAT3& uv = textureCoordinates->at(i);
			const XMFLOAT3& normal = normals.at(i);
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

	void DiffuseLightingDemo::ToggleAnimation()
	{
		mAnimationEnabled = !mAnimationEnabled;
	}

	void DiffuseLightingDemo::UpdateAmbientLight(const GameTime& gameTime)
	{
		static float ambientIntensity = mPixelCBufferPerFrameData.AmbientColor.x;

		assert(mKeyboard != nullptr);

		if (mKeyboard->IsKeyDown(Keys::PageUp) && ambientIntensity < 1.0f)
		{
			ambientIntensity += gameTime.ElapsedGameTimeSeconds().count();
			ambientIntensity = min(ambientIntensity, 1.0f);

			mPixelCBufferPerFrameData.AmbientColor = XMFLOAT4(ambientIntensity, ambientIntensity, ambientIntensity, 1.0f);
			mGame->Direct3DDeviceContext()->UpdateSubresource(mPixelCBufferPerFrame.Get(), 0, nullptr, &mPixelCBufferPerFrameData, 0, 0);
		}
		else if (mKeyboard->IsKeyDown(Keys::PageDown) && ambientIntensity > 0.0f)
		{
			ambientIntensity -= gameTime.ElapsedGameTimeSeconds().count();
			ambientIntensity = max(ambientIntensity, 0.0f);

			mPixelCBufferPerFrameData.AmbientColor = XMFLOAT4(ambientIntensity, ambientIntensity, ambientIntensity, 1.0f);
			mGame->Direct3DDeviceContext()->UpdateSubresource(mPixelCBufferPerFrame.Get(), 0, nullptr, &mPixelCBufferPerFrameData, 0, 0);
		}
	}

	void DiffuseLightingDemo::UpdateDirectionalLight(const GameTime& gameTime)
	{
		static float directionalIntensity = mPixelCBufferPerFrameData.LightColor.x;

		assert(mKeyboard != nullptr);

		float elapsedTime = gameTime.ElapsedGameTimeSeconds().count();
		bool updateCBuffer = false;

		// Update directional light intensity		
		if (mKeyboard->IsKeyDown(Keys::Home) && directionalIntensity < 1.0f)
		{
			directionalIntensity += elapsedTime;
			directionalIntensity = min(directionalIntensity, 1.0f);

			mPixelCBufferPerFrameData.LightColor = XMFLOAT4(directionalIntensity, directionalIntensity, directionalIntensity, 1.0f);
			mDirectionalLight->SetColor(mPixelCBufferPerFrameData.LightColor);
			updateCBuffer = true;			
		}
		else if (mKeyboard->IsKeyDown(Keys::End) && directionalIntensity > 0.0f)
		{
			directionalIntensity -= elapsedTime;
			directionalIntensity = max(directionalIntensity, 0.0f);

			mPixelCBufferPerFrameData.LightColor = XMFLOAT4(directionalIntensity, directionalIntensity, directionalIntensity, 1.0f);
			mDirectionalLight->SetColor(mPixelCBufferPerFrameData.LightColor);
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
			lightRotationMatrix *= XMMatrixRotationAxis(mDirectionalLight->RightVector(), rotationAmount.y);
		}

		if (rotationAmount.x != 0.0f || rotationAmount.y != 0.0f)
		{
			mDirectionalLight->ApplyRotation(lightRotationMatrix);
			mProxyModel->ApplyRotation(lightRotationMatrix);
			mPixelCBufferPerFrameData.LightDirection = mDirectionalLight->DirectionToLight();
			updateCBuffer = true;			
		}

		if (updateCBuffer)
		{
			mGame->Direct3DDeviceContext()->UpdateSubresource(mPixelCBufferPerFrame.Get(), 0, nullptr, &mPixelCBufferPerFrameData, 0, 0);
		}
	}
}