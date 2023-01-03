#pragma once

#include "stdafx.h"
#include "Plugin.h"

struct HouseSearch : public HouseInfo {

	HouseSearch(const HouseInfo& house)
	{
		this->Center = house.Center;
		this->Size = house.Size;
		GenerateSearchLocations();
	}
	float wallThickness{ 5.0f };

	bool shouldCheck{ true };
	bool fullySearched{ false };

	float timeSinceLooted{};
	const float minTimeBeforeRecheck{100.f};

	int minWidthBetweenSearchLocations{10};
	int minHeightBetweenSearchLocations{10};

	const float acceptanceRadius{3.0f};

	int currentLocationIndex{ 0 };

	std::vector<Elite::Vector2> searchLocations{};

	void UpdateTimeSinceLooted(float dt) {
		timeSinceLooted += dt;
		if (timeSinceLooted > minTimeBeforeRecheck) {
			shouldCheck = true;
			fullySearched = false;
		}
	}

	void GenerateSearchLocations() {
		Elite::Vector2 bottomLeft{Center.x - Size.x/2.f + wallThickness, Center.y - Size.y/2.f + wallThickness};

		Elite::Vector2 currentSearchLocation{ bottomLeft };
		searchLocations.push_back(currentSearchLocation);

		int rowAmount = int(Size.y - 2 * wallThickness) / minWidthBetweenSearchLocations;
		int colAmount = int(Size.x - 2 * wallThickness) / minHeightBetweenSearchLocations;

		for (int rowIndex{}; rowIndex < rowAmount; ++rowIndex) {
			for (int colIndex{}; colIndex < colAmount; ++colIndex) {
				currentSearchLocation = Elite::Vector2{bottomLeft.x + rowIndex * minWidthBetweenSearchLocations, bottomLeft.y + colIndex * minHeightBetweenSearchLocations};
				searchLocations.push_back(currentSearchLocation);
			}
		}
	}

	std::vector<Elite::Vector2> GetSearchLocations() {
		return searchLocations;
	}

	Elite::Vector2 GetCurrentLocation() {
		if (currentLocationIndex < searchLocations.size()) {
			return searchLocations.at(currentLocationIndex);
		}
	}

	bool IsPointInsideHouse(Elite::Vector2 point) {
		bool insideX = point.x > (Center.x - Size.x / 2.f + wallThickness) && point.x < (Center.x + Size.x / 2.f + wallThickness);
		bool insideY = point.y > (Center.y - Size.y / 2.f + wallThickness) && point.y < (Center.y + Size.y / 2.f + wallThickness);

		return insideX && insideY;
	}

	bool CheckedCurrentLocation(Elite::Vector2 playerLocation) {
		Elite::Vector2 currentLocation = GetCurrentLocation();

		if (Elite::DistanceSquared(playerLocation, currentLocation) < acceptanceRadius * acceptanceRadius) {
			if (currentLocationIndex < searchLocations.size() - 1) {
				//Set the index to the next location
				++currentLocationIndex;
				return true;
			}
			else {
				fullySearched = true;
				//Set the current location index back to 0 for when you come back to recheck it
				currentLocationIndex = 0;
				return true;
			}
		}
		return false;
	}

};