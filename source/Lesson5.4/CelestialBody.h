#pragma once
class CelestialBody : public Library::DrawableGameComponent
{
public:
	typedef struct PlanetaryData
	{
		PlanetaryData(float s, float d, float r, float o, float t, std::wstring tp):
			scale(s), distance(d), rotation(r), orbit(o), tilt(t), texturePath(tp)
		{}
		float scale;
		float distance;
		float rotation;
		float orbit;
		float tilt;
		std::wstring texturePath;
	} PlanetaryData;

	const static PlanetaryData MercuryData;
	const static PlanetaryData VenusData;
	const static PlanetaryData EarthData;
	const static PlanetaryData MarsData;
	const static PlanetaryData JupiterData;
	const static PlanetaryData SaturnData;
	const static PlanetaryData UranusData;
	const static PlanetaryData NeptuneData;
	const static PlanetaryData PlutoData;
	const static PlanetaryData MoonData;
	
	CelestialBody(Library::Game & game, const std::shared_ptr<Library::Camera>& camera, Library::PointLight& lightReference, const PlanetaryData& planetaryData);
	virtual ~CelestialBody();
	CelestialBody(const CelestialBody& rhs) = delete;
	CelestialBody& operator=(const CelestialBody& rhs) = delete;
	CelestialBody(CelestialBody&& rhs) = delete;
	CelestialBody& operator=(CelestialBody&& rhs) = delete;

	void Initialize() override;
	void Update(const Library::GameTime& gameTime) override;
	void Draw(const Library::GameTime& gameTime) override;
	void CreateVertexBuffer(const Library::Mesh& mesh, ID3D11Buffer** vertexBuffer) const;
	float GetCurrentRotation() const;
	float GetCurrentOrbit() const;

protected:
	struct VSCBufferPerFrame
	{
		DirectX::XMFLOAT3 LightPosition;
		float LightRadius;

		VSCBufferPerFrame() :
			LightPosition(Library::Vector3Helper::Zero), LightRadius(50.0f) { }
		VSCBufferPerFrame(const DirectX::XMFLOAT3 lightPosition, float lightRadius) :
			LightPosition(lightPosition), LightRadius(lightRadius) { }
	};

	struct VSCBufferPerObject
	{
		DirectX::XMFLOAT4X4 WorldViewProjection;
		DirectX::XMFLOAT4X4 World;

		VSCBufferPerObject() = default;
		VSCBufferPerObject(const DirectX::XMFLOAT4X4& wvp, const DirectX::XMFLOAT4X4& world) :
			WorldViewProjection(wvp), World(world) { }
	};

	struct PSCBufferPerFrame
	{
		DirectX::XMFLOAT3 CameraPosition;
		float Padding;
		DirectX::XMFLOAT3 AmbientColor;
		float Padding2;
		DirectX::XMFLOAT3 LightPosition;
		float Padding3;
		DirectX::XMFLOAT3 LightColor;
		float Padding4;

		PSCBufferPerFrame() :
			CameraPosition(Library::Vector3Helper::Zero), AmbientColor(Library::Vector3Helper::Zero),
			LightPosition(Library::Vector3Helper::Zero), LightColor(Library::Vector3Helper::Zero)
		{
		}

		PSCBufferPerFrame(const DirectX::XMFLOAT3& cameraPosition, const DirectX::XMFLOAT3& ambientColor, const DirectX::XMFLOAT3& lightPosition, const DirectX::XMFLOAT3& lightColor) :
			CameraPosition(cameraPosition), AmbientColor(ambientColor),
			LightPosition(lightPosition), LightColor(lightColor)
		{
		}
	};

	struct PSCBufferPerObject
	{
		DirectX::XMFLOAT3 SpecularColor;
		float SpecularPower;

		PSCBufferPerObject() :
			SpecularColor(1.0f, 1.0f, 1.0f), SpecularPower(128.0f) { }

		PSCBufferPerObject(const DirectX::XMFLOAT3& specularColor, float specularPower) :
			SpecularColor(specularColor), SpecularPower(specularPower) { }
	};

	void ToggleAnimation();

	static const float ModelRotationRate;
	static const float OrbitalScale;

	DirectX::XMFLOAT4X4 mWorldMatrix;
	PSCBufferPerFrame mPSCBufferPerFrameData;
	VSCBufferPerFrame mVSCBufferPerFrameData;
	VSCBufferPerObject mVSCBufferPerObjectData;
	PSCBufferPerObject mPSCBufferPerObjectData;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> mVertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> mPixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> mInputLayout;
	Microsoft::WRL::ComPtr<ID3D11Buffer> mVertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> mIndexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> mVSCBufferPerFrame;
	Microsoft::WRL::ComPtr<ID3D11Buffer> mVSCBufferPerObject;
	Microsoft::WRL::ComPtr<ID3D11Buffer> mPSCBufferPerFrame;
	Microsoft::WRL::ComPtr<ID3D11Buffer> mPSCBufferPerObject;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mColorTexture;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mSpecularMap;
	std::uint32_t mIndexCount;
	Library::PointLight& mPointLight;
	PlanetaryData mPlanetaryData;
	Library::KeyboardComponent* mKeyboard;
	bool mAnimationEnabled;
	float mCurrentRotation;
	float mCurrentOrbit;
};

