#include "pch.h"
#include "Moon.h"


using namespace std;
using namespace Library;
using namespace DirectX;

Moon::Moon(Library::Game& game, const std::shared_ptr<Library::Camera>& camera, Library::PointLight& lightReference,
	const PlanetaryData& planetaryData, std::shared_ptr<CelestialBody> planet) :
	CelestialBody(game, camera, lightReference, planetaryData), mPlanet(planet), mCurrentOrbitAroundPlanet(0.0f)
{
}

Moon::~Moon()
{
}

void Moon::Initialize()
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
	wstring textureName = CelestialBody::MoonData.texturePath;
	ThrowIfFailed(CreateDDSTextureFromFile(mGame->Direct3DDevice(), textureName.c_str(), nullptr, mColorTexture.ReleaseAndGetAddressOf()), "CreateDDSTextureFromFile() failed.");

	// Setup the point light
	mVSCBufferPerFrameData.LightPosition = mPointLight.Position();
	mVSCBufferPerFrameData.LightRadius = mPointLight.Radius();
	mPSCBufferPerFrameData.LightPosition = mPointLight.Position();
	mPSCBufferPerFrameData.LightColor = ColorHelper::ToFloat3(mPointLight.Color(), true);

	// Update the vertex and pixel shader constant buffers
	mGame->Direct3DDeviceContext()->UpdateSubresource(mVSCBufferPerFrame.Get(), 0, nullptr, &mVSCBufferPerFrameData, 0, 0);
	mGame->Direct3DDeviceContext()->UpdateSubresource(mPSCBufferPerObject.Get(), 0, nullptr, &mPSCBufferPerObjectData, 0, 0);
}


void Moon::Update(const Library::GameTime& gameTime)
{
	if (mAnimationEnabled)
	{
		mCurrentRotation += gameTime.ElapsedGameTimeSeconds().count() * CelestialBody::MoonData.rotation * OrbitalScale;
		mCurrentOrbit += gameTime.ElapsedGameTimeSeconds().count() * mPlanetaryData.orbit * OrbitalScale;
		mCurrentOrbitAroundPlanet += gameTime.ElapsedGameTimeSeconds().count() * CelestialBody::MoonData.orbit * OrbitalScale;
		float moonScale = CelestialBody::MoonData.scale;

		XMStoreFloat4x4(&mWorldMatrix,
			XMMatrixScaling(moonScale, moonScale, moonScale) *
			XMMatrixRotationY(mCurrentRotation) *
			XMMatrixRotationRollPitchYaw(0.0f, 0.0f, mPlanetaryData.tilt) *
			XMMatrixTranslation(0.0f, 0.0f, CelestialBody::MoonData.distance * CelestialBody::OrbitalScale) *
			XMMatrixRotationY(mCurrentOrbitAroundPlanet) *
			XMMatrixTranslation(0.0f, 0.0f, mPlanetaryData.distance * CelestialBody::OrbitalScale) *
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
