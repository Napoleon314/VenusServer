////////////////////////////////////////////////////////////////////////////
//
//  Venus Server Header File.
//  Copyright (C), Venus Interactive Entertainment.2012
// -------------------------------------------------------------------------
//  File name:   VeBaseClientAgent.h
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

class VeBaseClient : public VeClientAgent
{
public:
	struct CalledHolder
	{
		CalledHolder(VeBaseClient& kClient, VeNetEntity::DataEntity& kData,
			VeNetEntity::RpcFunc& kFunc, VeUInt16 u16Call)
			: m_kData(kData), m_kRpcFunc(kFunc), m_u16Call(u16Call)
		{
			m_kNode.m_tContent = &kClient;
			kClient.m_kCallHolderList.AttachBack(m_kNode);
		}

		VeRefNode<VeBaseClient*> m_kNode;
		VeNetEntity::DataEntity& m_kData;
		VeNetEntity::RpcFunc& m_kRpcFunc;
		VeUInt16 m_u16Call;
	};

	struct CallHolder
	{
		CallHolder(VeBaseClient& kClient,
			VePair<VeNetEntity::DataEntityPtr,VeInt32>& kData,
			VeNetEntity::RpcFunc& kFunc) : m_kData(kData), m_kRpcFunc(kFunc)
		{
			m_kNode.m_tContent = &kClient;
			kClient.m_kCallHolderList.AttachBack(m_kNode);
		}

		VeRefNode<VeBaseClient*> m_kNode;
		VePair<VeNetEntity::DataEntityPtr,VeInt32>& m_kData;
		VeNetEntity::RpcFunc& m_kRpcFunc;
	};

	VeBaseClient(VeServerBase* pkParent, const RakNet::SystemAddress& kAddress);

	virtual ~VeBaseClient();

	VeBaseServer* GetParent();

	virtual void OnConnect();

	virtual void OnDisconnect();

	virtual void ProcessUserEvent(VeUInt8 u8Event, RakNet::BitStream& kStream);

protected:
	friend class LocalFunctionCaller;
	friend class VeBaseServer;

	void CreateLuaData(lua_State* pkState, VeNetEntity::DataEntity& kData);

	void ReleaseLuaDatas(lua_State* pkState);

	void OnCallFunc(RakNet::BitStream& kStream);

	void OnResCallFunc(RakNet::BitStream& kStream);

	static VeInt32 LinkEntity(lua_State* pkState);

	static VeInt32 OnCallFinished(lua_State* pkState);

	static VeInt32 CallLocalFunction(lua_State* pkState);

	static VeInt32 OnCallLocalFinished(lua_State* pkState);

	static VeInt32 CallClientFunction(lua_State* pkState);
	
	VeRefList<VeNetEntity::DataEntity*> m_kPropertyList;
	VeVector< VePair<VeNetEntity::DataEntityPtr,VeInt32> > m_kEntArray;
	VeRefList<VeBaseClient*> m_kCallHolderList;
	VeMap<VeUInt32,VeClient::FuncCall> m_kLuaCallMap;

};
