#include "stdafx.h"
#include "Inventory.h"
#include <map>

Inventory::Inventory(IExamInterface* pInterface, int maxGunAmount, int maxMedkitAmount, int maxFoodAmount, 
	int minGunAmmoAmount, int minMedkitChargeAmount, int minFoodEnergyAmount)
	:m_pInterface{ pInterface },
	m_Size{ pInterface->Inventory_GetCapacity() },
	m_MaxGunAmount{maxGunAmount},
	m_MaxMedkitAmount{maxMedkitAmount},
	m_MaxFoodAmount{maxFoodAmount},
	m_MinGunAmmoAmount{minGunAmmoAmount},
	m_MinMedkitChargeAmount{minMedkitChargeAmount},
	m_MinFoodEnergyAmount{minFoodEnergyAmount}
{
	m_Items.reserve(m_Size);
	//fill it up with Random drops, this is only internally used and will be seen as Empty
	ItemInfo emptyItem{};
	emptyItem.Type = eItemType::RANDOM_DROP;
	std::fill(m_Items.begin(), m_Items.end(), emptyItem);
}

UINT Inventory::GetFreeSlot() const
{
	for (UINT inventoryIndex{ 0 }; inventoryIndex < m_Size; ++inventoryIndex) {
		//Random drop is seen as empty
		if (m_Items.at(inventoryIndex).Type == eItemType::RANDOM_DROP) {
			return inventoryIndex;
		}
	}
	return invalid_index;
}

int Inventory::GetAmountOfType(eItemType itemType) const
{
	//Use count if to count the amount of items of a certain type
	const int itemAmount = std::count_if(m_Items.begin(), m_Items.end(), [itemType](ItemInfo item) {
		return (item.Type == itemType);
		});

	return itemAmount;
}

bool Inventory::HasTooManyOfType(eItemType itemType) const
{
	switch (itemType)
	{
	case eItemType::PISTOL:
	case eItemType::SHOTGUN:
		if (GetAmountOfType(eItemType::PISTOL) + GetAmountOfType(eItemType::SHOTGUN) >= m_MaxGunAmount) {
			return true;
		}
		return false;
		break;
	case eItemType::MEDKIT:
		if (GetAmountOfType(eItemType::MEDKIT) >= m_MaxMedkitAmount) {
			return true;
		}
		return false;
		break;
	case eItemType::FOOD:
		if (GetAmountOfType(eItemType::FOOD) >= m_MaxFoodAmount) {
			return true;
		}
		return false;
		break;
	}
	return false;
}

UINT Inventory::GetAlmostEmptyItemOfType(eItemType itemType)
{
	//If there are no items of this type, return invalid index
	if (!ContainsItemOfType(itemType)) {
		return invalid_index;
	}
	
	//Make a map of items to check since there can be multiple of a type
	std::map<UINT, ItemInfo> itemsToCheck{};
	for (UINT index{ 0 }; index < m_Size; ++index) {
		ItemInfo item = m_Items.at(index);
		if (item.Type == itemType) {
			itemsToCheck.insert(std::pair<UINT, ItemInfo>(index, item));
		}
	}

	//Check all the items and if any of them are almost empty, return the inventory index
	for (auto indexItemPair : itemsToCheck) {
		switch (itemType)
		{
		case eItemType::PISTOL:
		case eItemType::SHOTGUN:
			if (m_pInterface->Weapon_GetAmmo(indexItemPair.second) < m_MinGunAmmoAmount) {
				return indexItemPair.first;
			}
			break;
		case eItemType::MEDKIT:
			if (m_pInterface->Medkit_GetHealth(indexItemPair.second) < m_MinMedkitChargeAmount) {
				return indexItemPair.first;
			}
			break;
		case eItemType::FOOD:
			if (m_pInterface->Food_GetEnergy(indexItemPair.second) < m_MinFoodEnergyAmount) {
				return indexItemPair.first;
			}
			break;
		}
	}
	return invalid_index;
}

bool Inventory::ContainsItemOfType(eItemType itemType) const
{
	//Iterator for item
	auto itemIterator = std::find_if(m_Items.begin(), m_Items.end(), [itemType](ItemInfo item) {
		return item.Type == itemType;
		});

	//If the iterator is end, no item of this type was found in the inventory
	if (itemIterator == m_Items.end()) {
		return false;
	}

	return true;

	//Alternative implementation with GetAmountOfType == 0
}

bool Inventory::IsFull() const
{
	UINT freeSlotIndex = GetFreeSlot();
	//If there are no free slots the inventory is full
	return freeSlotIndex == invalid_index;
}

bool Inventory::PickupItem(EntityInfo item)
{
	//Check if the item is an actual item
	if (item.Type != eEntityType::ITEM) {
		return false;
	}
	
	ItemInfo itemInfo{};
	m_pInterface->Item_GetInfo(item, itemInfo);

	//If it is garbage, instantly destroy it
	if (itemInfo.Type == eItemType::GARBAGE) {
		m_pInterface->Item_Destroy(item);
		return true;
	}

	//First check if your current inventory is full
	if (!IsFull()) {

		//Check if you dont already have too many items of this type
		if (HasTooManyOfType(itemInfo.Type)) {
			return false;
		}

		//See if you can grab the item
		if (!m_pInterface->Item_Grab(item, itemInfo)) {
			return false;
		}

		//Get the first free slot and add the item to both inventories
		UINT slotIndex = GetFreeSlot();
		m_pInterface->Inventory_AddItem(slotIndex, itemInfo);
		m_Items[slotIndex] = itemInfo;
		return true;
	}
	//decide if this item is worth being picked up, over an item already in there
	else {
		UINT almostEmptyItemIndex = GetAlmostEmptyItemOfType(itemInfo.Type);
		if (almostEmptyItemIndex != invalid_index) {
			//Get rid of the item at that current index
			m_pInterface->Inventory_RemoveItem(almostEmptyItemIndex);
			//Pick up the new item and add it to both inventories
			//See if you can grab the item
			if (!m_pInterface->Item_Grab(item, itemInfo)) {
				return false;
			}

			m_pInterface->Inventory_AddItem(almostEmptyItemIndex, itemInfo);
			m_Items[almostEmptyItemIndex] = itemInfo;
			return true;
		}
		else {
			return false;
		}
	}
}
