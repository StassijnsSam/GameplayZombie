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

		float acceptanceRadius{ 2.0f };

		bool dataFound = pBlackboard->GetData("Target", target) &&
			pBlackboard->GetData("PlayerInfo", playerInfo) &&
			pBlackboard->GetData("Interface", pInterface) &&
			pBlackboard->GetData("CanRun", canRun);

		if (!dataFound || pInterface == nullptr) {
			return BehaviorState::Failure;
		}

		target = pInterface->NavMesh_GetClosestPathPoint(target);

		//If it is close to the target, let him stop
		if (Elite::DistanceSquared(target, playerInfo.Position) < acceptanceRadius * acceptanceRadius) {
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
		Elite::Vector2 target{};
		AgentInfo playerInfo{};
		IExamInterface* pInterface{};
		SteeringPlugin_Output steering{};
		bool canRun{};

		float fleeRadius{ 50.f };

		bool dataFound = pBlackboard->GetData("Target", target) &&
			pBlackboard->GetData("PlayerInfo", playerInfo) &&
			pBlackboard->GetData("Interface", pInterface) &&
			pBlackboard->GetData("CanRun", canRun);

		if (!dataFound || pInterface == nullptr) {
			return BehaviorState::Failure;
		}

		target = pInterface->NavMesh_GetClosestPathPoint(target);

		//If it is far enough from the target, let him stop
		if (Elite::DistanceSquared(target, playerInfo.Position) > fleeRadius * fleeRadius) {
			//Set isfleeing and wasfleeing
			pBlackboard->ChangeData("WasFleeing", true);
			pBlackboard->ChangeData("IsFleeing", false);
			pBlackboard->ChangeData("CanRun", false);
			steering.LinearVelocity = { 0, 0 };
		}
		else {
			steering.RunMode = canRun;
			steering.LinearVelocity = playerInfo.Position - target;
			steering.LinearVelocity.Normalize();
			steering.LinearVelocity *= playerInfo.MaxLinearSpeed;
		}

		pBlackboard->ChangeData("SteeringOutput", steering);
		return BehaviorState::Success;
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

	BehaviorState GetReadyToEscapePurgeZone(Blackboard* pBlackboard) {
		AgentInfo playerInfo{};
		PurgeZoneInfo currentPurgeZone{};

		bool dataFound = pBlackboard->GetData("CurrentPurgeZone", currentPurgeZone) &&
			pBlackboard->GetData("PlayerInfo", playerInfo);

		if (dataFound == false) {
			return BehaviorState::Failure;
		}

		pBlackboard->ChangeData("Target", currentPurgeZone.Center);
		pBlackboard->ChangeData("CanRun", true);
		
		return BehaviorState::Success;
	}

	BehaviorState GetReadyToFlee(Blackboard* pBlackboard) {
		pBlackboard->ChangeData("IsFleeing", true);
		pBlackboard->ChangeData("WasFleeing", false);
		pBlackboard->ChangeData("CanRun", true);

		//Set sprint based on health, what type of enemy is closeby etc

		return BehaviorState::Success;
	}

	BehaviorState SetClosestEnemyAsTarget(Blackboard* pBlackboard) {
		AgentInfo playerInfo{};
		std::vector<EnemyInfo> enemiesInFOV{};

		bool dataFound = pBlackboard->GetData("EnemiesInFOV", enemiesInFOV) &&
			pBlackboard->GetData("PlayerInfo", playerInfo);

		if (dataFound == false) {
			return BehaviorState::Failure;
		}

		//If there are no enemies in the FOV, this cant work
		if (enemiesInFOV.size() <= 0) {
			return BehaviorState::Failure;
		}

		EnemyInfo closestEnemy{ enemiesInFOV.at(0) };
		float minDistanceSquared{Elite::DistanceSquared(playerInfo.Position, closestEnemy.Location)};
		for (const EnemyInfo& enemy : enemiesInFOV) {
			float distanceSquared{ Elite::DistanceSquared(playerInfo.Position, enemy.Location )};
			if (distanceSquared < minDistanceSquared) {
				closestEnemy = enemy;
				minDistanceSquared = distanceSquared;
			}
		}

		pBlackboard->ChangeData("Target", closestEnemy.Location);
		return BehaviorState::Success;
	}

	BehaviorState ShootTarget(Blackboard* pBlackboard) {
		Inventory* pInventory{};

		bool dataFound = pBlackboard->GetData("Inventory", pInventory);

		if (dataFound == false || pInventory == nullptr) {
			return BehaviorState::Failure;
		}

		//Prefer pistol over shotgun, If you have a pistol shoot it, else shoot the shotgun
		//	Change later on, based on how many enemies are in the FOV
		//	Inventory keeps track of the ammo and will throw away an empty gun automatically
		if (pInventory->ContainsItemOfType(eItemType::PISTOL)) {
			bool hasFired = pInventory->UseItemOfType(eItemType::PISTOL);
			if (hasFired) {
				return BehaviorState::Success;
			}	
		}
		else {
			if (pInventory->ContainsItemOfType(eItemType::SHOTGUN)) {
				bool hasFired = pInventory->UseItemOfType(eItemType::SHOTGUN);
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

		pBlackboard->ChangeData("Target", pointBehind);
		return BehaviorState::Success;
	}

	BehaviorState SetClosestItemAsTarget(Blackboard* pBlackboard) {
		AgentInfo playerInfo{};
		std::vector<EntityInfo> itemsInFOV{};

		bool dataFound = pBlackboard->GetData("ItemsInFOV", itemsInFOV) &&
			pBlackboard->GetData("PlayerInfo", playerInfo);

		if (dataFound == false) {
			return BehaviorState::Failure;
		}

		//If there are no items in the FOV, this cant work
		if (!(itemsInFOV.size() > 0)) {
			return BehaviorState::Failure;
		}

		EntityInfo closestItem{ itemsInFOV.at(0) };
		float minDistanceSquared{ Elite::DistanceSquared(playerInfo.Position, closestItem.Location) };
		for (const EntityInfo& item : itemsInFOV) {
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
		AgentInfo playerInfo{};
		EntityInfo closestItem{};
		Inventory* pInventory{};
		IExamInterface* pInterface{};

		auto dataFound = pBlackboard->GetData("PlayerInfo", playerInfo) &&
			pBlackboard->GetData("ClosestItem", closestItem) &&
			pBlackboard->GetData("Inventory", pInventory) && 
			pBlackboard->GetData("Interface", pInterface);

		if (dataFound == false || pInventory == nullptr || pInterface == nullptr) {
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
			return BehaviorState::Success;
		}

		//If it was impossible to pick it up, return faillure
		return BehaviorState::Failure;
	}

	BehaviorState SetClosestHouseAsTarget(Blackboard* pBlackboard) {
		std::vector<HouseInfo> housesInFOV{};
		AgentInfo PlayerInfo{};

		bool dataFound = pBlackboard->GetData("HousesInFOV", housesInFOV) &&
			pBlackboard->GetData("PlayerInfo", PlayerInfo);

		if (dataFound == false) {
			return BehaviorState::Failure;
		}

		if (housesInFOV.size() <= 0) {
			return BehaviorState::Failure;
		}

		HouseInfo closestHouse{ housesInFOV.at(0) };
		float closestDistanceSquared{ Elite::DistanceSquared(PlayerInfo.Position, closestHouse.Center) };
		for (const HouseInfo& house : housesInFOV) {
			float distanceSquared = Elite::DistanceSquared(PlayerInfo.Position, house.Center);
			if (distanceSquared < closestDistanceSquared) {
				closestHouse = house;
				closestDistanceSquared = distanceSquared;
			}
		}

		pBlackboard->ChangeData("Target", closestHouse.Center);
		return BehaviorState::Success;
	}
}

namespace BT_Conditions
{
	bool IsInPurgeZone(Blackboard* pBlackboard) {
		std::vector<PurgeZoneInfo> purgeZonesInFOV{};
		AgentInfo playerInfo{};
		
		bool dataFound = pBlackboard->GetData("PurgeZonesInFOV", purgeZonesInFOV)&&
			pBlackboard->GetData("PlayerInfo", playerInfo);

		//If its not found or the vector is empty, there are no purge zones nearby
		if (dataFound == false || purgeZonesInFOV.empty()) {
			return false;
		}

		for (const PurgeZoneInfo& purgeZone : purgeZonesInFOV) {
			if (Elite::DistanceSquared(purgeZone.Center, playerInfo.Position) < purgeZone.Radius * purgeZone.Radius) {
				//You are inside a purge zone right now
				pBlackboard->ChangeData("CurrentPurgeZone", purgeZone);
				return true;
			}
		}
		return false;
	}

	bool IsEnemyInFOV(Blackboard* pBlackboard) {
		std::vector<EnemyInfo> enemiesInFOV{};

		bool dataFound = pBlackboard->GetData("EnemiesInFOV", enemiesInFOV);
		if (dataFound == false) {
			return false;
		}

		//If this vectors size is bigger than 0 it means there is one or more enemies in the FOV
		return enemiesInFOV.size() > 0;
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
		float minimumHealthToHeal{};

		bool dataFound = pBlackboard->GetData("PlayerInfo", playerInfo) && 
			pBlackboard->GetData("Inventory", pInventory) &&
			pBlackboard->GetData("MinimumHealthToHeal", minimumHealthToHeal);

		if (dataFound == false || pInventory == nullptr) {
			return false;
		}
		//Only if you have a medkit and your health is below the minimum health to heal, you should heal
		if (pInventory->ContainsItemOfType(eItemType::MEDKIT)) {
			if (playerInfo.Health < minimumHealthToHeal) {
				return true;
			}
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
		float minimumEnergyToEat{};

		bool dataFound = pBlackboard->GetData("PlayerInfo", playerInfo) &&
			pBlackboard->GetData("Inventory", pInventory) &&
			pBlackboard->GetData("MinimumEnergyToEat", minimumEnergyToEat);

		if (dataFound == false || pInventory == nullptr) {
			return false;
		}
		//Only if you have food and you are below the minimum energy to eat, you should eat
		if (pInventory->ContainsItemOfType(eItemType::FOOD)) {
			if (playerInfo.Energy < minimumEnergyToEat) {
				return true;
			}
		}
		return false;
	}

	bool WasBitten(Blackboard* pBlackboard) {
		AgentInfo playerInfo{};

		bool dataFound = pBlackboard->GetData("PlayerInfo", playerInfo);

		if (dataFound == false) {
			return false;
		}

		return playerInfo.WasBitten;
	}

	bool IsItemInFOV(Blackboard* pBlackboard) {
		std::vector<EntityInfo> itemsInFOV{};

		bool dataFound = pBlackboard->GetData("ItemsInFOV", itemsInFOV);
		if (dataFound == false) {
			return false;
		}

		//If this vectors size is bigger than 0 it means there is one or more items in the FOV
		return itemsInFOV.size() > 0;
	}

	bool IsBitten(Blackboard* pBlackboard) {
		AgentInfo playerInfo{};

		bool dataFound = pBlackboard->GetData("PlayerInfo", playerInfo);

		if (dataFound == false) {
			return false;
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
		const float acceptanceAngle{ 0.01f };

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

	bool IsHouseInFov(Blackboard* pBlackboard) {
		std::vector<HouseInfo> housesInFOV{};

		bool dataFound = pBlackboard->GetData("HousesInFOV", housesInFOV);

		if (dataFound == false) {
			return false;
		}

		return housesInFOV.size() > 0;
	}
}
#endif