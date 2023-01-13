#include "stdafx.h"
#include "Plugin.h"
#include "IExamInterface.h"
#include "EBlackboard.h"
#include "EBehaviorTree.h"
#include "Behaviors.h"
#include "Inventory.h"

using namespace std;

//Called only once, during initialization
void Plugin::Initialize(IBaseInterface* pInterface, PluginInfo& info)
{
	//Retrieving the interface
	//This interface gives you access to certain actions the AI_Framework can perform for you
	m_pInterface = static_cast<IExamInterface*>(pInterface);

	//Bit information about the plugin
	//Please fill this in!!
	info.BotName = "MinionExam";
	info.Student_FirstName = "Sam";
	info.Student_LastName = "Stassijns";
	info.Student_Class = "2DAE08";
	
	//Create inventory
	//Interface, max gun amount, max medkit amount, max food amount, min gun ammo amount (to reject), min medkit charge amount, min food energy amount
	m_pInventory = new Inventory(m_pInterface, 2, 2, 1, 2, 2, 2);
	//Create blackboard
	m_pBlackboard = new Blackboard();
	//Make the worldSearch
	m_pWorldSearch = new WorldSearch(m_pInterface->World_GetInfo());

	//Add blackboard data
	m_pBlackboard->AddData("Interface", m_pInterface);
	m_pBlackboard->AddData("PlayerInfo", m_pInterface->Agent_GetInfo());
	m_pBlackboard->AddData("WorldInfo", m_pInterface->World_GetInfo());
	m_pBlackboard->AddData("SteeringOutput", SteeringPlugin_Output{});
	m_pBlackboard->AddData("Inventory", m_pInventory);
	m_pBlackboard->AddData("FleeRadius", 60.f);
	m_pBlackboard->AddData("EnemiesInFOV", m_EnemiesInFOV);

	//Items
	m_pBlackboard->AddData("ItemsInFOV", m_ItemsInFOV);
	m_pBlackboard->AddData("KnownItems", &m_KnownItems);
	m_pBlackboard->AddData("ClosestItem", EntityInfo{});
	m_pBlackboard->AddData("NeededItemTypes", std::vector<eItemType>());
	m_pBlackboard->AddData("MaxItemWalkRange", 300.f);

	//Purge zone
	m_pBlackboard->AddData("PurgeZonesInFOV", m_PurgeZonesInFOV);
	m_pBlackboard->AddData("CurrentPurgeZone", PurgeZoneInfo{});

	m_pBlackboard->AddData("CanRun", m_CanRun);
	m_pBlackboard->AddData("WasFleeing", false);
	m_pBlackboard->AddData("IsFleeing", false);
	m_pBlackboard->AddData("Target", Elite::Vector2{0, 0});
	m_pBlackboard->AddData("FleeTarget", Elite::Vector2{ 0, 0 });

	//Health
	m_pBlackboard->AddData("MaxPlayerHealth", 10.0f);
	m_pBlackboard->AddData("IsInDanger", false);

	//Food
	m_pBlackboard->AddData("MaxPlayerEnergy", 10.0f);

	//Houses
	m_pBlackboard->AddData("HousesInFOV", m_HousesInFOV);
	m_pBlackboard->AddData("KnownHouses", &m_KnownHouses);
	m_pBlackboard->AddData("CurrentHouse", &HouseSearch{});
	m_pBlackboard->AddData("ClosestHouse", HouseInfo{});
	
	//World
	m_pBlackboard->AddData("WorldSearch", m_pWorldSearch);

	//Create behaviorTree
	m_pBehaviorTree = new BehaviorTree(m_pBlackboard,
		//Root
		new BehaviorSelector({
			//Run from purge zone
			new BehaviorSequence({
				new BehaviorConditional(BT_Conditions::IsInPurgeZone),
				new BehaviorAction(BT_Actions::GetReadyToEscapePurgeZone),
				new BehaviorAction(BT_Actions::Flee)
			}),
			//Use items needed to survive
			new BehaviorSelector({
				//Use medkit
				new BehaviorSequence({
					new BehaviorConditional(BT_Conditions::IsHurt),
					new BehaviorConditional(BT_Conditions::ShouldHeal),
					new BehaviorAction(BT_Actions::UseMedkit)
				}),
				//Eat food
				new BehaviorSequence({
					new BehaviorConditional(BT_Conditions::IsHungry),
					new BehaviorConditional(BT_Conditions::ShouldEat),
					new BehaviorAction(BT_Actions::EatFood)
				})
			}),
			//Enemy spotted
			new BehaviorSequence({
				new BehaviorConditional(BT_Conditions::IsEnemyInFOV),
				new BehaviorAction(BT_Actions::SetClosestEnemyAsTarget),
				new BehaviorSelector({
					//If you are facing an enemy, shoot it
					new BehaviorSequence({	
						new BehaviorConditional(BT_Conditions::HasGun),
						new BehaviorConditional(BT_Conditions::IsFacingTarget),
						new BehaviorAction(BT_Actions::ShootTarget)
					}),
					//If you see an enemy and have a gun, face it while walking back
					new BehaviorSequence({
						new BehaviorConditional(BT_Conditions::HasGun),
						new BehaviorAction(BT_Actions::Flee),
						new BehaviorAction(BT_Actions::FaceBehind)
					}),

					//If you see an enemy and have no gun, get ready to flee (and later find a weapon)
					new BehaviorSequence({
						new InvertedBehaviorConditional(BT_Conditions::HasGun),
						new BehaviorAction(BT_Actions::GetReadyToFlee),
						new BehaviorAction(BT_Actions::Flee)
					}),
				}),
			}),
			//Bitten by enemy
			new BehaviorSelector({
				//If you just got bitten and no enemy is in the FOV, set the target behind you
				new BehaviorSequence({
					new BehaviorConditional(BT_Conditions::IsBitten),
					new InvertedBehaviorConditional(BT_Conditions::IsEnemyInFOV),
					new BehaviorAction(BT_Actions::SetTargetBehindPlayer)
					}),
				//If you were bitten and have a gun, turn around while walking away
				new BehaviorSequence({
					new BehaviorConditional(BT_Conditions::WasBitten),
					new BehaviorConditional(BT_Conditions::HasGun),
					new BehaviorAction(BT_Actions::Flee),
					new BehaviorAction(BT_Actions::FaceBehind)
				}),
				//If you were bitten and have no gun, run away
				new BehaviorSequence({
					new BehaviorConditional(BT_Conditions::WasBitten),
					new InvertedBehaviorConditional(BT_Conditions::HasGun),
					new BehaviorAction(BT_Actions::GetReadyToFlee),
					new BehaviorAction(BT_Actions::Flee)
				})
			}),
			//Look behind you if you were fleeing
			new BehaviorSelector({
				//If still within flee radius, continue fleeing
				new BehaviorSequence({
					new BehaviorConditional(BT_Conditions::IsFleeing),
					new BehaviorAction(BT_Actions::Flee)
					}),
				//When you are done fleeing, turn around to see if you are still being followed
				new BehaviorSequence({
					new BehaviorConditional(BT_Conditions::WasFleeing),
					new BehaviorAction(BT_Actions::FaceBehind)
					})
			}),
			//Pickup items
			new BehaviorSequence({
				new BehaviorConditional(BT_Conditions::IsItemInFOV),
				new BehaviorSelector({
					//If an item is within pickup range try to pick it up
					new BehaviorSequence({
						new BehaviorConditional(BT_Conditions::IsItemInPickupRange),
						new BehaviorAction(BT_Actions::SetClosestItemAsTarget),
						new BehaviorConditional(BT_Conditions::ShouldPickUpClosestItem),
						new BehaviorAction(BT_Actions::PickUpClosestItem)
					}),
					//If an item is not within pickup range move to it
					new BehaviorSequence({
						new BehaviorAction(BT_Actions::SetClosestItemAsTarget),
						new BehaviorConditional(BT_Conditions::ShouldPickUpClosestItem),
						new BehaviorAction(BT_Actions::Seek)
					}),
					//If you should not pick up the item, save it as a known item
					new BehaviorSequence({
						new BehaviorAction(BT_Actions::SetClosestItemAsTarget),
						new InvertedBehaviorConditional(BT_Conditions::ShouldPickUpClosestItem),
						new BehaviorAction(BT_Actions::RememberItem)
					})
				}),
			}),
			//Go to known items you need 
			new BehaviorSequence({
				new BehaviorConditional(BT_Conditions::IsInNeedOfItem),
				new BehaviorConditional(BT_Conditions::ShouldPickupKnownItem),
				new BehaviorAction(BT_Actions::Seek)
			}),
			//Explore houses
			new BehaviorSelector({
				//If you are currently inside of a house, and should explore it, search all locations
				new BehaviorSequence({
					new BehaviorConditional(BT_Conditions::IsInsideHouse),
					new BehaviorConditional(BT_Conditions::ShouldSearchHouse),
					new BehaviorAction(BT_Actions::SearchHouse)
				}),
				//If any of the houses you already know should be looted, loot that
				new BehaviorSequence({
					new BehaviorConditional(BT_Conditions::ShouldSearchKnownHouse),
					new BehaviorAction(BT_Actions::SearchHouse)
				})
			}),
			//Explore world
			new BehaviorAction(BT_Actions::ExploreWorld),

			//Any fallback behavior (go to current target)
		})
	);

}

//Called only once
void Plugin::DllInit()
{
	//Called when the plugin is loaded
}

//Called only once
void Plugin::DllShutdown()
{
	SAFE_DELETE(m_pInventory);
	SAFE_DELETE(m_pBehaviorTree);
	//BehaviorTree takes ownership of passed blackboard, so no need to delete here
}

//Called only once, during initialization
void Plugin::InitGameDebugParams(GameDebugParams& params)
{
	params.AutoFollowCam = true; //Automatically follow the AI? (Default = true)
	params.RenderUI = true; //Render the IMGUI Panel? (Default = true)
	params.SpawnEnemies = true; //Do you want to spawn enemies? (Default = true)
	params.EnemyCount = 20; //How many enemies? (Default = 20)
	params.GodMode = false; //GodMode > You can't die, can be useful to inspect certain behaviors (Default = false)
	params.LevelFile = "GameLevel.gppl";
	params.AutoGrabClosestItem = true; //A call to Item_Grab(...) returns the closest item that can be grabbed. (EntityInfo argument is ignored)
	params.StartingDifficultyStage = 1;
	params.InfiniteStamina = false;
	params.SpawnDebugPistol = true;
	params.SpawnDebugShotgun = true;
	params.SpawnPurgeZonesOnMiddleClick = true;
	params.PrintDebugMessages = true;
	params.ShowDebugItemNames = true;
	params.Seed = 36;
}

//Only Active in DEBUG Mode
//(=Use only for Debug Purposes)
void Plugin::Update(float dt)
{
	//Demo Event Code
	//In the end your AI should be able to walk around without external input
	if (m_pInterface->Input_IsMouseButtonUp(Elite::InputMouseButton::eLeft))
	{
		//Update target based on input
		Elite::MouseData mouseData = m_pInterface->Input_GetMouseData(Elite::InputType::eMouseButton, Elite::InputMouseButton::eLeft);
		const Elite::Vector2 pos = Elite::Vector2(static_cast<float>(mouseData.X), static_cast<float>(mouseData.Y));
		m_Target = m_pInterface->Debug_ConvertScreenToWorld(pos);
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Space))
	{
		m_CanRun = true;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Left))
	{
		m_AngSpeed -= Elite::ToRadians(10);
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Right))
	{
		m_AngSpeed += Elite::ToRadians(10);
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_G))
	{
		m_GrabItem = true;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_U))
	{
		m_UseItem = true;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_R))
	{
		m_RemoveItem = true;
	}
	else if (m_pInterface->Input_IsKeyboardKeyUp(Elite::eScancode_Space))
	{
		m_CanRun = false;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Delete))
	{
		m_pInterface->RequestShutdown();
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_KP_Minus))
	{
		if (m_InventorySlot > 0)
			--m_InventorySlot;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_KP_Plus))
	{
		if (m_InventorySlot < 4)
			++m_InventorySlot;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Q))
	{
		ItemInfo info = {};
		m_pInterface->Inventory_GetItem(m_InventorySlot, info);
		std::cout << (int)info.Type << std::endl;
	}
}

//Update
//This function calculates the new SteeringOutput, called once per frame
SteeringPlugin_Output Plugin::UpdateSteering(float dt)
{
	//Clear all the data
	ClearData();
	//Reset State
	m_GrabItem = false;
	m_UseItem = false;
	m_RemoveItem = false;

	//Fill in the new data
	//	Fill in the agent info and update blackboard
	AgentInfo agentInfo = m_pInterface->Agent_GetInfo();
	m_pBlackboard->ChangeData("PlayerInfo", agentInfo);
	//	Fill in all the entities in the FOV in their respective categories and update blackboard
	UpdateEntitiesFOV();
	UpdateHousesFOV();
	//Update the timer on known houses so you will loot them again after some time
	UpdateKnownHouses(dt);
	//Update the fleeing timer
	UpdateWasFleeingTimer(dt);
	UpdateIsFleeingTimer(dt);
	//Update the canrun timer
	UpdateIsRunningTimer(dt);
	//Update danger timer
	UpdateIsInDangerTimer(dt);

	//Update the behaviorTree (with the new data)
	m_pBehaviorTree->Update(dt);

	//Get the steering
	SteeringPlugin_Output steering{};
	bool steeringFound = m_pBlackboard->GetData("SteeringOutput", steering);
	if (!steeringFound) {
		std::cout << "Steering was not found in the blackboard" << std::endl;
	}
 
	return steering;
}

//This function should only be used for rendering debug elements
void Plugin::Render(float dt) const
{
	//This Render function should only contain calls to Interface->Draw_... functions
	m_pInterface->Draw_SolidCircle(m_Target, .7f, { 0,0 }, { 1, 0, 0 });

	Elite::Vector2 target{};
	m_pBlackboard->GetData("Target", target);

	m_pInterface->Draw_Point(target, 5.0f, Elite::Vector3{ 0, 1, 0 });
}

vector<HouseInfo> Plugin::GetHousesInFOV() const
{
	vector<HouseInfo> vHousesInFOV = {};

	HouseInfo hi = {};
	for (int i = 0;; ++i)
	{
		if (m_pInterface->Fov_GetHouseByIndex(i, hi))
		{
			vHousesInFOV.push_back(hi);
			continue;
		}

		break;
	}

	return vHousesInFOV;
}

vector<EntityInfo> Plugin::GetEntitiesInFOV() const
{
	vector<EntityInfo> vEntitiesInFOV = {};

	EntityInfo ei = {};
	for (int i = 0;; ++i)
	{
		if (m_pInterface->Fov_GetEntityByIndex(i, ei))
		{
			vEntitiesInFOV.push_back(ei);
			continue;
		}

		break;
	}

	return vEntitiesInFOV;
}

void Plugin::ClearData()
{
	m_HousesInFOV.clear();
	m_ItemsInFOV.clear();
	m_EnemiesInFOV.clear();
	m_PurgeZonesInFOV.clear();
	//Now done with timers
	//m_pBlackboard->ChangeData("CanRun", false);
	//m_pBlackboard->ChangeData("WasFleeing", false);
}

void Plugin::UpdateEntitiesFOV()
{
	std::vector<EntityInfo> entitiesInFOV = GetEntitiesInFOV();
	
	//Update the member variables then update the value in the blackboard
	for (const EntityInfo& entity : entitiesInFOV) {
		if (entity.Type == eEntityType::ITEM) {
			//Keep the items as entities so it is easier to destroy them
			m_ItemsInFOV.push_back(entity);
		}
		if (entity.Type == eEntityType::ENEMY) {
			EnemyInfo enemyInfo{};
			m_pInterface->Enemy_GetInfo(entity, enemyInfo);
			m_EnemiesInFOV.push_back(enemyInfo);
		}
		if (entity.Type == eEntityType::PURGEZONE) {
			PurgeZoneInfo purgeZoneInfo{};
			m_pInterface->PurgeZone_GetInfo(entity, purgeZoneInfo);
			m_PurgeZonesInFOV.push_back(purgeZoneInfo);
		}
	}
	m_pBlackboard->ChangeData("ItemsInFOV", m_ItemsInFOV);
	m_pBlackboard->ChangeData("EnemiesInFOV", m_EnemiesInFOV);
	m_pBlackboard->ChangeData("PurgeZonesInFOV", m_PurgeZonesInFOV);
}

void Plugin::UpdateHousesFOV()
{
	std::vector<HouseInfo> housesInFOV = GetHousesInFOV();
	//Update the member variable and update the blackboard
	m_HousesInFOV = housesInFOV;
	m_pBlackboard->ChangeData("HousesInFOV", m_HousesInFOV);

	bool isNewHouse{ true };
	//If it is a new house add it to the known houses
	for (const HouseInfo& house : housesInFOV) {
		for (const HouseSearch& houseSearch : m_KnownHouses) {
			//If the centers are very close together, it means it is the same house
			if (Elite::DistanceSquared(house.Center, houseSearch.Center) < houseSearch.acceptanceRadius * houseSearch.acceptanceRadius) {
				isNewHouse = false;
			}
		}
		//If you checked all the known houses and it is still true, it is a new house
		if (isNewHouse) {
			m_KnownHouses.push_back(HouseSearch(house));
			//Data in Blackboard is automatically changed since it is a pointer
		}
	}
}

void Plugin::UpdateKnownHouses(float dt)
{
	for (HouseSearch& houseSearch : m_KnownHouses) {
		houseSearch.UpdateTimeSinceLooted(dt);
	}
}

void Plugin::UpdateWasFleeingTimer(float dt)
{
	bool wasFleeing{};
	m_pBlackboard->GetData("WasFleeing", wasFleeing);

	if (wasFleeing) {
		m_WasFleeingTimer -= dt;
		if (m_WasFleeingTimer <= 0) {
			m_pBlackboard->ChangeData("WasFleeing", false);
			m_WasFleeingTimer = m_MaxWasFleeingTime;
		}
	}
}

void Plugin::UpdateIsFleeingTimer(float dt)
{
	bool isFleeing{};
	m_pBlackboard->GetData("IsFleeing", isFleeing);

	if (isFleeing) {
		m_IsFleeingTimer -= dt;
		if (m_IsFleeingTimer <= 0) {
			m_pBlackboard->ChangeData("IsFleeing", false);
			m_IsFleeingTimer = m_MaxIsFleeingTime;
		}
	}
}

void Plugin::UpdateIsRunningTimer(float dt)
{
	bool canRun{};
	m_pBlackboard->GetData("CanRun", canRun);

	if (canRun) {
		m_IsRunningTimer -= dt;
		if (m_IsRunningTimer <= 0) {
			m_pBlackboard->ChangeData("CanRun", false);
			m_IsRunningTimer = m_MaxRunningTime;
		}
	}
}

void Plugin::UpdateIsInDangerTimer(float dt)
{
	bool isInDanger{};
	m_pBlackboard->GetData("IsInDanger", isInDanger);

	if (isInDanger) {
		m_IsInDangerTimer -= dt;
		if (m_IsInDangerTimer <= 0) {
			m_pBlackboard->ChangeData("IsInDanger", false);
			m_IsInDangerTimer = m_MaxDangerTime;
		}
	}
}
