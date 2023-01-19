/*=============================================================================*/
// Copyright 2020-2021 Elite Engine
/*=============================================================================*/
// Behaviors.h: Implementation of certain reusable behaviors for the BT version of the Agario Game
/*=============================================================================*/
#ifndef ELITE_APPLICATION_BEHAVIOR_TREE_BEHAVIORS
#define ELITE_APPLICATION_BEHAVIOR_TREE_BEHAVIORS
//-----------------------------------------------------------------
// Includes & Forward Declarations
//-----------------------------------------------------------------
#include "EliteMath/EMath.h"
#include "EBehaviorTree.h"
#include "Inventory.h"

//-----------------------------------------------------------------
// Behaviors
//-----------------------------------------------------------------
namespace BT_Actions
{
	BehaviorState Seek(Blackboard* pBlackboard) {
		Elite::Vector2 target{};
		AgentInfo playerInfo{};
		IExamInterface* pInterface{};
		SteeringPlugin_Output steering{};
		bool canRun{};
		bool isFleeing{};

		float acceptanceRadius{ 0.01f };

		bool dataFound = pBlackboard->GetData("Target", target) &&
			pBlackboard->GetData("PlayerInfo", playerInfo) &&
			pBlackboard->GetData("Interface", pInterface) &&
			pBlackboard->GetData("CanRun", canRun) &&
			pBlackboard->GetData("IsFleeing", isFleeing);

		if (!dataFound || pInterface == nullptr) {
			return BehaviorState::Failure;
		}

		target = pInterface->NavMesh_GetClosestPathPoint(target);

		//If it is close to the target, let him stop
		if (Elite::DistanceSquared(target, playerInfo.Position) < acceptanceRadius * acceptanceRadius) {
			//Set fleeing data
			if (isFleeing) {
				pBlackboard->ChangeData("IsFleeing", false);
				pBlackboard->ChangeData("WasFleeing", true);
				pBlackboard->ChangeData("CanRun", false);
			}
			steering.LinearVelocity = { 0, 0 };
		}
		else {
			steering.RunMode = canRun;
			steering.LinearVelocity = target - playerInfo.Position;
			steering.LinearVelocity.Normalize();
			steering.LinearVelocity *= playerInfo.MaxLinearSpeed;
		}
		
		pBlackboard->ChangeData("SteeringOutput", steering);
		return BehaviorState::Success;
	}

	BehaviorState Flee(Blackboard* pBlackboard)
	{
		Elite::Vector2 fleeTarget{};
		AgentInfo playerInfo{};
		IExamInterface* pInterface{};
		SteeringPlugin_Output steering{};
		bool canRun{};
		float fleeRadius{};

		bool dataFound = pBlackboard->GetData("FleeTarget", fleeTarget) &&
			pBlackboard->GetData("PlayerInfo", playerInfo) &&
			pBlackboard->GetData("Interface", pInterface) &&
			pBlackboard->GetData("CanRun", canRun) &&
			pBlackboard->GetData("FleeRadius", fleeRadius);

		if (!dataFound || pInterface == nullptr) {
			return BehaviorState::Failure;
		}

		//Calculate the spot to run to
		Elite::Vector2 playerPosition{ playerInfo.Position };

		Elite::Vector2 targetToPlayer = playerPosition - fleeTarget;
		targetToPlayer.Normalize();

		Elite::Vector2 newTarget = playerPosition + targetToPlayer * fleeRadius;

		pBlackboard->ChangeData("Target", newTarget);
		pBlackboard->ChangeData("IsFleeing", true);

		return Seek(pBlackboard);
	}

	BehaviorState Face(Blackboard* pBlackboard)
	{
		Elite::Vector2 target{};
		AgentInfo playerInfo{};
		//Get the steering so you will keep moving while facing the target
		SteeringPlugin_Output steering{};

		bool dataFound = pBlackboard->GetData("Target", target) &&
			pBlackboard->GetData("PlayerInfo", playerInfo) &&
			pBlackboard->GetData("SteeringOutput", steering);

		if (dataFound == false) {
			return BehaviorState::Failure;
		}

		const Elite::Vector2 toTarget{ target - playerInfo.Position };

		//angle between -pi and +pi
		const float angleTo{ atan2f(toTarget.y, toTarget.x) };
		float angleFrom{ playerInfo.Orientation };

		float deltaAngle = angleTo - angleFrom;

		steering.AutoOrient = false;
		steering.AngularVelocity = deltaAngle * playerInfo.MaxAngularSpeed;
		
		pBlackboard->ChangeData("SteeringOutput", steering);
		return BehaviorState::Success;
	}

	BehaviorState FaceBehind(Blackboard* pBlackboard)
	{
		Elite::Vector2 target{};
		AgentInfo playerInfo{};
		//Get the steering so you will keep moving while facing the target
		SteeringPlugin_Output steering{};

		bool dataFound = pBlackboard->GetData("Target", target) &&
			pBlackboard->GetData("PlayerInfo", playerInfo) &&
			pBlackboard->GetData("SteeringOutput", steering);

		if (dataFound == false) {
			return BehaviorState::Failure;
		}
		//To look behind you, flip the vector
		const Elite::Vector2 toPlayer{ playerInfo.Position - target};

		//angle between -pi and +pi
		const float angleTo{ atan2f(toPlayer.y, toPlayer.x) };
		float angleFrom{ playerInfo.Orientation };

		float deltaAngle = angleTo - angleFrom;

		steering.AutoOrient = false;
		steering.AngularVelocity = deltaAngle * playerInfo.MaxAngularSpeed;

		pBlackboard->ChangeData("SteeringOutput", steering);
		return BehaviorState::Success;
	}

	BehaviorState GetReadyToEscapePurgeZone(Blackboard* pBlackboard) {
		AgentInfo playerInfo{};
		PurgeZoneInfo currentPurgeZone{};

		bool dataFound = pBlackboard->GetData("CurrentPurgeZone", currentPurgeZone) &&
			pBlackboard->GetData("PlayerInfo", playerInfo);

		if (dataFound == false) {
			return BehaviorState::Failure;
		}

		pBlackboard->ChangeData("FleeTarget", currentPurgeZone.Center);
		pBlackboard->ChangeData("CanRun", true);
		
		return BehaviorState::Success;
	}

	BehaviorState GetReadyToFlee(Blackboard* pBlackboard) {
		AgentInfo playerInfo{};

		bool dataFound = pBlackboard->GetData("PlayerInfo", playerInfo);

		if (dataFound == false) {
			return BehaviorState::Failure;
		}

		float stamina = playerInfo.Stamina;

		pBlackboard->ChangeData("IsFleeing", true);
		pBlackboard->ChangeData("WasFleeing", false);
		//only sprint if stamina is high enough!
		if (stamina > 3.0f) {
			pBlackboard->ChangeData("CanRun", true);
		}

		return BehaviorState::Success;
	}

	BehaviorState SetClosestEnemyAsTarget(Blackboard* pBlackboard) {
		AgentInfo playerInfo{};
		std::vector<EnemyInfo>* pEnemiesInFOV{};

		bool dataFound = pBlackboard->GetData("EnemiesInFOV", pEnemiesInFOV) &&
			pBlackboard->GetData("PlayerInfo", playerInfo);

		if (dataFound == false || pEnemiesInFOV == nullptr) {
			return BehaviorState::Failure;
		}

		//If there are no enemies in the FOV, this cant work
		if (pEnemiesInFOV->size() <= 0) {
			return BehaviorState::Failure;
		}

		EnemyInfo closestEnemy{ pEnemiesInFOV->at(0) };
		float minDistanceSquared{Elite::DistanceSquared(playerInfo.Position, closestEnemy.Location)};
		for (const EnemyInfo& enemy : *pEnemiesInFOV) {
			float distanceSquared{ Elite::DistanceSquared(playerInfo.Position, enemy.Location )};
			if (distanceSquared < minDistanceSquared) {
				closestEnemy = enemy;
				minDistanceSquared = distanceSquared;
			}
		}

		pBlackboard->ChangeData("Target", closestEnemy.Location);
		pBlackboard->ChangeData("FleeTarget", closestEnemy.Location);
		return BehaviorState::Success;
	}

	BehaviorState ShootTarget(Blackboard* pBlackboard) {
		Inventory* pInventory{};
		std::vector<EnemyInfo>* pEnemiesInFov{};

		bool dataFound = pBlackboard->GetData("Inventory", pInventory) &&
			pBlackboard->GetData("EnemiesInFOV", pEnemiesInFov);

		if (dataFound == false || pInventory == nullptr || pEnemiesInFov == nullptr) {
			return BehaviorState::Failure;
		}

		//Prefer pistol over shotgun in general
		eItemType preferredGunType{eItemType::PISTOL};
		eItemType secondaryGunType{eItemType::SHOTGUN};

		if (pEnemiesInFov->size() > 1) {
			//if more than one enemy is in the FOV, prefer a shotgun
			preferredGunType = eItemType::SHOTGUN;
			secondaryGunType = eItemType::PISTOL;
		}

		//	Inventory keeps track of the ammo and will throw away an empty gun automatically
		if (pInventory->ContainsItemOfType(preferredGunType)) {
			bool hasFired = pInventory->UseItemOfType(preferredGunType);
			if (hasFired) {
				return BehaviorState::Success;
			}	
		}
		else {
			if (pInventory->ContainsItemOfType(secondaryGunType)) {
				bool hasFired = pInventory->UseItemOfType(secondaryGunType);
				if (hasFired) {
					return BehaviorState::Success;
				}			
			}
		}
		//If no guns were found somehow, and you were not able to fire them, return faillure
		return BehaviorState::Failure;
	}

	BehaviorState UseMedkit(Blackboard* pBlackboard) {
		Inventory* pInventory{};

		bool dataFound = pBlackboard->GetData("Inventory", pInventory);

		if (dataFound == false || pInventory == nullptr) {
			return BehaviorState::Failure;
		}

		//Inventory will automatically delete the medkit once it is used up
		if (pInventory->ContainsItemOfType(eItemType::MEDKIT)) {
			bool hasUsed = pInventory->UseItemOfType(eItemType::MEDKIT);
			if (hasUsed) {
				return BehaviorState::Success;
			}
		}
		//If no medkit was found or you were unable to use it, return faillure
		return BehaviorState::Failure;
	}

	BehaviorState EatFood(Blackboard* pBlackboard) {
		Inventory* pInventory{};

		bool dataFound = pBlackboard->GetData("Inventory", pInventory);

		if (dataFound == false || pInventory == nullptr) {
			return BehaviorState::Failure;
		}

		//Inventory will automatically delete the food once it is used up
		if (pInventory->ContainsItemOfType(eItemType::FOOD)) {
			bool hasUsed = pInventory->UseItemOfType(eItemType::FOOD);
			if (hasUsed) {
				return BehaviorState::Success;
			}
		}
		//If no food was found or you were unable to eat it, return faillure
		return BehaviorState::Failure;
	}

	BehaviorState SetTargetBehindPlayer(Blackboard* pBlackboard) {
		AgentInfo playerInfo{};
		Elite::Vector2 target{};
		const float distanceFactor{ 2.0f };

		bool dataFound = pBlackboard->GetData("PlayerInfo", playerInfo) &&
			pBlackboard->GetData("Target", target);

		if (dataFound == false) {
			return BehaviorState::Failure;
		}

		Elite::Vector2 pointBehind = Elite::Vector2(playerInfo.Position.x - distanceFactor * cosf(playerInfo.Orientation),
			playerInfo.Position.y - distanceFactor * sinf(playerInfo.Orientation));

		pBlackboard->ChangeData("FleeTarget", pointBehind);
		return BehaviorState::Success;
	}

	BehaviorState SetClosestItemAsTarget(Blackboard* pBlackboard) {
		AgentInfo playerInfo{};
		std::vector<EntityInfo>* pItemsInFOV{};

		bool dataFound = pBlackboard->GetData("ItemsInFOV", pItemsInFOV) &&
			pBlackboard->GetData("PlayerInfo", playerInfo);

		if (dataFound == false || pItemsInFOV == nullptr) {
			return BehaviorState::Failure;
		}

		//If there are no items in the FOV, this cant work
		if (!(pItemsInFOV->size() > 0)) {
			return BehaviorState::Failure;
		}

		EntityInfo closestItem{ pItemsInFOV->at(0) };
		float minDistanceSquared{ Elite::DistanceSquared(playerInfo.Position, closestItem.Location) };
		for (const EntityInfo& item : *pItemsInFOV) {
			float distanceSquared{ Elite::DistanceSquared(playerInfo.Position, item.Location) };
			if (distanceSquared < minDistanceSquared) {
				closestItem = item;
				minDistanceSquared = distanceSquared;
			}
		}
		pBlackboard->ChangeData("ClosestItem", closestItem);
		pBlackboard->ChangeData("Target", closestItem.Location);
		return BehaviorState::Success;
	}

	BehaviorState PickUpClosestItem(Blackboard* pBlackboard) {
		EntityInfo closestItem{};
		Inventory* pInventory{};
		IExamInterface* pInterface{};
		std::vector<ItemInfo>* pKnownItems{};

		auto dataFound = pBlackboard->GetData("ClosestItem", closestItem) &&
			pBlackboard->GetData("Inventory", pInventory) &&
			pBlackboard->GetData("Interface", pInterface) &&
			pBlackboard->GetData("KnownItems", pKnownItems);

		if (dataFound == false || pInventory == nullptr || pInterface == nullptr || pKnownItems == nullptr) {
			return BehaviorState::Failure;
		}

		//Closest item is empty
		if (closestItem.EntityHash == 0) {
			return BehaviorState::Failure;
		}
		bool hasPickedup = pInventory->PickupItem(closestItem);
		if (hasPickedup) {
			//Debug render the inventory (TODO: remove)
			pInventory->DebugRender();

			//Check if this item was a known item
			int indexOfKnowItem{ invalid_index };
			for (int index{}; index < int(pKnownItems->size()); ++index) {
				if (closestItem.Location.Distance(pKnownItems->at(index).Location) < 0.1f) {
					indexOfKnowItem = index;
				}
			}
			//It is a known item, delete it from the known item list
			if (indexOfKnowItem != invalid_index) {
				pKnownItems->erase(pKnownItems->begin() + indexOfKnowItem);
			}

			return BehaviorState::Success;
		}

		//If it was impossible to pick it up, return faillure
		return BehaviorState::Failure;
	}

	BehaviorState SearchHouse(Blackboard* pBlackboard) {
		HouseSearch* pCurrentHouse{};
		AgentInfo playerInfo{};

		bool dataFound = pBlackboard->GetData("CurrentHouse", pCurrentHouse) &&
			pBlackboard->GetData("PlayerInfo", playerInfo);

		if (dataFound == false || pCurrentHouse == nullptr) {
			return BehaviorState::Failure;
		}

		//Check if you are close to the current location, and then update it to the next one
		bool hasChecked = pCurrentHouse->UpdateCurrentLocation(playerInfo.Position);
		pBlackboard->ChangeData("Target", pCurrentHouse->GetCurrentLocation());

		//Go towards the new spot
		return Seek(pBlackboard);
	}

	BehaviorState ExploreWorld(Blackboard* pBlackboard) {
		WorldSearch* pWorldSearch{};
		AgentInfo playerInfo{};

		bool dataFound = pBlackboard->GetData("WorldSearch", pWorldSearch) &&
			pBlackboard->GetData("PlayerInfo", playerInfo);

		if (dataFound == false || pWorldSearch == nullptr) {
			return BehaviorState::Failure;
		}

		//Check if you are close to the current location, and then update it to the next one
		bool hasChecked = pWorldSearch->UpdateCurrentLocation(playerInfo.Position);
		pBlackboard->ChangeData("Target", pWorldSearch->GetCurrentLocation());

		//Go towards the new spot
		return Seek(pBlackboard);
	}

	BehaviorState RememberItem(Blackboard* pBlackboard) {
		std::vector<ItemInfo>* pKnownItems{};
		IExamInterface* pInterface{};
		EntityInfo closestItem{};

		bool dataFound = pBlackboard->GetData("KnownItems", pKnownItems) &&
			pBlackboard->GetData("Interface", pInterface) &&
			pBlackboard->GetData("ClosestItem", closestItem);

		if (dataFound == false || pKnownItems == nullptr || pInterface == nullptr) {
			return BehaviorState::Failure;
		}

		//Closest item is empty
		if (closestItem.EntityHash == 0) {
			return BehaviorState::Failure;
		}

		ItemInfo knownItem{};
		pInterface->Item_GetInfo(closestItem, knownItem);

		//Check to see if it is already in there, by location
		bool isNewItem{ true };
		for (const ItemInfo& item : *pKnownItems) {
			if (item.Location.Distance(closestItem.Location) < 0.1f) {
				isNewItem = false;
			}
		}

		if (isNewItem) {
			pKnownItems->push_back(knownItem);
			return BehaviorState::Success;
		}
		
		return BehaviorState::Failure;
	}

	BehaviorState MarkHouseAsUnsafe(Blackboard* pBlackboard) {
		HouseSearch* pCurrentHouse{};

		bool dataFound = pBlackboard->GetData("CurrentHouse", pCurrentHouse);

		if (dataFound == false || pCurrentHouse == nullptr) {
			return BehaviorState::Failure;
		}

		const float dangerTime{ 200.f };

		//Do not check the house
		pCurrentHouse->shouldCheck = false;
		pCurrentHouse->timeSinceLooted = pCurrentHouse->minTimeBeforeRecheck - dangerTime;
		
		return BehaviorState::Success;
	}
}

namespace BT_Conditions
{
	bool IsInPurgeZone(Blackboard* pBlackboard) {
		std::vector<PurgeZoneInfo>* pPurgeZonesInFOV{};
		AgentInfo playerInfo{};
		const float bufferZoneSquared{ 100.0f};
		
		bool dataFound = pBlackboard->GetData("PurgeZonesInFOV", pPurgeZonesInFOV)&&
			pBlackboard->GetData("PlayerInfo", playerInfo);

		//If its not found or the vector is empty
		if (dataFound == false || pPurgeZonesInFOV == nullptr) {
			return false;
		}

		//There is no purge zones in FOV
		if (pPurgeZonesInFOV->size() <= 0) {
			return false;
		}

		for (const PurgeZoneInfo& purgeZone : *pPurgeZonesInFOV) {
			if (Elite::DistanceSquared(purgeZone.Center, playerInfo.Position) < (purgeZone.Radius * purgeZone.Radius + bufferZoneSquared)) {
				//You are inside or close to a purge zone right now
				pBlackboard->ChangeData("CurrentPurgeZone", purgeZone);
				return true;
			}
		}
		return true;
	}

	bool IsEnemyInFOV(Blackboard* pBlackboard) {
		std::vector<EnemyInfo>* pEnemiesInFOV{};

		bool dataFound = pBlackboard->GetData("EnemiesInFOV", pEnemiesInFOV);
		if (dataFound == false || pEnemiesInFOV == nullptr) {
			return false;
		}

		//If this vectors size is bigger than 0 it means there is one or more enemies in the FOV
		return pEnemiesInFOV->size() > 0;
	}

	bool HasGun(Blackboard* pBlackboard) {
		Inventory* pInventory{};

		bool dataFound = pBlackboard->GetData("Inventory", pInventory);

		if (dataFound == false || pInventory == nullptr) {
			return false;
		}

		//If you have a pistol, or a shotgun, you are armed
		bool hasGun = (pInventory->ContainsItemOfType(eItemType::PISTOL) || pInventory->ContainsItemOfType(eItemType::SHOTGUN));
		return hasGun;
	}

	bool WasFleeing(Blackboard* pBlackboard) {
		bool wasFleeing{};

		bool dataFound = pBlackboard->GetData("WasFleeing", wasFleeing);

		if (dataFound == false) {
			return false;
		}

		return wasFleeing;
	}

	bool IsFleeing(Blackboard* pBlackboard) {
		bool IsFleeing{};

		bool dataFound = pBlackboard->GetData("IsFleeing", IsFleeing);

		if (dataFound == false) {
			return false;
		}

		return IsFleeing;
	}

	bool IsHurt(Blackboard* pBlackboard) {
		AgentInfo playerInfo{};
		float maxHealth{};

		bool dataFound = pBlackboard->GetData("PlayerInfo", playerInfo) &&
			pBlackboard->GetData("MaxPlayerHealth", maxHealth);

		if (dataFound == false) {
			return false;
		}

		//If you have less health than max health you are hurt
		return playerInfo.Health < maxHealth;
	}

	bool ShouldHeal(Blackboard* pBlackboard) {
		AgentInfo playerInfo{};
		Inventory* pInventory{};
		float maxHealth{};
		IExamInterface* pInterface{};

		bool dataFound = pBlackboard->GetData("PlayerInfo", playerInfo) && 
			pBlackboard->GetData("Inventory", pInventory) &&
			pBlackboard->GetData("MaxPlayerHealth", maxHealth) &&
			pBlackboard->GetData("Interface", pInterface);

		if (dataFound == false || pInventory == nullptr || pInterface == nullptr) {
			return false;
		}
		//If you dont have a medkit, you shouldnt try to heal
		if (!pInventory->ContainsItemOfType(eItemType::MEDKIT)) {
			return false;
		}
		UINT medkitIndex = pInventory->GetIndexOfItemOfType(eItemType::MEDKIT);
		if (medkitIndex == invalid_index) {
			return false;
		}
		//Check the medkit charges
		ItemInfo item{};
		pInterface->Inventory_GetItem(medkitIndex, item);
		int medkitCharges = pInterface->Medkit_GetHealth(item);

		//Check so you dont waste any medkit charges
		if (maxHealth - playerInfo.Health > medkitCharges) {
			return true;
		}

		return false;
	}

	bool IsHungry(Blackboard* pBlackboard) {
		AgentInfo playerInfo{};
		float maxEnergy{};

		bool dataFound = pBlackboard->GetData("PlayerInfo", playerInfo) &&
			pBlackboard->GetData("MaxPlayerEnergy", maxEnergy);

		if (dataFound == false) {
			return false;
		}

		//If you have less energy than the max energy, you are hungry
		return playerInfo.Energy < maxEnergy;
	}

	bool ShouldEat(Blackboard* pBlackboard) {
		AgentInfo playerInfo{};
		Inventory* pInventory{};
		float maxEnergy{};
		IExamInterface* pInterface{};

		bool dataFound = pBlackboard->GetData("PlayerInfo", playerInfo) &&
			pBlackboard->GetData("Inventory", pInventory) &&
			pBlackboard->GetData("MaxPlayerEnergy", maxEnergy) &&
			pBlackboard->GetData("Interface", pInterface);

		if (dataFound == false || pInventory == nullptr || pInterface == nullptr) {
			return false;
		}
		//If you dont have food you shouldnt eat
		if (!pInventory->ContainsItemOfType(eItemType::FOOD)) {
			return false;
		}
		UINT foodIndex = pInventory->GetIndexOfItemOfType(eItemType::FOOD);
		if (foodIndex == invalid_index) {
			return false;
		}
		//Check the food energy
		ItemInfo item{};
		pInterface->Inventory_GetItem(foodIndex, item);
		int foodEnergy = pInterface->Food_GetEnergy(item);

		//Check so you dont waste any food energy
		if (maxEnergy - playerInfo.Energy > foodEnergy) {
			return true;
		}

		return false;
	}

	bool WasBitten(Blackboard* pBlackboard) {
		bool isInDanger{};
		AgentInfo playerInfo{};

		bool dataFound = pBlackboard->GetData("IsInDanger", isInDanger) &&
			pBlackboard->GetData("PlayerInfo", playerInfo);

		if (dataFound == false) {
			return false;
		}

		if (playerInfo.WasBitten) {
			pBlackboard->ChangeData("IsInDanger", true);
			return true;
		}

		return isInDanger;
	}

	bool IsItemInFOV(Blackboard* pBlackboard) {
		std::vector<EntityInfo>* pItemsInFOV{};

		bool dataFound = pBlackboard->GetData("ItemsInFOV", pItemsInFOV);
		if (dataFound == false) {
			return false;
		}

		//If this vectors size is bigger than 0 it means there is one or more items in the FOV
		return pItemsInFOV->size() > 0;
	}

	bool IsBitten(Blackboard* pBlackboard) {
		AgentInfo playerInfo{};

		bool dataFound = pBlackboard->GetData("PlayerInfo", playerInfo);

		if (dataFound == false) {
			return false;
		}

		//Set that you are in danger once bitten
		if (playerInfo.Bitten) {
			pBlackboard->ChangeData("IsInDanger", true);
		}
		
		return playerInfo.Bitten;
	}

	bool IsItemInPickupRange(Blackboard* pBlackboard) {
		AgentInfo playerInfo{};
		EntityInfo itemInfo{};

		bool dataFound = pBlackboard->GetData("PlayerInfo", playerInfo) &&
			pBlackboard->GetData("ClosestItem", itemInfo);

		if (dataFound == false) {
			return false;
		}

		//if the item info is empty
		if (itemInfo.EntityHash == 0) {
			return false;
		}

		if (Elite::DistanceSquared(playerInfo.Position, itemInfo.Location) < playerInfo.GrabRange * playerInfo.GrabRange) {
			return true;
		}

		return false;
	}

	bool IsFacingTarget(Blackboard* pBlackboard) {
		AgentInfo playerInfo{};
		Elite::Vector2 target{};
		const float acceptanceAngle{ 0.08f };

		bool dataFound = pBlackboard->GetData("PlayerInfo", playerInfo) &&
			pBlackboard->GetData("Target", target);

		if (dataFound == false) {
			return false;
		}

		const Elite::Vector2 toTarget{ target - playerInfo.Position };

		//angle between -pi and +pi
		const float angleTo{ atan2f(toTarget.y, toTarget.x) };
		float angleFrom{ playerInfo.Orientation };

		float deltaAngle = angleTo - angleFrom;

		if (abs(deltaAngle) < acceptanceAngle) {
			return true;
		}

		return false;

	}

	bool IsInsideHouse(Blackboard* pBlackboard) {
		std::vector<HouseSearch>* pKnownHouses{};
		AgentInfo playerInfo{};

		bool dataFound = pBlackboard->GetData("KnownHouses", pKnownHouses) &&
			pBlackboard->GetData("PlayerInfo", playerInfo);

		if (dataFound == false || pKnownHouses == nullptr) {
			return false;
		}
	
		//Check if you are insideof the house
		for (HouseSearch& houseSearch : *pKnownHouses) {
			if (houseSearch.IsPointInsideHouse(playerInfo.Position)) {
				//Set the current house as the house you are in
				pBlackboard->ChangeData("CurrentHouse", &houseSearch);
				return true;
			}
		}
		//You are not inside a known house
		return false;
	}

	bool ShouldSearchHouse(Blackboard* pBlackboard) {
		HouseSearch* pCurrentHouse{};

		bool dataFound = pBlackboard->GetData("CurrentHouse", pCurrentHouse);

		if (dataFound == false || pCurrentHouse == nullptr) {
			return false;
		}

		//This will be true if the house was not fully searched, or if it has been some time since it has been searched
		return pCurrentHouse->shouldCheck;
	}

	bool ShouldSearchKnownHouse(Blackboard* pBlackboard) {
		std::vector<HouseSearch>* pKnownHouses{};

		bool dataFound = pBlackboard->GetData("KnownHouses", pKnownHouses);

		if (dataFound == false || pKnownHouses == nullptr) {
			return false;
		}

		//If any of the houses you know should be looted, check them out
		for (HouseSearch& houseSearch : *pKnownHouses) {
			if (houseSearch.shouldCheck) {
				pBlackboard->ChangeData("CurrentHouse", &houseSearch);
				return true;
			}
		}

		return false;
	}

	bool ShouldPickUpClosestItem(Blackboard* pBlackboard) {
		EntityInfo closestItem{};
		Inventory* pInventory{};
		IExamInterface* pInterface{};

		auto dataFound = pBlackboard->GetData("ClosestItem", closestItem) &&
			pBlackboard->GetData("Inventory", pInventory) &&
			pBlackboard->GetData("Interface", pInterface);

		if (dataFound == false || pInventory == nullptr || pInterface == nullptr) {
			return false;
		}

		//Closest item is empty
		if (closestItem.EntityHash == 0) {
			return false;
		}
		
		return pInventory->ShouldPickupItem(closestItem);
	}

	bool IsInNeedOfItem(Blackboard* pBlackboard) {
		Inventory* pInventory{};

		bool dataFound = pBlackboard->GetData("Inventory", pInventory);

		if (dataFound == false || pInventory == nullptr) {
			return false;
		}

		std::vector<eItemType> neededItemTypes{};

		if (!HasGun(pBlackboard)) {
			//set type of item you need to pistol and shotgun
			neededItemTypes.push_back(eItemType::PISTOL);
			neededItemTypes.push_back(eItemType::SHOTGUN);
		}

		if (IsHungry(pBlackboard)) {
			if (!pInventory->ContainsItemOfType(eItemType::FOOD)) {
				//set type of item you need to food
				neededItemTypes.push_back(eItemType::FOOD);
			}
		}

		//If you are hurt and have no medkit, you need a medkit
		if (IsHurt(pBlackboard)) {
			if (!pInventory->ContainsItemOfType(eItemType::MEDKIT)) {
				//set type of item you need to medkit
				neededItemTypes.push_back(eItemType::MEDKIT);
			}
		}

		pBlackboard->ChangeData("NeededItemTypes", neededItemTypes);

		if (neededItemTypes.size() > 0) {
			return true;
		}

		return false;
	}

	bool ShouldPickupKnownItem(Blackboard* pBlackboard) {
		std::vector<ItemInfo>* pKnownItems{};
		std::vector<eItemType> neededItemTypes{};
		AgentInfo playerInfo{};
		float maxItemWalkRange{};

		bool dataFound = pBlackboard->GetData("KnownItems", pKnownItems) &&
			pBlackboard->GetData("NeededItemTypes", neededItemTypes) &&
			pBlackboard->GetData("PlayerInfo", playerInfo) &&
			pBlackboard->GetData("MaxItemWalkRange", maxItemWalkRange);

		if (dataFound == false || pKnownItems == nullptr || neededItemTypes.size() <= 0) {
			return false;
		}

		//get the closest known item of the type you need
		float closestDistance = FLT_MAX;
		ItemInfo knownItem{};
		for (eItemType type : neededItemTypes) {
			for (const ItemInfo& item : *pKnownItems) {
				if (item.Type == type) {
					float distance = playerInfo.Position.Distance(item.Location);
					if (distance < closestDistance) {
						closestDistance = distance;
						knownItem = item;
					}
				}
			}
		}
		
		//if the closestdistance is still max, you dont known any items of those types, return false
		if (abs(closestDistance - FLT_MAX) <= 0.001f) {
			return false;
		}

		//if its further than the max walkrange, ignore it
		if (closestDistance > maxItemWalkRange) {
			return false;
		}

		//If you get to here, you should go and pick it up, so set the item location as target
		pBlackboard->ChangeData("Target", knownItem.Location);
		return true;
	}

}
#endif