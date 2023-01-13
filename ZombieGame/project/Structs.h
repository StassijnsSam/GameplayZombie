#pragma once

#include "stdafx.h"
#include "Plugin.h"

struct HouseSearch : public HouseInfo {

	HouseSearch(const HouseInfo& house)
	{
		this->Center = house.Center;
		this->Size = house.Size;
		minWidthBetweenSearchLocations = int(this->Size.x - 2 * wallThickness) / 4;
		minHeightBetweenSearchLocations = int(this->Size.y - 2 * wallThickness) / 4;
		GenerateSearchLocations();
	}
	HouseSearch() {
		this->Center = Elite::Vector2{ 0, 0 };
		this->Size = Elite::Vector2{ 0, 0 };
	}

	HouseSearch operator=(const HouseSearch& houseSearch) {
		this->Center = houseSearch.Center;
		this->Size = houseSearch.Size;
		this->shouldCheck = houseSearch.shouldCheck;
		this->currentLocationIndex = houseSearch.currentLocationIndex;
		this->searchLocations = houseSearch.searchLocations;
		return *this;
	}

	float wallThickness{ 3.5f };

	bool shouldCheck{ true };

	float timeSinceLooted{};
	const float minTimeBeforeRecheck{800.f};

	int minWidthBetweenSearchLocations{};
	int minHeightBetweenSearchLocations{};

	const float acceptanceRadius{3.0f};

	UINT currentLocationIndex{ 0 };

	std::vector<Elite::Vector2> searchLocations{};

	void UpdateTimeSinceLooted(float dt) {
		timeSinceLooted += dt;
		if (timeSinceLooted > minTimeBeforeRecheck) {
			shouldCheck = true;
			timeSinceLooted = 0;
		}
	}

	void GenerateSearchLocations() {
		Elite::Vector2 bottomLeftCenter{Center.x - Size.x/3.f + wallThickness, Center.y - Size.y/3.f + wallThickness};
		Elite::Vector2 bottomRightCenter{ Center.x + Size.x / 3.f - wallThickness, Center.y - Size.y / 3.f + wallThickness };
		Elite::Vector2 topLeftCenter{ Center.x - Size.x / 3.f + wallThickness, Center.y + Size.y / 3.f - wallThickness };
		Elite::Vector2 topRightCenter{ Center.x + Size.x / 3.f - wallThickness, Center.y + Size.y / 3.f - wallThickness };

		//Use the corners and the center to fully search the house
		searchLocations.push_back(bottomLeftCenter);
		searchLocations.push_back(topLeftCenter);
		searchLocations.push_back(topRightCenter);
		searchLocations.push_back(bottomRightCenter);
		searchLocations.push_back(Center);
	}

	std::vector<Elite::Vector2> GetSearchLocations() {
		return searchLocations;
	}

	Elite::Vector2 GetCurrentLocation() {
		if (currentLocationIndex < searchLocations.size()) {
			return searchLocations.at(currentLocationIndex);
		}
		//Invalid location
		return Elite::Vector2{ 0, 0 };
	}

	bool IsPointInsideHouse(Elite::Vector2 point) const{
		bool insideX = point.x > (Center.x - (Size.x / 2.f)) && point.x < (Center.x + (Size.x / 2.f));
		bool insideY = point.y > (Center.y - (Size.y / 2.f) ) && point.y < (Center.y + (Size.y / 2.f));

		return insideX && insideY;
	}

	bool UpdateCurrentLocation(Elite::Vector2 playerLocation) {
		Elite::Vector2 currentLocation = GetCurrentLocation();

		if (Elite::DistanceSquared(playerLocation, currentLocation) < acceptanceRadius * acceptanceRadius) {
			if (currentLocationIndex < searchLocations.size() - 1) {
				//Set the index to the next location
				currentLocationIndex += 1;
				return true;
			}
			else {
				shouldCheck = false;
				//Set the current location index back to 0 for when you come back to recheck it
				currentLocationIndex = 0;
				return true;
			}
		}
		return false;
	}

};

struct WorldSearch : public WorldInfo {
	WorldSearch(const WorldInfo& world)
	{
		this->Center = world.Center;
		this->Dimensions = world.Dimensions;
		minWidthBetweenSearchLocations = int(world.Dimensions.x/2.f) / 10;
		minHeightBetweenSearchLocations = int(world.Dimensions.y/2.f) / 10;
		GenerateSearchLocations();
	}
	WorldSearch() {
		this->Center = Elite::Vector2{ 0, 0 };
		this->Dimensions = Elite::Vector2{ 0, 0 };
	}

	int minWidthBetweenSearchLocations{};
	int minHeightBetweenSearchLocations{};

	const float acceptanceRadius{ 3.0f };
	//6 because this means you will not get anywhere close to the border
	const int squareAmount{ 4 };

	UINT currentLocationIndex{ 0 };

	std::vector<Elite::Vector2> searchLocations{};

	void GenerateSearchLocations() {
		//Most of the houses and the loot are at the middle of the map, so you never really want to get to the borders
		// You start from the center and do a square search, with bigger and bigger squares
		searchLocations.push_back(Center);
		for (int index{1}; index < squareAmount; ++index) {
			//Coordinates of the square
			Elite::Vector2 bottomLeft{ Center.x - index * minWidthBetweenSearchLocations, Center.y - index * minHeightBetweenSearchLocations};
			Elite::Vector2 bottomRight{ Center.x + index * minWidthBetweenSearchLocations, Center.y - index * minHeightBetweenSearchLocations };
			Elite::Vector2 topLeft{ Center.x - index * minWidthBetweenSearchLocations, Center.y + index * minHeightBetweenSearchLocations };
			Elite::Vector2 topRight{ Center.x + index * minWidthBetweenSearchLocations, Center.y + index * minHeightBetweenSearchLocations };
			
			searchLocations.push_back(bottomLeft);
			searchLocations.push_back(topLeft);
			searchLocations.push_back(topRight);
			searchLocations.push_back(bottomRight);

			//Make the search locations further out
			minWidthBetweenSearchLocations += int(Dimensions.x / 2.f) / 20;
			minHeightBetweenSearchLocations += int(Dimensions.y / 2.f) / 20;
		}
	}

	std::vector<Elite::Vector2> GetSearchLocations() {
		return searchLocations;
	}

	Elite::Vector2 GetCurrentLocation() {
		if (currentLocationIndex < searchLocations.size()) {
			return searchLocations.at(currentLocationIndex);
		}
		//Invalid location
		return Elite::Vector2{ 0, 0 };
	}

	bool UpdateCurrentLocation(Elite::Vector2 playerLocation) {
		Elite::Vector2 currentLocation = GetCurrentLocation();

		if (Elite::DistanceSquared(playerLocation, currentLocation) < acceptanceRadius * acceptanceRadius) {
			if (currentLocationIndex < searchLocations.size() - 1) {
				//Set the index to the next location
				currentLocationIndex += 1;
				return true;
			}
			else {
				//Set the current location index back to 0 so you return to the center
				currentLocationIndex = 0;
				return true;
			}
		}
		return false;
	}
};