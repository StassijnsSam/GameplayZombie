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
#include "SteeringBehaviors.h"

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

		bool dataFound = pBlackboard->GetData("Target", target) &&
			pBlackboard->GetData("PlayerInfo", playerInfo) &&
			pBlackboard->GetData("Interface", pInterface) &&
			pBlackboard->GetData("CanRun", canRun);

		if (!dataFound || pInterface == nullptr) {
			return BehaviorState::Failure;
		}

		target = pInterface->NavMesh_GetClosestPathPoint(target);

		//If it is close to the target, let him stop
		if (Elite::DistanceSquared(target, playerInfo.Position) < 4.0f) {
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
}

namespace BT_Conditions
{
	
}
#endif