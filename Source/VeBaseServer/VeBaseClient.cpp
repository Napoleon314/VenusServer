////////////////////////////////////////////////////////////////////////////
//
//  Venus Server Source File.
//  Copyright (C), Venus Interactive Entertainment.2012
// -------------------------------------------------------------------------
//  File name:   VeBaseClientAgent.cpp
//  Version:     v1.00
//  Created:     4/11/2014 by Napoleon
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//  http://www.venusie.com
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

extern "C"
{
#	include <lua.h>
#	include <lauxlib.h>
#	include <lualib.h>
}

#include <RakPeerInterface.h>
#include <RakNetTypes.h>
#include <MessageIdentifiers.h>
#include <BitStream.h>

using namespace RakNet;

//--------------------------------------------------------------------------
VeBaseClient::VeBaseClient(VeServerBase* pkParent,
	const RakNet::SystemAddress& kAddress)
	: VeClientAgent(pkParent, kAddress)
{

}
//--------------------------------------------------------------------------
VeBaseClient::~VeBaseClient()
{
	
}
//--------------------------------------------------------------------------
VeBaseServer* VeBaseClient::GetParent()
{
	return (VeBaseServer*)m_pkParent;
}
//--------------------------------------------------------------------------
void VeBaseClient::OnConnect()
{
	m_kEntArray.Resize(GetParent()->m_spEntityMgr->GetEntityNum());
	printf("OnConnect %s\n", GetAddress());
}
//--------------------------------------------------------------------------
void VeBaseClient::OnDisconnect()
{
	m_kCallHolderList.Clear();
	ReleaseLuaDatas(g_pLua->GetLua());
	printf("OnDisconnect %s\n", GetAddress());
}
//--------------------------------------------------------------------------
void VeBaseClient::ProcessUserEvent(VeUInt8 u8Event, RakNet::BitStream& kStream)
{
	switch(u8Event)
	{
	case ID_USER_PACKET_ENUM + VeClient::REQ_CALL_FUNC + 1:
		OnCallFunc(kStream);
		break;
	case ID_USER_PACKET_ENUM + VeClient::RES_CALL_FUNC + 1:
		OnResCallFunc(kStream);
		break;
	default:
		printf("Unknown event %d\n", u8Event);
		break;
	}
}
//--------------------------------------------------------------------------
void VeBaseClient::OnCallFunc(RakNet::BitStream& kStream)
{
	VeUInt16 u16Call;
	if(!kStream.Read(u16Call)) return;
	VeUInt16 u16Ent;
	if(!kStream.Read(u16Ent)) return;
	VeUInt16 u16Func;
	if(!kStream.Read(u16Func)) return;
	VeNetEntityPtr spEntity = GetParent()->m_spEntityMgr->GetEntity(u16Ent);
	if(spEntity && u16Func < spEntity->m_kBaseFuncArray.Size())
	{
		VeNetEntity::DataEntity* pkData(NULL);
		if(spEntity->m_eType == VeNetEntity::TYPE_GLOBAL)
		{
			pkData = spEntity->m_kDataArray.Front();
			VE_ASSERT(u16Ent < m_kEntArray.Size());
			CreateLuaData(g_pLua->GetLua(), *pkData);
		}
		else
		{
			if(u16Ent < m_kEntArray.Size())
			{
				pkData = m_kEntArray[u16Ent].m_tFirst;
			}
		}
		if(pkData)
		{
			VeNetEntity::RpcFunc& kFunc = spEntity->m_kBaseFuncArray[u16Func];
			lua_State* pkThread = g_pLua->MakeFullCallThread();
			VE_ASSERT(pkThread);
			VeLuaBind::StackHolder kHoldMain(g_pLua->GetLua());
			VeLuaBind::StackHolder kHoldThread(pkThread);
			lua_rawgeti(pkThread, LUA_REGISTRYINDEX, kFunc.m_i32FuncRef);
			new(lua_newuserdata(pkThread, sizeof(CalledHolder))) CalledHolder(*this, *pkData, kFunc, u16Call);
			lua_pushcclosure(pkThread, &OnCallFinished, 1);
			VE_ASSERT(m_kEntArray[u16Ent].m_tSecond);
			lua_rawgeti(pkThread, LUA_REGISTRYINDEX, m_kEntArray[u16Ent].m_tSecond);
			VeInt32 i32ParamNum = VeNetEntity::RetParam(pkThread, kStream, kFunc.m_kInParams);
			if(i32ParamNum == kFunc.m_kInParams.Size())
			{
				VeLuaBind::CallFuncThread(g_pLua->GetLua(), pkThread, 3 + i32ParamNum);
			}
		}
	}
}
//--------------------------------------------------------------------------
VeInt32 VeBaseClient::OnCallFinished(lua_State* pkState)
{
	CalledHolder& kHolder = *(CalledHolder*)lua_touserdata(pkState, lua_upvalueindex(1));
	if(kHolder.m_kNode.IsAttach())
	{
		kHolder.m_kNode.m_tContent->GetParent()->SyncEntities();
		if(kHolder.m_kRpcFunc.m_kOutParams.Size())
		{
			BitStream kStream;
			kStream.Reset();
			kStream.Write(VeUInt8(ID_USER_PACKET_ENUM + VeClient::RES_CALL_FUNC + 1));
			kStream.Write(kHolder.m_u16Call);
			if(VeNetEntity::Write(kStream, pkState, kHolder.m_kRpcFunc.m_kOutParams))
			{
				kHolder.m_kNode.m_tContent->SendPacket(kStream);
			}
			else
			{
				VeNetEntity& kEntity = *(VeNetEntity*)kHolder.m_kData.m_pkObject;
				printf("Error return in %s.%s", (const VeChar8*)kEntity.m_kName,
					(const VeChar8*)kHolder.m_kRpcFunc.m_kName);
			}
		}
		kHolder.m_kNode.Detach();
	}
	return 0;
}
//--------------------------------------------------------------------------
void VeBaseClient::OnResCallFunc(RakNet::BitStream& kStream)
{
	VeUInt16 u16Call;
	if(!kStream.Read(u16Call)) return;
	VeMap<VeUInt32,VeClient::FuncCall>::iterator it = m_kLuaCallMap.Find(u16Call);
	if(it == m_kLuaCallMap.End()) return;
	VE_ASSERT(it->m_tSecond.m_pkFunc);
	VeClient::FuncCall kCall(it->m_tSecond);
	m_kLuaCallMap.Erase(it);
	VeInt32 i32Top = lua_gettop(kCall.m_kHolder.m_tFirst);
	VeInt32 i32ParamNum = VeNetEntity::RetParam(kCall.m_kHolder.m_tFirst,
		kStream, kCall.m_pkFunc->m_kOutParams);
	if(VeUInt32(i32ParamNum) != kCall.m_pkFunc->m_kOutParams.Size())
	{
		lua_settop(kCall.m_kHolder.m_tFirst, i32Top);
		VeNetEntity::RetParam(kCall.m_kHolder.m_tFirst, kCall.m_pkFunc->m_kOutParams);		
	}
	VeLuaBind::Resume(kCall.m_kHolder.m_tFirst, kCall.m_kHolder.m_tSecond,
		g_pLua->GetLua(), i32ParamNum);
}
//--------------------------------------------------------------------------
