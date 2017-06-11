#include "pch.h"
#include "CelestialBody.h"


using namespace std;
using namespace Library;
using namespace DirectX;

const float CelestialBody::ModelRotationRate = XM_PI;

CelestialBody::CelestialBody(Library::Game& game, const std::shared_ptr<Library::Camera>& camera, Library::PointLight& lightReference):
	DrawableGameComponent(game, camera), mWorldMatrix(MatrixHelper::Identity), mIndexCount(0), mPointLight(lightReference)
{
}

CelestialBody::~CelestialBody()
{
}

void CelestialBody::Initialize()
{
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
	wstring textureName = L"Content\\Textures\\EarthComposite.dds";
	ThrowIfFailed(CreateDDSTextureFromFile(mGame->Direct3DDevice(), textureName.c_str(), nullptr, mColorTexture.ReleaseAndGetAddressOf()), "CreateDDSTextureFromFile() failed.");

	textureName = L"Content\\Textures\\EarthSpecularMap.png";
	ThrowIfFailed(CreateWICTextureFromFile(mGame->Direct3DDevice(), textureName.c_str(), nullptr, mSpecularMap.ReleaseAndGetAddressOf()), "CreateWICTextureFromFile() failed.");

	// Setup the point light
	mVSCBufferPerFrameData.LightPosition = mPointLight.Position();
	mVSCBufferPerFrameData.LightRadius = mPointLight.Radius();
	mPSCBufferPerFrameData.LightPosition = mPointLight.Position();
	mPSCBufferPerFrameData.LightColor = ColorHelper::ToFloat3(mPointLight.Color(), true);

	// Update the vertex and pixel shader constant buffers
	mGame->Direct3DDeviceContext()->UpdateSubresource(mVSCBufferPerFrame.Get(), 0, nullptr, &mVSCBufferPerFrameData, 0, 0);
	mGame->Direct3DDeviceContext()->UpdateSubresource(mPSCBufferPerObject.Get(), 0, nullptr, &mPSCBufferPerObjectData, 0, 0);
}

void CelestialBody::Update(const Library::GameTime& gameTime)
{
	static float angle = 0.0f;
	static float orbit = 0.0f;

	if (true)//mAnimationEnabled)
	{
		angle += gameTime.ElapsedGameTimeSeconds().count() * ModelRotationRate;
		orbit += gameTime.ElapsedGameTimeSeconds().count() * (1.0f / 23.9345f) * 10/*orbital scale*/;
		
		XMStoreFloat4x4(&mWorldMatrix, 
			XMMatrixRotationY(angle) * 
			XMMatrixTranslation(0.0f, 0.0f, -149.0f) *
			XMMatrixRotationY(orbit));
//		XMFLOAT4X4 translation(
//			2, 0, 0, 0,
//			0, 2, 0, 0,
//			0, 0, 2, 0,
//			0, 0, 0, 1);
//		mWorldMatrix._14 *= 200;
//		mWorldMatrix._24 *= 1;
//		mWorldMatrix._34 *= 1;
//		mWorldMatrix._44 *= 1;
		
	}

	mVSCBufferPerFrameData.LightPosition = mPointLight.Position();
	mVSCBufferPerFrameData.LightRadius = mPointLight.Radius();
	mPSCBufferPerFrameData.LightPosition = mPointLight.Position();
	mPSCBufferPerFrameData.LightColor = ColorHelper::ToFloat3(mPointLight.Color(), true);

//	if (mKeyboard != nullptr)
//	{
//		if (mKeyboard->WasKeyPressedThisFrame(Keys::Space))
//		{
//			ToggleAnimation();
//		}
//
//		UpdateAmbientLight(gameTime);
//		UpdatePointLight(gameTime);
//		UpdateSpecularLight(gameTime);
//	}
}

void CelestialBody::Draw(const Library::GameTime& gameTime)
{
	UNREFERENCED_PARAMETER(gameTime);
	assert(mCamera != nullptr);

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

	direct3DDeviceContext->DrawIndexed(mIndexCount, 0, 0);
}

void CelestialBody::CreateVertexBuffer(const Mesh& mesh, ID3D11Buffer** vertexBuffer) const
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
