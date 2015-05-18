////////////////////////////////////////////////////////////////////////////
//
//  Venus Server Header File.
//  Copyright (C), Venus Interactive Entertainment.2012
// -------------------------------------------------------------------------
//  File name:   VeDatabaseServer.h
//  Version:     v1.00
//  Created:     4/11/2014 by Napoleon
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//  http://www.venusie.com
////////////////////////////////////////////////////////////////////////////

#pragma once

class VeDatabaseServer;

class VeDatabaseClient : public VeClientAgent
{
public:
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

	VeDatabaseClient(VeServerBase* pkParent, const RakNet::SystemAddress& kAddress);

	virtual ~VeDatabaseClient();

	VeDatabaseServer* GetParent();

	virtual void OnConnect();

	virtual void OnDisconnect();

	virtual void ProcessUserEvent(VeUInt8 u8Event, RakNet::BitStream& kStream);

	void LoadEntity(RakNet::BitStream& kStream);

	void UpdateEntity(RakNet::BitStream& kStream);

protected:
	VeRefList<VeNetEntity::DataEntity*> m_kEntityList;

};

class VeDatabaseServer : public VeServerBase
{
public:
	struct InitData
	{
		VeServerBase::InitData m_kSuperData;
		VeString m_kHost;
		VeString m_kUser;
		VeString m_kPassword;
		VeString m_kDatabase;
		VeUInt32 m_u32Port;
	};

	struct PropertyDesc
	{
		PropertyDesc() : m_bArray(false) {}

		PropertyDesc& operator = (const PropertyDesc& kDesc)
		{
			m_kName = kDesc.m_kName;
			m_kType = kDesc.m_kType;
			m_bArray = kDesc.m_bArray;
			return *this;
		}

		VeString m_kName;
		VeString m_kType;
		bool m_bArray;
	};

	VeDatabaseServer(const InitData& kData);

	virtual ~VeDatabaseServer();

	virtual VeClientAgentPtr CreateClientAgent(const RakNet::SystemAddress& kAddress);

	virtual void OnStart();

	virtual void OnEnd();

protected:
	friend class VeDatabaseClient;
	VeEntityManagerDatabasePtr m_spEntityMgr;

};

VeSmartPointer(VeDatabaseServer);
