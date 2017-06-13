#pragma once
#include "CelestialBody.h"
class Moon :
	public CelestialBody
{
public:
	Moon(Library::Game & game, const std::shared_ptr<Library::Camera>& camera, Library::PointLight& lightReference, 
		const PlanetaryData& planetaryData, std::shared_ptr<CelestialBody> planet);
	virtual ~Moon();

	void Initialize() override;
	void Update(const Library::GameTime& gameTime) override;

private:
	std::shared_ptr<CelestialBody> mPlanet;
	float mCurrentOrbitAroundPlanet;
};

