////////////////////////////////////////////////////////////////////////////
//
//  Venus Server Source File.
//  Copyright (C), Venus Interactive Entertainment.2012
// -------------------------------------------------------------------------
//  File name:   VeBaseServer.cpp
//  Version:     v1.00
//  Created:     4/11/2014 by Napoleon
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
VeDatabaseLink::VeDatabaseLink(VeBaseServer* pkParent)
	: VeClientBase("database"), m_pkParent(pkParent), m_bEnable(false)
{

}
//--------------------------------------------------------------------------
VeDatabaseLink::~VeDatabaseLink()
{

}
//--------------------------------------------------------------------------
bool VeDatabaseLink::IsEnable()
{
	return m_bEnable;
}
//--------------------------------------------------------------------------
void VeDatabaseLink::OnConnect()
{
	printf("Database Server connect succeeded\n");
	m_bEnable = true;
	if(m_pkParent)
	{
		m_pkParent->_Start();
	}
}
//--------------------------------------------------------------------------
void VeDatabaseLink::OnConnectFailed(DisconnectType eType)
{
	printf("Database Server connect failed\n");
	m_bEnable = false;
}
//--------------------------------------------------------------------------
void VeDatabaseLink::OnDisconnect(DisconnectType eType)
{
	printf("Database server disconnected\n");
	m_bEnable = false;
}
//--------------------------------------------------------------------------
void VeDatabaseLink::ProcessUserEvent(VeUInt8 u8Event,
	RakNet::BitStream& kStream)
{
	switch(u8Event)
	{
	case ID_USER_PACKET_ENUM + RES_LOAD_ENTITY_SUCCEEDED + 1:
		{
			VeUInt32 u32Call;
			if(!kStream.Read(u32Call)) return;
			VeMap<VeUInt32,LuaHolder>::iterator it = m_kLuaCallMap.Find(u32Call);
			if(it == m_kLuaCallMap.End()) return;
			VeUInt16 u16Entity;
			if(!kStream.Read(u16Entity)) return;
			VeNetEntityPtr spEntity = m_pkParent->m_spEntityMgr->GetEntity(u16Entity);
			if(!spEntity) return;
			VeNetEntity::DataEntityPtr spData = m_pkParent->m_spEntityMgr->CreateEntity(*spEntity, kStream);
			if(spData)
			{
				VeBaseServer::PushLuaData(it->m_tSecond.m_tFirst, *spData);
				VeLuaBind::Resume(it->m_tSecond.m_tFirst, it->m_tSecond.m_tSecond, g_pLua->GetLua(), 1);
			}
			else
			{
				VeLuaBind::Resume(it->m_tSecond.m_tFirst, it->m_tSecond.m_tSecond, g_pLua->GetLua(), 0);
			}
			m_kLuaCallMap.Erase(it);
		}
		break;
	case ID_USER_PACKET_ENUM + RES_LOAD_ENTITY_FAILED + 1:
		{
			VeUInt32 u32Call;
			if(!kStream.Read(u32Call)) return;
			VeMap<VeUInt32,LuaHolder>::iterator it = m_kLuaCallMap.Find(u32Call);
			if(it == m_kLuaCallMap.End()) return;
			VeUInt16 u16Entity;
			if(!kStream.Read(u16Entity)) return;
			VeNetEntityPtr spEntity = m_pkParent->m_spEntityMgr->GetEntity(u16Entity);
			if(!spEntity) return;
			if(spEntity->m_eType == VeNetEntity::TYPE_GLOBAL)
			{
				if(spEntity->m_kDataArray.Size())
				{
					VeBaseServer::PushLuaData(it->m_tSecond.m_tFirst, *spEntity->m_kDataArray.Front());
					VeLuaBind::Resume(it->m_tSecond.m_tFirst, it->m_tSecond.m_tSecond, g_pLua->GetLua(), 1);
				}
				else
				{
					printf("Load global entity [%s] failed\n", (const VeChar8*)(spEntity->m_kName));
					VeLuaBind::Resume(it->m_tSecond.m_tFirst, it->m_tSecond.m_tSecond, g_pLua->GetLua(), 0);
				}
			}
			else
			{
				VE_ASSERT(spEntity->m_kProperties.Front()->GetType() == VeNetEntity::Property::TYPE_VALUE);
				VeNetEntity::Value& kValue = (VeNetEntity::Value&)*spEntity->m_kProperties.Front();
				VE_ASSERT(kValue.m_spKeyMap);
				if(kValue.m_spKeyMap->GetType() == VeNetEntity::KeyMap::TYPE_STRING)
				{
					VeStringMap<VeUInt32>& kMap = ((VeNetEntity::StrMap&)*kValue.m_spKeyMap).m_kMap;
					VeString kKey;
					if(!Read(kStream, kKey)) return;
					VeUInt32* pu32Iter = kMap.Find(kKey);
					if(pu32Iter)
					{
						VE_ASSERT((*pu32Iter) < spEntity->m_kDataArray.Size());
						VeBaseServer::PushLuaData(it->m_tSecond.m_tFirst, *spEntity->m_kDataArray[*pu32Iter]);
						VeLuaBind::Resume(it->m_tSecond.m_tFirst, it->m_tSecond.m_tSecond, g_pLua->GetLua(), 1);
					}
					else
					{
						printf("Load entity [%s:\"%s\"] failed\n", (const VeChar8*)(spEntity->m_kName), (const VeChar8*)kKey);
						VeLuaBind::Resume(it->m_tSecond.m_tFirst, it->m_tSecond.m_tSecond, g_pLua->GetLua(), 0);
					}
				}
			}
			m_kLuaCallMap.Erase(it);
		}
		break;
	default:
		printf("Unknown event %d\n", u8Event);
		break;
	}
}
//--------------------------------------------------------------------------
void VeDatabaseLink::LoadEntity(VeUInt16 u16Entity, LuaHolder kData)
{
	VeUInt32 u32CallID(0);
	if(m_kLuaCallMap.Size())
	{
		u32CallID = m_kLuaCallMap.Last()->m_tFirst + 1;
	}
	m_kLuaCallMap[u32CallID] = kData;
	BitStream kStream;
	kStream.Reset();
	kStream.Write(VeUInt8(ID_USER_PACKET_ENUM + REQ_LOAD_ENTITY + 1));
	kStream.Write(u32CallID);
	kStream.Write(u16Entity);
	SendPacket(kStream);
}
//--------------------------------------------------------------------------
void VeDatabaseLink::LoadEntity(VeUInt16 u16Entity, const VeChar8* pcKey,
	LuaHolder kData)
{
	VeUInt32 u32CallID(0);
	if(m_kLuaCallMap.Size())
	{
		u32CallID = m_kLuaCallMap.Last()->m_tFirst + 1;
	}
	m_kLuaCallMap[u32CallID] = kData;
	BitStream kStream;
	kStream.Reset();
	kStream.Write(VeUInt8(ID_USER_PACKET_ENUM + REQ_LOAD_ENTITY + 1));
	kStream.Write(u32CallID);
	kStream.Write(u16Entity);
	Write(kStream, pcKey);
	SendPacket(kStream);
}
//--------------------------------------------------------------------------
void VeDatabaseLink::LoadEntity(VeUInt16 u16Entity, VeInt64 i64Key,
	LuaHolder kData)
{
	VeUInt32 u32CallID(0);
	if(m_kLuaCallMap.Size())
	{
		u32CallID = m_kLuaCallMap.Last()->m_tFirst + 1;
	}
	m_kLuaCallMap[u32CallID] = kData;
	BitStream kStream;
	kStream.Reset();
	kStream.Write(VeUInt8(ID_USER_PACKET_ENUM + REQ_LOAD_ENTITY + 1));
	kStream.Write(u32CallID);
	kStream.Write(u16Entity);
	kStream.Write(i64Key);
	SendPacket(kStream);
}
//--------------------------------------------------------------------------
void VeDatabaseLink::SyncEntity(VeNetEntity::DataEntity& kData)
{
	VeNetEntity& kEntity = *(VeNetEntity*)kData.m_pkObject;
	BitStream kStream;
	kStream.Reset();
	kStream.Write(VeUInt8(ID_USER_PACKET_ENUM + REQ_UPDATE_ENTITY + 1));
	VE_ASSERT(kEntity.m_u32Index < VE_UINT16_MAX);
	kStream.Write(VeUInt16(kEntity.m_u32Index));
	if(kEntity.m_eType != VeNetEntity::TYPE_GLOBAL)
	{
		VE_ASSERT(kEntity.m_kProperties.Front()->GetType() == VeNetEntity::Property::TYPE_VALUE);
		VeNetEntity::Value& kValue = (VeNetEntity::Value&)*kEntity.m_kProperties.Front();
		if(kValue.m_eType == VeNetEntity::VALUE_VeString)
		{
			VeNetEntity::DataValue& kDataValue = (VeNetEntity::DataValue&)*kData.m_kDataArray.Front();
			Write(kStream, (const VeChar8*)kDataValue.m_pbyBuffer + 2, *(VeUInt16*)kDataValue.m_pbyBuffer);
		}
	}
	VeNetEntity::WriteUpdate(kStream, kData, VeNetEntity::Property::FLAG_DATABASE);
	SendPacket(kStream);
}
//--------------------------------------------------------------------------
VeBaseServer::VeBaseServer(const InitData& kData)
	: VeServerBase(kData.m_kSuperData), m_f64TimeTick(0)
	, m_bTimeTickStarted(false), m_bLuaLoaded(false)
{
	m_spDatabaseLink = VE_NEW VeDatabaseLink(this);
	if(g_pClientManager->AddClient(m_spDatabaseLink))
	{
		if(!g_pClientManager->ConnectSync(m_spDatabaseLink, kData.m_kHost, kData.m_u16Port, kData.m_kPassword))
		{
			m_spDatabaseLink = NULL;
		}
	}
	else
	{
		m_spDatabaseLink = NULL;
	}

	VeInitDelegate(VeBaseServer, OnLuaLoad);
	g_pLua->Require(m_kPath[1] + "/*", &VeDelegate(OnLuaLoad));
}
//--------------------------------------------------------------------------
VeBaseServer::~VeBaseServer()
{

}
//--------------------------------------------------------------------------
VeClientAgentPtr VeBaseServer::CreateClientAgent(
	const RakNet::SystemAddress& kAddress)
{
	return VE_NEW VeBaseClient(this, kAddress);
}
//--------------------------------------------------------------------------
void VeBaseServer::OnStart()
{
	AddToBanList("*");
}
//--------------------------------------------------------------------------
void VeBaseServer::OnEnd()
{
	if(m_spDatabaseLink)
	{
		g_pClientManager->Disconnect(m_spDatabaseLink);
		m_spDatabaseLink->m_pkParent = NULL;
		m_spDatabaseLink = NULL;
	}
}
//--------------------------------------------------------------------------
void VeBaseServer::OnUpdate()
{
	if(m_bTimeTickStarted)
	{
		m_f64TimeTick += g_pTime->GetDeltaTime64() * 100.0;
		VeLuaCall<void>("VeNet.OnBaseUpdate");
	}
}
//--------------------------------------------------------------------------
void VeBaseServer::_Start()
{
	if(m_bLuaLoaded && m_spDatabaseLink && m_spDatabaseLink->IsEnable())
	{
		m_spEntityMgr = VE_NEW VeEntityManagerBase();
		VeDirectory* pkDir = g_pResourceManager->CreateDir(m_kPath[0]);
		if(pkDir)
		{
			VeVector<VeFixedString> kList;
			pkDir->FindFileList("*.xml", kList);
			for(VeVector<VeFixedString>::iterator it = kList.Begin();
				it != kList.End(); ++it)
			{
				VeXMLDocument kDoc;
				(*pkDir->OpenSync(*it)) >> kDoc;
				m_spEntityMgr->LoadConfig(kDoc.RootElement());
				printf("%s loaded\n", *it);
			}
			g_pResourceManager->DestoryDir(pkDir);
		}

		VeLuaCall<void>("VeNet.OnBaseStart", this);
	}
}
//--------------------------------------------------------------------------
void VeBaseServer::LoadEntity(lua_State* pkState, const VeChar8* pcName)
{
	VeNetEntityPtr spEntity = m_spEntityMgr->GetEntity(pcName);
	if(spEntity)
	{
		if(spEntity->m_eType == VeNetEntity::TYPE_GLOBAL)
		{
			if(spEntity->m_kDataArray.Size())
			{
				VeBaseServer::PushLuaData(pkState, *spEntity->m_kDataArray.Front());
				VeLuaBind::ManualReturn(1);
			}
			else
			{
				VE_ASSERT(m_spDatabaseLink && spEntity->m_u32Index < VE_UINT16_MAX);
				VeInt32 i32Ref = VeLuaBind::Pause(pkState);
				m_spDatabaseLink->LoadEntity(VeUInt16(spEntity->m_u32Index), VeMakePair(pkState, i32Ref));
			}
		}
		else
		{
			printf("LoadEntity: entity type is not global\n");
		}
	}
	else
	{
		printf("LoadEntity: entity not found\n");
	}
}
//--------------------------------------------------------------------------
void VeBaseServer::LoadEntity(lua_State* pkState, const VeChar8* pcName,
	const VeChar8* pcKey)
{
	VeNetEntityPtr spEntity = m_spEntityMgr->GetEntity(pcName);
	if(spEntity)
	{
		if(spEntity->m_eType != VeNetEntity::TYPE_GLOBAL)
		{
			VE_ASSERT(spEntity->m_kProperties.Front()->GetType() == VeNetEntity::Property::TYPE_VALUE);
			VeNetEntity::Value& kValue = (VeNetEntity::Value&)*spEntity->m_kProperties.Front();
			if(kValue.m_spKeyMap && kValue.m_spKeyMap->GetType() == VeNetEntity::KeyMap::TYPE_STRING)
			{
				VeStringMap<VeUInt32>& kMap = ((VeNetEntity::StrMap&)*kValue.m_spKeyMap).m_kMap;
				VeUInt32* pu32Iter = kMap.Find(pcKey);
				if(pu32Iter)
				{
					VE_ASSERT((*pu32Iter) < spEntity->m_kDataArray.Size());
					VeBaseServer::PushLuaData(pkState, *spEntity->m_kDataArray[*pu32Iter]);
					VeLuaBind::ManualReturn(1);
				}
				else
				{
					VE_ASSERT(m_spDatabaseLink && spEntity->m_u32Index < VE_UINT16_MAX);
					VeInt32 i32Ref = VeLuaBind::Pause(pkState);
					m_spDatabaseLink->LoadEntity(VeUInt16(spEntity->m_u32Index), pcKey, VeMakePair(pkState, i32Ref));
				}
			}
			else
			{
				printf("LoadEntityByStr: entity has not a string key\n");
			}
		}
		else
		{
			printf("LoadEntityByStr: entity type is global\n");
		}
	}
	else
	{
		printf("LoadEntityByStr: entity not found\n");
	}
}
//--------------------------------------------------------------------------
void VeBaseServer::LoadEntity(lua_State* pkState, const VeChar8* pcName,
	VeInt64 i64Key)
{
	VeNetEntityPtr spEntity = m_spEntityMgr->GetEntity(pcName);
	if(spEntity)
	{
		if(spEntity->m_eType != VeNetEntity::TYPE_GLOBAL)
		{
			VE_ASSERT(spEntity->m_kProperties.Front()->GetType() == VeNetEntity::Property::TYPE_VALUE);
			VeNetEntity::Value& kValue = (VeNetEntity::Value&)*spEntity->m_kProperties.Front();
			if(kValue.m_spKeyMap && kValue.m_spKeyMap->GetType() == VeNetEntity::KeyMap::TYPE_INTEGER)
			{
				VeHashMap<VeInt64,VeUInt32>& kMap = ((VeNetEntity::IntMap&)*kValue.m_spKeyMap).m_kMap;
				VeUInt32* pu32Iter = kMap.Find(i64Key);
				if(pu32Iter)
				{
					VE_ASSERT((*pu32Iter) < spEntity->m_kDataArray.Size());
					VeBaseServer::PushLuaData(pkState, *spEntity->m_kDataArray[*pu32Iter]);
					VeLuaBind::ManualReturn(1);
				}
				else
				{
					VE_ASSERT(m_spDatabaseLink && spEntity->m_u32Index < VE_UINT16_MAX);
					VeInt32 i32Ref = VeLuaBind::Pause(pkState);
					m_spDatabaseLink->LoadEntity(VeUInt16(spEntity->m_u32Index), i64Key, VeMakePair(pkState, i32Ref));
				}
			}
			else
			{
				printf("LoadEntityByInt: entity has not a integer key\n");
			}
		}
		else
		{
			printf("LoadEntityByInt: entity type is global\n");
		}
	}
	else
	{
		printf("LoadEntityByInt: entity not found\n");
	}
}
//--------------------------------------------------------------------------
void VeBaseServer::ReadyForConnections()
{
	ClearBanList();
	printf("%s Server Started, Waiting For Connections.\n", GetName());
	VeVector<VeString> kStrArray;
	GetSocketsAddress(kStrArray);
	for(VeUInt32 i(0); i < kStrArray.Size(); ++i)
	{
		printf("Ports Used By: %d. %s\n", i+1, kStrArray[i].GetString());
	}
	GetServiceIP(kStrArray);
	for(VeUInt32 i(0); i < kStrArray.Size(); ++i)
	{
		printf("\nService IP: \n%i. %s\n", i+1, kStrArray[i].GetString());
	}
	printf("%s Server GUID is %u\n", (const VeChar8*)m_kName, GetGuidFromAddress());
}
//--------------------------------------------------------------------------
void VeBaseServer::SyncEntities()
{
	m_spEntityMgr->m_kDirtyCache.m_kDirtyList.BeginIterator();
	while(!m_spEntityMgr->m_kDirtyCache.m_kDirtyList.IsEnd())
	{
		VeNetEntity::Data* pkData = m_spEntityMgr->m_kDirtyCache.m_kDirtyList.GetIterNode()->m_tContent;
		m_spEntityMgr->m_kDirtyCache.m_kDirtyList.Next();
		VE_ASSERT(pkData && pkData->GetType() == VeNetEntity::Data::TYPE_ENTITY);
		SyncEntity(*(VeNetEntity::DataEntity*)pkData);
	}
	VE_ASSERT(m_spEntityMgr->m_kDirtyCache.m_kDirtyList.Empty());
}
//--------------------------------------------------------------------------
void VeBaseServer::SyncEntity(VeNetEntity::DataEntity& kData)
{
	m_spDatabaseLink->SyncEntity(kData);
	VeNetEntity& kEntity = *(VeNetEntity*)kData.m_pkObject;
	if(!kData.m_kNode.IsAttach(kEntity.m_kFreeList))
	{
		VeBaseClient& kClient = VeRefCast(&VeBaseClient::m_kPropertyList, *kData.m_kNode.GetRefList());
		BitStream kStream;
		kStream.Reset();
		kStream.Write(VeUInt8(ID_USER_PACKET_ENUM + VeClient::NOTI_ENT_UPDATED + 1));
		VE_ASSERT(kEntity.m_u32Index < VE_UINT16_MAX);
		kStream.Write(VeUInt16(kEntity.m_u32Index));
		VeNetEntity::WriteUpdate(kStream, kData, VeNetEntity::Property::FLAG_CLIENT);
		kClient.SendPacket(kStream); 
	}
	VeNetEntity::ClearDirty(kData);
}
//--------------------------------------------------------------------------
void VeBaseServer::SetTimeTick(VeFloat64 f64Time)
{
	m_f64TimeTick = f64Time;
	m_bTimeTickStarted = true;
}
//--------------------------------------------------------------------------
VeFloat64 VeBaseServer::GetTimeTick()
{
	return m_f64TimeTick;
}
//--------------------------------------------------------------------------
VeImplRunLuaDelegate(VeBaseServer, OnLuaLoad, pcFile, bEnd)
{
	if(bEnd)
	{
		m_bLuaLoaded = true;
		_Start();
	}
	else
	{
		printf("%s loaded\n", pcFile);
	}
}
//--------------------------------------------------------------------------
VeImplementLuaExport(VeBaseServer)
{
	Module() [
		Class<VeBaseServer>().
			Def("GetName", &VeBaseServer::GetName).
			Def("GetPort", &VeBaseServer::GetPort).
			Def("GetMaxConnections", &VeBaseServer::GetMaxConnections).
			Def("GetTimeOut", &VeBaseServer::GetTimeOut).
			Def("AddToBanList", &VeBaseServer::AddToBanList).
			Def("RemoveFromBanList", &VeBaseServer::RemoveFromBanList).
			Def("ClearBanList", &VeBaseServer::ClearBanList).
			Def("IsBanned", &VeBaseServer::IsBanned).
			Def("LoadEntity", (void (VeBaseServer::*)(lua_State*,const VeChar8*))&VeBaseServer::LoadEntity).
			Def("LoadEntity", (void (VeBaseServer::*)(lua_State*,const VeChar8*,const VeChar8*))&VeBaseServer::LoadEntity).
			Def("ReadyForConnections", &VeBaseServer::ReadyForConnections).
			Def("SyncEntities", (void (VeBaseServer::*)())&VeBaseServer::SyncEntities).
			Def("SetTimeTick", &VeBaseServer::SetTimeTick).
			Def("GetTimeTick", &VeBaseServer::GetTimeTick)
	];
}
//--------------------------------------------------------------------------
namespace VeLuaBind
{
	VeLuaClassEnumFuncImpl(VeBaseServer, ArrayFunc);
}
//--------------------------------------------------------------------------
VeLuaClassEnumImpl(VeBaseServer, ArrayFunc)
{
	VeLuaClassEnumBase(VeBaseServer, ArrayFunc);
	VeLuaClassEnumSlot(VeBaseServer, ArrayFunc, ARRAY_SIZE, "size");
	VeLuaClassEnumSlot(VeBaseServer, ArrayFunc, ARRAY_FRONT, "front");
	VeLuaClassEnumSlot(VeBaseServer, ArrayFunc, ARRAY_BACK, "back");
	VeLuaClassEnumSlot(VeBaseServer, ArrayFunc, ARRAY_PUSHBACK, "push_back");
	VeLuaClassEnumSlot(VeBaseServer, ArrayFunc, ARRAY_POPBACK, "pop_back");
	VeLuaClassEnumSlot(VeBaseServer, ArrayFunc, ARRAY_DELETE, "delete");
	VeLuaClassEnumSlot(VeBaseServer, ArrayFunc, ARRAY_CLEAR, "clear");
}
//--------------------------------------------------------------------------
