#pragma once

#include "Exam_HelperStructs.h"
#include "IExamInterface.h"

//-1 in UINT (UINT_MAX)
enum {
	invalid_index = 4294967295U
};

class Inventory final{
public:
	Inventory(IExamInterface* pInterface, int maxGunAmount, int maxMedkitAmount, int maxFoodAmount,
		int minGunAmmoAmount, int minMedkitChargeAmount, int minFoodEnergyAmount);
	~Inventory() = default;

	//Delete copy and move constructors and operators
	Inventory(const Inventory& inventory) = delete;
	Inventory(Inventory&& inventory) = delete;
	Inventory& operator=(const Inventory& inventory) = delete;
	Inventory& operator=(Inventory&& inventory) = delete;

	UINT GetFreeSlot() const;
	bool ContainsItemOfType(eItemType itemType) const;
	bool PickupItem(EntityInfo item);
	bool UseItemOfType(eItemType itemType);

private:
	//Size could be a non constant in case you can pick up extra backpacks with storage space etc
	const UINT m_Size{};
	const int m_MaxGunAmount;
	const int m_MaxMedkitAmount;
	const int m_MaxFoodAmount;

	const int m_MinGunAmmoAmount;
	const int m_MinMedkitChargeAmount;
	const int m_MinFoodEnergyAmount;

	IExamInterface* m_pInterface{ nullptr };
	std::vector<ItemInfo> m_Items;

	int GetAmountOfType(eItemType itemType) const;
	bool IsFull() const;
	bool HasTooManyOfType(eItemType itemType) const;
	UINT GetAlmostEmptyItemOfType(eItemType itemType);
};