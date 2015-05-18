////////////////////////////////////////////////////////////////////////////
//
//  Venus Server Source File.
//  Copyright (C), Venus Interactive Entertainment.2012
// -------------------------------------------------------------------------
//  File name:   VeBaseServerLua.cpp
//  Version:     v1.00
//  Created:     17/11/2014 by Napoleon
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
void VeBaseServer::CreateLuaData(lua_State* pkState,
	VeNetEntity::DataEntity& kEntity)
{
	if(!kEntity.m_i32Ref)
	{
		VeNetEntity::RetParam(pkState, kEntity, true);
		kEntity.m_i32Ref = luaL_ref(pkState, LUA_REGISTRYINDEX);
	}
}
//--------------------------------------------------------------------------
void VeBaseServer::PushLuaData(lua_State* pkState,
	VeNetEntity::DataEntity& kEntity)
{
	CreateLuaData(pkState, kEntity);
	lua_rawgeti(pkState, LUA_REGISTRYINDEX, kEntity.m_i32Ref);
}
//--------------------------------------------------------------------------
class LocalFunctionCaller : public VeBackgroundTask
{
public:
	LocalFunctionCaller(lua_State* pkThread, VeInt32 i32Ref, VeUInt32 u32Num)
		: m_pkThread(pkThread), m_i32Ref(i32Ref), m_u32ParamsNum(u32Num) {}
	
	virtual void DoMainThreadTask(VeBackgroundTaskQueue& kMgr)
	{
		VeLuaBind::CallFuncThread(g_pLua->GetLua(), m_pkThread, m_u32ParamsNum);
		luaL_unref(g_pLua->GetLua(), LUA_REGISTRYINDEX, m_i32Ref);
	}

	lua_State* m_pkThread;
	VeInt32 m_i32Ref;
	VeUInt32 m_u32ParamsNum;

};
//--------------------------------------------------------------------------
VeInt32 VeBaseClient::CallLocalFunction(lua_State* pkState)
{
	CallHolder& kHolder = *(CallHolder*)lua_touserdata(pkState, lua_upvalueindex(1));
	if(kHolder.m_kNode.IsAttach())
	{
		VeInt32 i32Top = lua_gettop(pkState);
		if(VeUInt32(i32Top) == kHolder.m_kRpcFunc.m_kInParams.Size())
		{
			if(kHolder.m_kRpcFunc.m_kOutParams.Size())
			{
				lua_rawgeti(g_pLua->GetLua(), LUA_REGISTRYINDEX, g_pLua->GetFullCall());
				VE_ASSERT(lua_type(g_pLua->GetLua(), -1) == LUA_TFUNCTION);
				lua_State* pkThread = lua_newthread(g_pLua->GetLua());
				VeInt32 i32Ref = luaL_ref(g_pLua->GetLua(), LUA_REGISTRYINDEX);
				lua_xmove(g_pLua->GetLua(), pkThread, 1);
				VE_ASSERT(lua_gettop(pkThread) == 1);
				VeLuaBind::StackHolder kHoldMain(g_pLua->GetLua());
				VeLuaBind::StackHolder kHoldThread(pkThread);
				lua_rawgeti(pkThread, LUA_REGISTRYINDEX, kHolder.m_kRpcFunc.m_i32FuncRef);
				lua_pushthread(pkState);
				lua_xmove(pkState, pkThread, 1);
				lua_pushcclosure(pkThread, &OnCallLocalFinished, 1);
				VE_ASSERT(kHolder.m_kData.m_tSecond);
				lua_rawgeti(pkThread, LUA_REGISTRYINDEX, kHolder.m_kData.m_tSecond);
				for(VeUInt32 i(0); i < VeUInt32(i32Top); ++i)
				{
					lua_pushvalue(pkState, i + 1);
					lua_xmove(pkState, pkThread, 1);
				}
				g_pResourceManager->AddFGTask(VeResourceManager::BG_QUEUE_PROCESS, VE_NEW LocalFunctionCaller(pkThread, i32Ref, 3 + i32Top));
				return lua_yield(pkState, 0);
			}
			else
			{
				lua_State* pkThread = VeLuaBind::MakeFuncThread(g_pLua->GetLua(), kHolder.m_kRpcFunc.m_i32FuncRef);
				VeLuaBind::StackHolder kHoldMain(g_pLua->GetLua());
				VeLuaBind::StackHolder kHoldThread(pkThread);
				VE_ASSERT(kHolder.m_kData.m_tSecond);
				lua_rawgeti(pkThread, LUA_REGISTRYINDEX, kHolder.m_kData.m_tSecond);
				for(VeUInt32 i(0); i < VeUInt32(i32Top); ++i)
				{
					lua_pushvalue(pkState, i + 1);
					lua_xmove(pkState, pkThread, 1);
				}
				VeLuaBind::CallFuncThread(g_pLua->GetLua(), pkThread, 1 + i32Top);
				return 0;
			}
		}
		return VeNetEntity::RetParam(pkState, kHolder.m_kRpcFunc.m_kOutParams);
	}
	return 0;
}
//--------------------------------------------------------------------------
VeInt32 VeBaseClient::OnCallLocalFinished(lua_State* pkState)
{
	lua_State* pkThread = lua_tothread(pkState, lua_upvalueindex(1));
	VeInt32 i32Top = lua_gettop(pkState);
	for(VeUInt32 i(0); i < VeUInt32(i32Top); ++i)
	{
		lua_pushvalue(pkState, i + 1);
		lua_xmove(pkState, pkThread, 1);
	}
	lua_resume(pkThread, g_pLua->GetLua(), 0);
	return 0;
}
//--------------------------------------------------------------------------
VeInt32 VeBaseClient::CallClientFunction(lua_State* pkState)
{
	CallHolder& kHolder = *(CallHolder*)lua_touserdata(pkState, lua_upvalueindex(1));
	if(kHolder.m_kNode.IsAttach())
	{
		VE_ASSERT(kHolder.m_kData.m_tFirst && kHolder.m_kNode.m_tContent);
		VeNetEntity& kEnt = *(VeNetEntity*)kHolder.m_kData.m_tFirst->m_pkObject;
		VeBaseClient& kClient = *kHolder.m_kNode.m_tContent;
		VeUInt32 u32CallID(0);
		if(kHolder.m_kRpcFunc.m_kOutParams.Size() && kClient.m_kLuaCallMap.Size())
		{
			u32CallID = kClient.m_kLuaCallMap.Last()->m_tFirst + 1;
		}
		VE_ASSERT(u32CallID < 0xffff);
		BitStream kStream;
		kStream.Reset();
		kStream.Write(VeUInt8(ID_USER_PACKET_ENUM + VeClient::REQ_CALL_FUNC + 1));
		kStream.Write(VeUInt16(u32CallID));
		kStream.Write(VeUInt16(kEnt.m_u32Index));
		kStream.Write(VeUInt16(kHolder.m_kRpcFunc.m_u32Index));
		if(VeNetEntity::Write(kStream, pkState, kHolder.m_kRpcFunc.m_kInParams))
		{
			kClient.SendPacket(kStream);
			if(kHolder.m_kRpcFunc.m_kOutParams.Size())
			{
				VE_ASSERT(kClient.m_kLuaCallMap.Find(u32CallID) == kClient.m_kLuaCallMap.End());
				VeClient::FuncCall& kCall = kClient.m_kLuaCallMap[u32CallID];
				kCall.m_kHolder.m_tFirst = pkState;
				lua_pushthread(pkState);
				VE_ASSERT(lua_tothread(pkState, -1) == pkState);
				kCall.m_kHolder.m_tSecond = luaL_ref(pkState, LUA_REGISTRYINDEX);
				kCall.m_pkFunc = &kHolder.m_kRpcFunc;
				return lua_yield(pkState, 0);
			}
			return 0;
		}
		return VeNetEntity::RetParam(pkState, kHolder.m_kRpcFunc.m_kOutParams);
	}
	return 0;
}
//--------------------------------------------------------------------------
VeInt32 VeBaseClient::LinkEntity(lua_State* pkState)
{
	VeRefNode<VeBaseClient*>& kNode = *(VeRefNode<VeBaseClient*>*)lua_touserdata(pkState, lua_upvalueindex(1));
	if(kNode.IsAttach() && lua_type(pkState, -1) == LUA_TUSERDATA)
	{
		VeNetEntity::LuaSupport* pkSupport = *(VeNetEntity::LuaSupport**)lua_touserdata(pkState, -1);
		if(pkSupport->m_pkParent && pkSupport->m_pkParent->GetType() == VeNetEntity::Data::TYPE_ENTITY)
		{
			VeNetEntity::DataEntity& kData = *(VeNetEntity::DataEntity*)pkSupport->m_pkParent;
			VeNetEntity& kEntity = *(VeNetEntity*)kData.m_pkObject;
			VE_ASSERT(kEntity.m_u32Index < kNode.m_tContent->m_kEntArray.Size());
			if(!kNode.m_tContent->m_kEntArray[kEntity.m_u32Index].m_tFirst)
			{
				if(kEntity.m_eType == VeNetEntity::TYPE_PROPERTY)
				{
					if(kData.m_kNode.IsAttach(kEntity.m_kFreeList))
					{
						kNode.m_tContent->CreateLuaData(pkState, kData);
						kNode.m_tContent->m_kPropertyList.AttachBack(kData.m_kNode);
						BitStream kStream;
						kStream.Reset();
						kStream.Write(VeUInt8(ID_USER_PACKET_ENUM + VeClient::NOTI_ENT_LINKED + 1));
						kStream.Write(VeUInt16(kEntity.m_u32Index));
						VeNetEntity::Write(kStream, kData, VeNetEntity::Property::FLAG_CLIENT);
						kNode.m_tContent->SendPacket(kStream);
						lua_rawgeti(pkState, LUA_REGISTRYINDEX, kNode.m_tContent->m_kEntArray[kEntity.m_u32Index].m_tSecond);
						return 1;
					}
				}
			}
		}
	}
	return 0;
}
//--------------------------------------------------------------------------
void VeBaseClient::CreateLuaData(lua_State* pkState,
	VeNetEntity::DataEntity& kData)
{
	VeNetEntity& kEnt = *(VeNetEntity*)kData.m_pkObject;
	VE_ASSERT(kEnt.m_u32Index < m_kEntArray.Size());
	if(!m_kEntArray[kEnt.m_u32Index].m_tFirst)
	{
		VE_ASSERT(!m_kEntArray[kEnt.m_u32Index].m_tSecond);
		m_kEntArray[kEnt.m_u32Index].m_tFirst = &kData;

		lua_newtable(pkState);
		lua_newtable(pkState);
		lua_pushstring(pkState, "__newindex");
		lua_pushcclosure(pkState, &VeNetEntity::NoWriteNewIndex, 0);
		lua_rawset(pkState, -3);
		lua_pushstring(pkState, "__index");
		lua_newtable(pkState);
		
		lua_pushstring(pkState, "prop");
		VeBaseServer::PushLuaData(pkState, kData);
		lua_rawset(pkState, -3);

		lua_pushstring(pkState, "user");
		lua_newtable(pkState);
		lua_rawset(pkState, -3);

		lua_pushstring(pkState, "agent");
		lua_newtable(pkState);
		lua_newtable(pkState);

		lua_pushstring(pkState, "__newindex");
		lua_pushcclosure(pkState, &VeNetEntity::NoWriteNewIndex, 0);
		lua_rawset(pkState, -3);
		lua_pushstring(pkState, "__index");
		lua_newtable(pkState);

		lua_pushstring(pkState, "LinkEntity");
		{
			VeRefNode<VeBaseClient*>& kNode = *new (lua_newuserdata(pkState, sizeof(VeRefNode<VeBaseClient*>))) VeRefNode<VeBaseClient*>();
			kNode.m_tContent = this;
			m_kCallHolderList.AttachBack(kNode);
		}
		lua_pushcclosure(pkState, &VeBaseClient::LinkEntity, 1);
		lua_rawset(pkState, -3);

		lua_rawset(pkState, -3);

		lua_setmetatable(pkState, -2);
		lua_rawset(pkState, -3);

		lua_pushstring(pkState, "base");
		lua_newtable(pkState);
		lua_newtable(pkState);
		lua_pushstring(pkState, "__newindex");
		lua_pushcclosure(pkState, &VeNetEntity::NoWriteNewIndex, 0);
		lua_rawset(pkState, -3);
		lua_pushstring(pkState, "__index");
		lua_newtable(pkState);
		for(VeUInt32 i(0); i < kEnt.m_kBaseFuncArray.Size(); ++i)
		{
			lua_pushstring(pkState, kEnt.m_kBaseFuncArray[i].m_kName);
			new(lua_newuserdata(pkState, sizeof(CallHolder))) CallHolder(*this, m_kEntArray[kEnt.m_u32Index], kEnt.m_kBaseFuncArray[i]);
			lua_pushcclosure(pkState, &VeBaseClient::CallLocalFunction, 1);
			lua_rawset(pkState, -3);
		}
		lua_rawset(pkState, -3);
		lua_setmetatable(pkState, -2);
		lua_rawset(pkState, -3);

		lua_pushstring(pkState, "client");
		lua_newtable(pkState);
		lua_newtable(pkState);
		lua_pushstring(pkState, "__newindex");
		lua_pushcclosure(pkState, &VeNetEntity::NoWriteNewIndex, 0);
		lua_rawset(pkState, -3);
		lua_pushstring(pkState, "__index");
		lua_newtable(pkState);
		for(VeUInt32 i(0); i < kEnt.m_kClientFuncArray.Size(); ++i)
		{
			lua_pushstring(pkState, kEnt.m_kClientFuncArray[i].m_kName);
			new(lua_newuserdata(pkState, sizeof(CallHolder))) CallHolder(*this, m_kEntArray[kEnt.m_u32Index], kEnt.m_kClientFuncArray[i]);
			lua_pushcclosure(pkState, &VeBaseClient::CallClientFunction, 1);
			lua_rawset(pkState, -3);
		}
		lua_rawset(pkState, -3);
		lua_setmetatable(pkState, -2);
		lua_rawset(pkState, -3);

		lua_rawset(pkState, -3);
		lua_setmetatable(pkState, -2);

		m_kEntArray[kEnt.m_u32Index].m_tSecond = luaL_ref(pkState, LUA_REGISTRYINDEX);
	}
}
//--------------------------------------------------------------------------
void VeBaseClient::ReleaseLuaDatas(lua_State* pkState)
{
	for(VeUInt32 i(0); i < m_kEntArray.Size(); ++i)
	{
		if(m_kEntArray[i].m_tFirst)
		{
			if(m_kEntArray[i].m_tFirst->m_kNode.IsAttach(m_kPropertyList))
			{
				VeNetEntity& kEntity = *(VeNetEntity*)m_kEntArray[i].m_tFirst->m_pkObject;
				kEntity.m_kFreeList.AttachBack(m_kEntArray[i].m_tFirst->m_kNode);
			}
			VE_ASSERT(m_kEntArray[i].m_tSecond);
			luaL_unref(pkState, LUA_REGISTRYINDEX, m_kEntArray[i].m_tSecond);
			m_kEntArray[i].m_tFirst = NULL;
			m_kEntArray[i].m_tSecond = 0;
		}
	}
	m_kEntArray.Clear();
	VE_ASSERT(m_kPropertyList.Empty());
}
//--------------------------------------------------------------------------
