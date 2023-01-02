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
			steering.LinearVelocity = { 0, 0 };
			//turn around to where you ran from? so you can run further if still being chased
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

	BehaviorState EscapePurgeZone(Blackboard* pBlackboard) {
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
}
#endif