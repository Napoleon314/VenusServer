////////////////////////////////////////////////////////////////////////////
//
//  Venus Server Source File.
//  Copyright (C), Venus Interactive Entertainment.2012
// -------------------------------------------------------------------------
//  File name:   VeBaseEntity.cpp
//  Version:     v1.00
//  Created:     13/11/2014 by Napoleon
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//  http://www.venusie.com
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include <RakPeerInterface.h>
#include <RakNetTypes.h>
#include <MessageIdentifiers.h>
#include <BitStream.h>

using namespace RakNet;

//--------------------------------------------------------------------------
VeEntityManagerBase::VeEntityManagerBase() : VeNetEntityManager(TYPE_BASE)
{

}
//--------------------------------------------------------------------------
VeEntityManagerBase::~VeEntityManagerBase()
{

}
//--------------------------------------------------------------------------
static VeNetEntity::DataEntityPtr CreateData(BitStream& kStream,
	const VeNetEntityPtr& spEntity)
{
	VeNetEntity::DataEntityPtr spDataEnt = VE_NEW VeNetEntity::DataEntity;
	spEntity->m_kFreeList.AttachBack(spDataEnt->m_kNode);
	spDataEnt->m_pkObject = spEntity;

	for(VeUInt32 i(0); i < spEntity->m_kProperties.Size(); ++i)
	{
		VeNetEntity::DataPtr spData;
		if((spEntity->m_kProperties[i])->HasFlag(VeNetEntity::Property::FLAG_DATABASE))
		{
			spData = VeNetEntity::CreateData(kStream, *spEntity->m_kProperties[i]);
		}
		else
		{
			spData = VeNetEntity::CreateData(*spEntity->m_kProperties[i]);
		}
		VE_ASSERT(spData);
		spData->m_u32Index = i;
		spData->m_pkHolder = spDataEnt;
		spDataEnt->m_kDataArray.PushBack(spData);
	}

	return spDataEnt;
}
//--------------------------------------------------------------------------
VeNetEntity::DataEntityPtr VeEntityManagerBase::CreateEntity(
	VeNetEntity& kEntity, BitStream& kStream)
{
	VeNetEntity::DataEntityPtr spData = CreateData(kStream, &kEntity);
	VE_ASSERT(spData);
	if(kEntity.m_eType == VeNetEntity::TYPE_GLOBAL)
	{
		if(kEntity.m_kDataArray.Empty())
		{
			spData->m_pkHolder = &m_kDirtyCache;
			kEntity.m_kDataArray.PushBack(spData);
			kEntity.m_kFreeList.AttachBack(spData->m_kNode);
		}
		VE_ASSERT(kEntity.m_kDataArray.Size());
		return kEntity.m_kDataArray.Front();
	}
	else
	{
		VE_ASSERT(kEntity.m_kProperties.Front()->GetType() == VeNetEntity::Property::TYPE_VALUE);
		VeNetEntity::Value& kValue = (VeNetEntity::Value&)*kEntity.m_kProperties.Front();
		VE_ASSERT(kValue.m_spKeyMap);
		if(kValue.m_spKeyMap->GetType() == VeNetEntity::KeyMap::TYPE_STRING)
		{
			VE_ASSERT(kValue.m_eType == VeNetEntity::VALUE_VeString);
			VeStringMap<VeUInt32>& kMap = ((VeNetEntity::StrMap&)*kValue.m_spKeyMap).m_kMap;
			VeNetEntity::DataValue& kValue = (VeNetEntity::DataValue&)*spData->m_kDataArray.Front();
			VeUInt32* pu32Iter = kMap.Find((const VeChar8*)kValue.m_pbyBuffer + 2);
			if(!pu32Iter)
			{
				pu32Iter = &kMap[(const VeChar8*)kValue.m_pbyBuffer + 2];
				*pu32Iter = kEntity.m_kDataArray.Size();
				spData->m_pkHolder = &m_kDirtyCache;
				kEntity.m_kDataArray.PushBack(spData);
				kEntity.m_kFreeList.AttachBack(spData->m_kNode);
			}
			VE_ASSERT(pu32Iter);
			return kEntity.m_kDataArray[*pu32Iter];
		}
	}
	return NULL;
}
//--------------------------------------------------------------------------
VeEntityManagerBase::EntityCache::EntityCache()
{

}
//--------------------------------------------------------------------------
VeEntityManagerBase::EntityCache::~EntityCache()
{

}
//--------------------------------------------------------------------------
VeNetEntity::Data::Type VeEntityManagerBase::EntityCache::GetType() const
{
	return VeNetEntity::Data::TYPE_MAX;
}
//--------------------------------------------------------------------------
void VeEntityManagerBase::EntityCache::NotifyDirty(
	VeRefNode<VeNetEntity::Data*>& kNode)
{
	if(kNode.m_tContent && kNode.m_tContent->GetType()
		== VeNetEntity::Data::TYPE_ENTITY)
	{
		m_kDirtyList.AttachBack(kNode);
	}
}
//--------------------------------------------------------------------------
