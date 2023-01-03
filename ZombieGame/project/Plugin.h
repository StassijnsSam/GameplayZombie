#pragma once
#include "IExamPlugin.h"
#include "Exam_HelperStructs.h"
#include "Structs.h"

class IBaseInterface;
class IExamInterface;
class Blackboard;
class BehaviorTree;
class Inventory;

class Plugin :public IExamPlugin
{
public:
	Plugin() {};
	virtual ~Plugin() {};

	void Initialize(IBaseInterface* pInterface, PluginInfo& info) override;
	void DllInit() override;
	void DllShutdown() override;

	void InitGameDebugParams(GameDebugParams& params) override;
	void Update(float dt) override;

	SteeringPlugin_Output UpdateSteering(float dt) override;
	void Render(float dt) const override;

private:
	//Interface, used to request data from/perform actions with the AI Framework
	IExamInterface* m_pInterface = nullptr;
	std::vector<HouseInfo> GetHousesInFOV() const;
	std::vector<EntityInfo> GetEntitiesInFOV() const;

	Elite::Vector2 m_Target = {};
	bool m_CanRun = false; //Demo purpose
	bool m_GrabItem = false; //Demo purpose
	bool m_UseItem = false; //Demo purpose
	bool m_RemoveItem = false; //Demo purpose
	float m_AngSpeed = 0.f; //Demo purpose

	UINT m_InventorySlot = 0;

	//Added member variables
	float m_MaxWasFleeingTime{ 2.0f };
	float m_WasFleeingTimer{ m_MaxWasFleeingTime };

	float m_MaxRunningTime{ 1.3f };
	float m_IsRunningTimer{ m_MaxRunningTime };

	Blackboard* m_pBlackboard{ nullptr };
	BehaviorTree* m_pBehaviorTree{nullptr};
	Inventory* m_pInventory{ nullptr };

	std::vector<EntityInfo> m_ItemsInFOV{};
	std::vector<EnemyInfo> m_EnemiesInFOV{};
	std::vector<PurgeZoneInfo> m_PurgeZonesInFOV{};
	std::vector<HouseInfo> m_HousesInFOV{};

	std::vector<HouseSearch> m_KnownHouses{};

	void ClearData();
	void UpdateEntitiesFOV();
	void UpdateHousesFOV();
	void UpdateWasFleeingTimer(float dt);
	void UpdateIsRunningTimer(float dt);

};

//ENTRY
//This is the first function that is called by the host program
//The plugin returned by this function is also the plugin used by the host program
extern "C"
{
	__declspec (dllexport) IPluginBase* Register()
	{
		return new Plugin();
	}
}