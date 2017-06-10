#include "pch.h"

using namespace std;
using namespace Library;
using namespace DirectX;

namespace Rendering
{
	RTTI_DEFINITIONS(PointDemo)

	PointDemo::PointDemo(Game& game) :
		DrawableGameComponent(game)
	{
	}

	void PointDemo::Initialize()
	{
		// Load a compiled vertex shader
		std::vector<char> compiledVertexShader;
		Utility::LoadBinaryFile(L"Content\\Shaders\\PointDemoVS.cso", compiledVertexShader);		
		ThrowIfFailed(mGame->Direct3DDevice()->CreateVertexShader(&compiledVertexShader[0], compiledVertexShader.size(), nullptr, mVertexShader.ReleaseAndGetAddressOf()), "ID3D11Device::CreatedVertexShader() failed.");

		// Load a compiled pixel shader
		std::vector<char> compiledPixelShader;
		Utility::LoadBinaryFile(L"Content\\Shaders\\PointDemoPS.cso", compiledPixelShader);
		ThrowIfFailed(mGame->Direct3DDevice()->CreatePixelShader(&compiledPixelShader[0], compiledPixelShader.size(), nullptr, mPixelShader.ReleaseAndGetAddressOf()), "ID3D11Device::CreatedPixelShader() failed.");
	}

	void PointDemo::Draw(const GameTime& gameTime)
	{
		UNREFERENCED_PARAMETER(gameTime);

		ID3D11DeviceContext* direct3DDeviceContext = mGame->Direct3DDeviceContext();
		direct3DDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

		direct3DDeviceContext->VSSetShader(mVertexShader.Get(), nullptr, 0);
		direct3DDeviceContext->PSSetShader(mPixelShader.Get(), nullptr, 0);

		direct3DDeviceContext->Draw(1, 0);
	}
}