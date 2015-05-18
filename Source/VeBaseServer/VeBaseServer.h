////////////////////////////////////////////////////////////////////////////
//
//  Venus Server Header File.
//  Copyright (C), Venus Interactive Entertainment.2012
// -------------------------------------------------------------------------
//  File name:   VeBaseServer.h
//  Version:     v1.00
//  Created:     4/11/2014 by Napoleon
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//  http://www.venusie.com
////////////////////////////////////////////////////////////////////////////

#pragma once

class VeBaseServer;

class VeDatabaseLink : public VeClientBase
{
public:
	typedef VePair<lua_State*,VeInt32> LuaHolder;

	enum Request
	{
		REQ_LOAD_ENTITY,
		REQ_UPDATE_ENTITY,
		REQ_MAX
	};

	enum Response
	{
		RES_LOAD_ENTITY_SUCCEEDED,
		RES_LOAD_ENTITY_FAILED,
		RES_MAX
	};

	VeDatabaseLink(VeBaseServer* pkParent);

	virtual ~VeDatabaseLink();

	bool IsEnable();

	virtual void OnConnect();

	virtual void OnConnectFailed(DisconnectType eType);

	virtual void OnDisconnect(DisconnectType eType);

	virtual void ProcessUserEvent(VeUInt8 u8Event, RakNet::BitStream& kStream);

	void LoadEntity(VeUInt16 u16Entity, LuaHolder kData);

	void LoadEntity(VeUInt16 u16Entity, const VeChar8* pcKey, LuaHolder kData);

	void LoadEntity(VeUInt16 u16Entity, VeInt64 i64Key, LuaHolder kData);

	void SyncEntity(VeNetEntity::DataEntity& kData);

protected:
	friend class VeBaseServer;
	VeBaseServer* m_pkParent;
	VeMap<VeUInt32,LuaHolder> m_kLuaCallMap;
	bool m_bEnable;

};

VeSmartPointer(VeDatabaseLink);

class VeBaseServer : public VeServerBase
{
	VeDeclareLuaRttiExport(VeBaseServer);
public:
	enum ArrayFunc
	{
		ARRAY_SIZE,
		ARRAY_FRONT,
		ARRAY_BACK,
		ARRAY_PUSHBACK,
		ARRAY_POPBACK,
		ARRAY_DELETE,
		ARRAY_CLEAR,
		ARRAY_MAX
	};
	VeLuaClassEnumDef(ArrayFunc);

	struct InitData
	{
		VeServerBase::InitData m_kSuperData;
		VeString m_kHost;
		VeUInt16 m_u16Port;
		VeUInt16 m_u16Align;
		VeString m_kPassword;
	};

	VeBaseServer(const InitData& kData);

	virtual ~VeBaseServer();

	virtual VeClientAgentPtr CreateClientAgent(const RakNet::SystemAddress& kAddress);

	virtual void OnStart();

	virtual void OnEnd();

	virtual void OnUpdate();

	void LoadEntity(lua_State* pkState, const VeChar8* pcName);

	void LoadEntity(lua_State* pkState, const VeChar8* pcName, const VeChar8* pcKey);

	void LoadEntity(lua_State* pkState, const VeChar8* pcName, VeInt64 i64Key);

	void ReadyForConnections();

	void SyncEntities();

	void SetTimeTick(VeFloat64 f64Time);

	VeFloat64 GetTimeTick();

	static void CreateLuaData(lua_State* pkState, VeNetEntity::DataEntity& kData);

	static void PushLuaData(lua_State* pkState, VeNetEntity::DataEntity& kData);

protected:
	friend class VeDatabaseLink;
	friend class VeBaseClient;

	void _Start();

	void SyncEntity(VeNetEntity::DataEntity& kData);
	
	VeDatabaseLinkPtr m_spDatabaseLink;
	VeEntityManagerBasePtr m_spEntityMgr;
	VeDeclRunLuaDelegate(VeBaseServer, OnLuaLoad);
	VeFloat64 m_f64TimeTick;
	bool m_bLuaLoaded;
	bool m_bTimeTickStarted;

};

namespace VeLuaBind
{
	VeLuaClassEnumFuncDef(, VeBaseServer, ArrayFunc);
}

VeSmartPointer(VeBaseServer);
