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

	float wallThickness{ 3.0f };

	bool shouldCheck{ true };

	float timeSinceLooted{};
	const float minTimeBeforeRecheck{100.f};

	int minWidthBetweenSearchLocations{};
	int minHeightBetweenSearchLocations{};

	const float acceptanceRadius{3.0f};

	UINT currentLocationIndex{ 0 };

	std::vector<Elite::Vector2> searchLocations{};

	void UpdateTimeSinceLooted(float dt) {
		timeSinceLooted += dt;
		if (timeSinceLooted > minTimeBeforeRecheck) {
			shouldCheck = true;
		}
	}

	void GenerateSearchLocations() {
		Elite::Vector2 bottomLeft{Center.x - Size.x/2.f + wallThickness, Center.y - Size.y/2.f + wallThickness};
		Elite::Vector2 bottomRight{ Center.x + Size.x / 2.f - wallThickness, Center.y - Size.y / 2.f + wallThickness };
		Elite::Vector2 topLeft{ Center.x - Size.x / 2.f + wallThickness, Center.y + Size.y / 2.f - wallThickness };
		Elite::Vector2 topRight{ Center.x + Size.x / 2.f - wallThickness, Center.y + Size.y / 2.f - wallThickness };

		//Use the corners and the center to fully search the house
		searchLocations.push_back(bottomLeft);
		searchLocations.push_back(Center);
		searchLocations.push_back(topLeft);
		searchLocations.push_back(topRight);
		searchLocations.push_back(Center);
		searchLocations.push_back(bottomRight);
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