#include "pch.h"
#include "CelestialBody.h"


using namespace std;
using namespace Library;
using namespace DirectX;

const float CelestialBody::ModelRotationRate = XM_PI;
const float CelestialBody::OrbitalScale = 100.0f;

const CelestialBody::PlanetaryData CelestialBody::MercuryData(0.382f, -57.9f, (1.0f / 58.646f), (1.0f / 87.969f), 0.0f, L"Content\\Textures\\mercury.dds");
const CelestialBody::PlanetaryData CelestialBody::VenusData(0.949f, -108.2f, (1.0f / 243.01f), (1.0f / 224.70f), 3.0962f, L"Content\\Textures\\venus.dds");
const CelestialBody::PlanetaryData CelestialBody::EarthData(1.0f, -149.6f, (1.0f / 23.9345f), (1.0f / 365.256f), 0.40613812f, L"Content\\Textures\\EarthComposite.dds");
const CelestialBody::PlanetaryData CelestialBody::MarsData(0.532f, -227.9f, (1.0f / 24.623f), (1.0f / 686.98f), 0.43825218f, L"Content\\Textures\\mars.dds");
const CelestialBody::PlanetaryData CelestialBody::JupiterData(11.19f, -778.6f, (1.0f / 9.842f), (1.0f / 4328.9f), 0.05305801f, L"Content\\Textures\\jupiter.dds");
const CelestialBody::PlanetaryData CelestialBody::SaturnData(9.26f, -1433.0f, (1.0f / 10.233f), (1.0f / 10734.65f), 0.471239f, L"Content\\Textures\\saturn.dds");
const CelestialBody::PlanetaryData CelestialBody::UranusData(4.01f, -2873.0f, (1.0f / 17.2f), (1.0f / 30674.6f), 1.708677f, L"Content\\Textures\\uranus.dds");
const CelestialBody::PlanetaryData CelestialBody::NeptuneData(3.88f, -4495.0f, (1.0f / 16.11f), (1.0f / 59757.8f), 0.5166175f, L"Content\\Textures\\neptune.dds");
const CelestialBody::PlanetaryData CelestialBody::PlutoData(0.18f, -5906.0f, (1.0f / 153.2976f), (1.0f / 90494.45f), 2.1293f, L"Content\\Textures\\pluto.dds");
const CelestialBody::PlanetaryData CelestialBody::MoonData(0.27254f, -38.44f, (1.0f / 27.0f), (1.0f / 27.0f), 0.0261799f, L"Content\\Textures\\moon.dds");

CelestialBody::CelestialBody(Library::Game& game, const std::shared_ptr<Library::Camera>& camera, 
	Library::PointLight& lightReference, const PlanetaryData& planetaryData):
	DrawableGameComponent(game, camera), mWorldMatrix(MatrixHelper::Identity), mIndexCount(0),
	mPointLight(lightReference), mPlanetaryData(planetaryData), mKeyboard(nullptr), mAnimationEnabled(true), mCurrentRotation(0.0f), mCurrentOrbit(0.0f)
{
}

CelestialBody::~CelestialBody()
{
}

void CelestialBody::Initialize()
{
	// Retrieve the keyboard service
	mKeyboard = reinterpret_cast<KeyboardComponent*>(mGame->Services().GetService(KeyboardComponent::TypeIdClass()));

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
	wstring textureName = mPlanetaryData.texturePath;
	ThrowIfFailed(CreateDDSTextureFromFile(mGame->Direct3DDevice(), textureName.c_str(), nullptr, mColorTexture.ReleaseAndGetAddressOf()), "CreateDDSTextureFromFile() failed.");

	if (textureName == L"Content\\Textures\\EarthComposite.dds")
	{	// Only apply specular map if we're rendering earth (yeah, gross)
		textureName = L"Content\\Textures\\EarthSpecularMap.png";
		ThrowIfFailed(CreateWICTextureFromFile(mGame->Direct3DDevice(), textureName.c_str(), nullptr, mSpecularMap.ReleaseAndGetAddressOf()), "CreateWICTextureFromFile() failed.");
	}

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
	if (mAnimationEnabled)
	{
		mCurrentRotation += gameTime.ElapsedGameTimeSeconds().count() * mPlanetaryData.rotation * OrbitalScale;
		mCurrentOrbit += gameTime.ElapsedGameTimeSeconds().count() * mPlanetaryData.orbit * OrbitalScale;
		
		XMStoreFloat4x4(&mWorldMatrix,
			XMMatrixScaling(mPlanetaryData.scale, mPlanetaryData.scale, mPlanetaryData.scale) *
			XMMatrixRotationY(mCurrentRotation) *
			XMMatrixRotationRollPitchYaw(0.0f, 0.0f, mPlanetaryData.tilt) *
			XMMatrixTranslation(0.0f, 0.0f, mPlanetaryData.distance) *
			XMMatrixRotationY(mCurrentOrbit)
			);
	}

	mVSCBufferPerFrameData.LightPosition = mPointLight.Position();
	mVSCBufferPerFrameData.LightRadius = mPointLight.Radius();
	mPSCBufferPerFrameData.LightPosition = mPointLight.Position();
	mPSCBufferPerFrameData.LightColor = ColorHelper::ToFloat3(mPointLight.Color(), true);

	if (mKeyboard != nullptr)
	{
		if (mKeyboard->WasKeyPressedThisFrame(Keys::Space))
		{
			ToggleAnimation();
		}
	}
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

float CelestialBody::GetCurrentRotation() const
{
	return mCurrentRotation;
}

float CelestialBody::GetCurrentOrbit() const
{
	return mCurrentOrbit;
}

void CelestialBody::ToggleAnimation()
{
	mAnimationEnabled = !mAnimationEnabled;
}
