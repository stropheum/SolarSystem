#include "pch.h"

using namespace std;
using namespace Library;
using namespace DirectX;

namespace Rendering
{
	RTTI_DEFINITIONS(PointLightDemo)

	const float PointLightDemo::ModelRotationRate = XM_PI;
	const float PointLightDemo::LightModulationRate = UCHAR_MAX;
	const float PointLightDemo::LightMovementRate = 10.0f;

	PointLightDemo::PointLightDemo(Game & game, const shared_ptr<Camera>& camera) :
		DrawableGameComponent(game, camera), mWorldMatrix(MatrixHelper::Identity), mPointLight(game, XMFLOAT3(0.0f, 0.0f, 0.0f), 5000000.0f),
		mRenderStateHelper(game), mIndexCount(0), mTextPosition(0.0f, 40.0f), mAnimationEnabled(true)
	{
	}

	bool PointLightDemo::AnimationEnabled() const
	{
		return mAnimationEnabled;
	}

	void PointLightDemo::SetAnimationEnabled(bool enabled)
	{
		mAnimationEnabled = enabled;
	}

	void PointLightDemo::Initialize()
	{
		// Comment begin
		// Load a compiled vertex shader
		vector<char> compiledVertexShader;
		Utility::LoadBinaryFile(L"Content\\Shaders\\PointLightDemoVS.cso", compiledVertexShader);
		ThrowIfFailed(mGame->Direct3DDevice()->CreateVertexShader(&compiledVertexShader[0], compiledVertexShader.size(), nullptr, mVertexShader.ReleaseAndGetAddressOf()), "ID3D11Device::CreatedVertexShader() failed.");

		// Load a compiled pixel shader
		vector<char> compiledPixelShader;
		Utility::LoadBinaryFile(L"Content\\Shaders\\PointLightDemoPS.cso", compiledPixelShader);
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

		// comment end
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

		// Comment Begin
		// Load textures for the color and specular maps
		wstring textureName = L"Content\\Textures\\sun.dds";
		ThrowIfFailed(CreateDDSTextureFromFile(mGame->Direct3DDevice(), textureName.c_str(), nullptr, mColorTexture.ReleaseAndGetAddressOf()), "CreateDDSTextureFromFile() failed.");

		// Create text rendering helpers
		mSpriteBatch = make_unique<SpriteBatch>(mGame->Direct3DDeviceContext());
		mSpriteFont = make_unique<SpriteFont>(mGame->Direct3DDevice(), L"Content\\Fonts\\Arial_14_Regular.spritefont");

		// Retrieve the keyboard service
		mKeyboard = reinterpret_cast<KeyboardComponent*>(mGame->Services().GetService(KeyboardComponent::TypeIdClass()));
		
		// Setup the point light
		mVSCBufferPerFrameData.LightPosition = mPointLight.Position();
		mVSCBufferPerFrameData.LightRadius = mPointLight.Radius();
		mPSCBufferPerFrameData.LightPosition = mPointLight.Position();
		mPSCBufferPerFrameData.LightColor = ColorHelper::ToFloat3(mPointLight.Color(), true);

		// Update the vertex and pixel shader constant buffers
		mGame->Direct3DDeviceContext()->UpdateSubresource(mVSCBufferPerFrame.Get(), 0, nullptr, &mVSCBufferPerFrameData, 0, 0);
		mGame->Direct3DDeviceContext()->UpdateSubresource(mPSCBufferPerObject.Get(), 0, nullptr, &mPSCBufferPerObjectData, 0, 0);

		// Load a proxy model for the point light
		mProxyModel = make_unique<ProxyModel>(*mGame, mCamera, "Content\\Models\\PointLightProxy.obj.bin", 0.5f);
		mProxyModel->Initialize();
		mProxyModel->SetPosition(mPointLight.Position());
	}

	void PointLightDemo::Update(const GameTime& gameTime)
	{
		UNREFERENCED_PARAMETER(gameTime);
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
			UpdatePointLight(gameTime);
			UpdateSpecularLight(gameTime);
		}

		mProxyModel->Update(gameTime);
	}

	void PointLightDemo::Draw(const GameTime& gameTime)
	{
		UNREFERENCED_PARAMETER(gameTime);
		assert(mCamera != nullptr);

		// Comment Begin
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
		XMStoreFloat4x4(&mVSCBufferPerObjectData.WorldViewProjection, wvp * XMMatrixScaling(2.0f, 2.0f, 2.0f));
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

		direct3DDeviceContext->DrawIndexed(mIndexCount, 0, 0);
		// Comment end

		mProxyModel->Draw(gameTime);
	}

	Library::PointLight& PointLightDemo::GetPointLight() 
	{
		return mPointLight;
	}

	void PointLightDemo::CreateVertexBuffer(const Mesh& mesh, ID3D11Buffer** vertexBuffer) const
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

	void PointLightDemo::ToggleAnimation()
	{
		mAnimationEnabled = !mAnimationEnabled;
	}

	void PointLightDemo::UpdateAmbientLight(const GameTime& gameTime)
	{
		UNREFERENCED_PARAMETER(gameTime);
		mPSCBufferPerFrameData.AmbientColor.x = 1.0f;
		mPSCBufferPerFrameData.AmbientColor.y = 1.0f;
		mPSCBufferPerFrameData.AmbientColor.z = 1.0f;
		mGame->Direct3DDeviceContext()->UpdateSubresource(mPSCBufferPerFrame.Get(), 0, nullptr, &mPSCBufferPerFrameData, 0, 0);
	}

	void PointLightDemo::UpdatePointLight(const GameTime& gameTime)
	{
		UNREFERENCED_PARAMETER(gameTime);
		static float lightIntensity = mPSCBufferPerFrameData.LightColor.x;

		assert(mKeyboard != nullptr);

		float elapsedTime = gameTime.ElapsedGameTimeSeconds().count();
		bool updateCBuffer = false;

		// Update point light intensity
		if (mKeyboard->IsKeyDown(Keys::Home) && lightIntensity < 2.0f)
		{
			lightIntensity += elapsedTime;
			lightIntensity = min(lightIntensity, 2.0f);

			mPSCBufferPerFrameData.LightColor = XMFLOAT3(lightIntensity, lightIntensity, lightIntensity);
			mPointLight.SetColor(mPSCBufferPerFrameData.LightColor.x, mPSCBufferPerFrameData.LightColor.y, mPSCBufferPerFrameData.LightColor.z, 1.0f);
			updateCBuffer = true;
		}
		else if (mKeyboard->IsKeyDown(Keys::End) && lightIntensity > 0.0f)
		{
			lightIntensity -= elapsedTime;
			lightIntensity = max(lightIntensity, 0.0f);

			mPSCBufferPerFrameData.LightColor = XMFLOAT3(lightIntensity, lightIntensity, lightIntensity);
			mPointLight.SetColor(mPSCBufferPerFrameData.LightColor.x, mPSCBufferPerFrameData.LightColor.y, mPSCBufferPerFrameData.LightColor.z, 1.0f);
			updateCBuffer = true;
		}

		// Move point light
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
			mPointLight.SetPosition(mPointLight.PositionVector() + movement);
			mProxyModel->SetPosition(mPointLight.Position());
			mVSCBufferPerFrameData.LightPosition = mPointLight.Position();
			mPSCBufferPerFrameData.LightPosition = mPointLight.Position();
			updateCBuffer = true;
		}

		// Update the light's radius
		if (mKeyboard->IsKeyDown(Keys::V))
		{
			float radius = mPointLight.Radius() + LightModulationRate * elapsedTime;
			mPointLight.SetRadius(radius);
			mVSCBufferPerFrameData.LightRadius = mPointLight.Radius();
			updateCBuffer = true;
		}

		if (mKeyboard->IsKeyDown(Keys::B))
		{
			float radius = mPointLight.Radius() - LightModulationRate * elapsedTime;
			radius = max(radius, 0.0f);
			mPointLight.SetRadius(radius);
			mVSCBufferPerFrameData.LightRadius = mPointLight.Radius();
			updateCBuffer = true;
		}

		if (updateCBuffer)
		{			
			mGame->Direct3DDeviceContext()->UpdateSubresource(mVSCBufferPerFrame.Get(), 0, nullptr, &mVSCBufferPerFrameData, 0, 0);
			mGame->Direct3DDeviceContext()->UpdateSubresource(mPSCBufferPerFrame.Get(), 0, nullptr, &mPSCBufferPerFrameData, 0, 0);
		}
	}

	void PointLightDemo::UpdateSpecularLight(const GameTime& gameTime)
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