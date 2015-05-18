////////////////////////////////////////////////////////////////////////////
//
//  Venus Server Source File.
//  Copyright (C), Venus Interactive Entertainment.2012
// -------------------------------------------------------------------------
//  File name:   VeDatabaseServer.cpp
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
VeDatabaseClient::VeDatabaseClient(VeServerBase* pkParent,
	const RakNet::SystemAddress& kAddress)
	: VeClientAgent(pkParent, kAddress)
{

}
//--------------------------------------------------------------------------
VeDatabaseClient::~VeDatabaseClient()
{

}
//--------------------------------------------------------------------------
VeDatabaseServer* VeDatabaseClient::GetParent()
{
	return (VeDatabaseServer*)m_pkParent;
}
//--------------------------------------------------------------------------
void VeDatabaseClient::OnConnect()
{
	printf("OnConnect %s\n", GetAddress());
}
//--------------------------------------------------------------------------
void VeDatabaseClient::OnDisconnect()
{
	m_kEntityList.BeginIterator();
	while(!m_kEntityList.IsEnd())
	{
		VeNetEntity::DataEntity* pkData = m_kEntityList.GetIterNode()->m_tContent;
		m_kEntityList.Next();
		VeNetEntity& kEntity = (VeNetEntity&)*(pkData->m_pkObject);
		kEntity.m_kFreeList.AttachBack(pkData->m_kNode);
	}
	printf("OnDisconnect %s\n", GetAddress());
}
//--------------------------------------------------------------------------
void VeDatabaseClient::ProcessUserEvent(VeUInt8 u8Event,
	RakNet::BitStream& kStream)
{
	switch(u8Event)
	{
	case ID_USER_PACKET_ENUM + REQ_LOAD_ENTITY + 1:
		LoadEntity(kStream);
		break;
	case ID_USER_PACKET_ENUM + REQ_UPDATE_ENTITY + 1:
		UpdateEntity(kStream);
		break;
	default:
		printf("Unknown event %d\n", u8Event);
		break;
	}
}
//--------------------------------------------------------------------------
void VeDatabaseClient::LoadEntity(RakNet::BitStream& kStream)
{
	VeUInt32 u32Call;
	if(!kStream.Read(u32Call)) return;
	VeUInt16 u16Entity;
	if(!kStream.Read(u16Entity)) return;
	VeNetEntityPtr spEntity = GetParent()->m_spEntityMgr->GetEntity(u16Entity);
	if(spEntity)
	{
		VeNetEntity::DataEntityPtr spData;
		if(spEntity->m_eType == VeNetEntity::TYPE_GLOBAL)
		{
			spData = GetParent()->m_spEntityMgr->LoadEntityData(*spEntity);
			if(!spData)
			{
				BitStream kStream;
				kStream.Reset();
				kStream.Write(VeUInt8(ID_USER_PACKET_ENUM + RES_LOAD_ENTITY_FAILED + 1));
				kStream.Write(u32Call);
				kStream.Write(u16Entity);
				SendPacket(kStream);
				return;
			}
		}
		else
		{
			VE_ASSERT(spEntity->m_kProperties.Front()->GetType() == VeNetEntity::Property::TYPE_VALUE);
			VeNetEntity::Value& kValue = (VeNetEntity::Value&)*spEntity->m_kProperties.Front();
			VE_ASSERT(kValue.m_spKeyMap);
			switch(kValue.m_spKeyMap->GetType())
			{
			case VeNetEntity::KeyMap::TYPE_STRING:
				{
					VeString kKey;
					if(!Read(kStream, kKey)) return;
					spData = GetParent()->m_spEntityMgr->LoadEntityData(*spEntity, kKey);
					if(!spData)
					{
						BitStream kStream;
						kStream.Reset();
						kStream.Write(VeUInt8(ID_USER_PACKET_ENUM + RES_LOAD_ENTITY_FAILED + 1));
						kStream.Write(u32Call);
						kStream.Write(u16Entity);
						Write(kStream, kKey);
						SendPacket(kStream);
						return;
					}
				}
				break;
			case VeNetEntity::KeyMap::TYPE_INTEGER:
				{
					VeInt64 i64Key;
					if(!kStream.Read(i64Key)) return;
					//spData = GetParent()->m_spEntityMgr->LoadEntityData(*spEntity, i64Key);
					if(!spData)
					{
						BitStream kStream;
						kStream.Reset();
						kStream.Write(VeUInt8(ID_USER_PACKET_ENUM + RES_LOAD_ENTITY_FAILED + 1));
						kStream.Write(u32Call);
						kStream.Write(u16Entity);
						kStream.Write(i64Key);
						SendPacket(kStream);
						return;
					}
				}
				break;
			default:
				break;
			}
		}
		if(spData)
		{
			m_kEntityList.AttachBack(spData->m_kNode);
			BitStream kStream;
			kStream.Reset();
			kStream.Write(VeUInt8(ID_USER_PACKET_ENUM + RES_LOAD_ENTITY_SUCCEEDED + 1));
			kStream.Write(u32Call);
			VeNetEntity::Write(kStream, *spData);
			SendPacket(kStream);
		}
	}
}
//--------------------------------------------------------------------------
void VeDatabaseClient::UpdateEntity(BitStream& kStream)
{
	VeUInt16 u16Entity;
	if(!kStream.Read(u16Entity)) return;
	VeNetEntityPtr spEntity = GetParent()->m_spEntityMgr->GetEntity(u16Entity);
	if(spEntity)
	{
		VeNetEntity::DataEntityPtr spData;
		if(spEntity->m_eType == VeNetEntity::TYPE_GLOBAL)
		{
			spData = GetParent()->m_spEntityMgr->GetEntityData(*spEntity);
		}
		else
		{
			VE_ASSERT(spEntity->m_kProperties.Front()->GetType() == VeNetEntity::Property::TYPE_VALUE);
			VeNetEntity::Value& kValue = (VeNetEntity::Value&)*spEntity->m_kProperties.Front();
			VE_ASSERT(kValue.m_spKeyMap);
			if(kValue.m_spKeyMap->GetType() == VeNetEntity::KeyMap::TYPE_STRING)
			{
				VeString kKey;
				if(!Read(kStream, kKey)) return;
				spData = GetParent()->m_spEntityMgr->GetEntityData(*spEntity, kKey);
			}
		}
		if(spData && spData->m_kNode.IsAttach(m_kEntityList))
		{
			VeNetEntity::ReadUpdate(kStream, *spData);
		}
		else
		{
			printf("Error to update entity with wrong connection\n");
		}
	}
}
//--------------------------------------------------------------------------
VeDatabaseServer::VeDatabaseServer(const InitData& kData)
	: VeServerBase(kData.m_kSuperData)
{
#	ifdef VE_ENABLE_MYSQL
	m_spEntityMgr = VE_NEW VeEntityManagerMySQL();
#	endif

	VE_ASSERT(m_spEntityMgr);

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

	while(!m_spEntityMgr->ConnectDatabase(kData.m_kHost, kData.m_kUser,
		kData.m_kPassword, kData.m_kDatabase, kData.m_u32Port))
	{
		printf("Connect database %s@%s failed.\n", (const VeChar8*)(kData.m_kHost), (const VeChar8*)(kData.m_kUser));
	}
	printf("Connect database %s@%s successful.\n", (const VeChar8*)(kData.m_kHost), (const VeChar8*)(kData.m_kUser));

	m_spEntityMgr->InitDatabase();
}
//--------------------------------------------------------------------------
VeDatabaseServer::~VeDatabaseServer()
{

}
//--------------------------------------------------------------------------
VeClientAgentPtr VeDatabaseServer::CreateClientAgent(
	const RakNet::SystemAddress& kAddress)
{
	return VE_NEW VeDatabaseClient(this, kAddress);
}
//--------------------------------------------------------------------------
void VeDatabaseServer::OnStart()
{
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
void VeDatabaseServer::OnEnd()
{

}
//--------------------------------------------------------------------------
