////////////////////////////////////////////////////////////////////////////
//
//  Venus Server Header File.
//  Copyright (C), Venus Interactive Entertainment.2012
// -------------------------------------------------------------------------
//  File name:   VeDatabaseEntity.h
//  Version:     v1.00
//  Created:     10/11/2014 by Napoleon
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//  http://www.venusie.com
////////////////////////////////////////////////////////////////////////////

#pragma once

class VeEntityManagerDatabase : public VeNetEntityManager
{
public:
	VeEntityManagerDatabase();

	virtual ~VeEntityManagerDatabase();

	virtual bool ConnectDatabase(const VeChar8* pcHost, const VeChar8* pcUser,
		const VeChar8* pcPasswd, const VeChar8* pcDatabase, VeUInt32 u32Port) = 0;

	virtual void InitDatabase() = 0;

	virtual VeNetEntity::DataEntityPtr GetEntityData(VeNetEntity& kEntity) = 0;

	virtual VeNetEntity::DataEntityPtr GetEntityData(VeNetEntity& kEntity, VeString& kKey) = 0;

	virtual VeNetEntity::DataEntityPtr LoadEntityData(VeNetEntity& kEntity) = 0;

	virtual VeNetEntity::DataEntityPtr LoadEntityData(VeNetEntity& kEntity, VeString& kKey) = 0;
	
};

VeSmartPointer(VeEntityManagerDatabase);

#ifdef VE_ENABLE_MYSQL

struct st_mysql;

class VeEntityManagerMySQL : public VeEntityManagerDatabase
{
public:
	struct EntityHolder : public VeNetEntity::DataHolder
	{
		EntityHolder();

		virtual ~EntityHolder();

		virtual Type GetType() const;

		virtual void NotifyDirty(VeRefNode<VeNetEntity::Data*>& kNode);

		VeEntityManagerMySQL* m_pkEntityMgr;
	};

	VeEntityManagerMySQL();

	virtual ~VeEntityManagerMySQL();

	virtual bool ConnectDatabase(const VeChar8* pcHost, const VeChar8* pcUser,
		const VeChar8* pcPasswd, const VeChar8* pcDatabase, VeUInt32 u32Port);

	virtual void InitDatabase();

	virtual VeNetEntity::DataEntityPtr GetEntityData(VeNetEntity& kEntity);

	virtual VeNetEntity::DataEntityPtr GetEntityData(VeNetEntity& kEntity, VeString& kKey);

	virtual VeNetEntity::DataEntityPtr LoadEntityData(VeNetEntity& kEntity);

	virtual VeNetEntity::DataEntityPtr LoadEntityData(VeNetEntity& kEntity, VeString& kKey);

protected:
	void InitEntity(VeNetEntity& kEntity);

	VeSizeT ColumProperty(VeChar8* pcDesc, VeNetEntity& kEntity, VeNetEntity::Property& kProperty, VeUInt32& u32Count, VeUInt32 u32Table);

	VeSizeT ColumProperty(VeChar8* pcDesc, VeNetEntity& kEntity, VeNetEntity::Property& kProperty, VeStringMap<VeUInt32>& kColumMap, VeUInt32 u32Table);

	VeSizeT ColumValue(VeChar8* pcDesc, VeNetEntity::Value& kValue, VeUInt32& u32Count, VeUInt32 u32Table);

	VeSizeT ColumValue(VeChar8* pcDesc, VeNetEntity::Value& kValue);

	VeSizeT ColumValue(VeChar8* pcDesc, VeNetEntity::Value& kValue, VeStringMap<VeUInt32>& kColumMap, VeUInt32 u32Table);

	VeSizeT ColumEnum(VeChar8* pcDesc, VeNetEntity::Enum& kEnum, VeUInt32& u32Count, VeUInt32 u32Table);

	VeSizeT ColumEnum(VeChar8* pcDesc, VeNetEntity::Enum& kEnum, VeStringMap<VeUInt32>& kColumMap, VeUInt32 u32Table);

	VeSizeT ColumStruct(VeChar8* pcDesc, VeNetEntity& kEntity, VeNetEntity::Struct& kStruct, VeUInt32& u32Count, VeUInt32 u32Table);

	VeSizeT ColumStruct(VeChar8* pcDesc, VeNetEntity& kEntity, VeNetEntity::Struct& kStruct, VeStringMap<VeUInt32>& kColumMap, VeUInt32 u32Table);

	void ColumArray(VeNetEntity& kEntity, VeNetEntity::Array& kArray, VeUInt32 u32Table);

	bool HasTable(const VeChar8* pcName, VeUInt32 u32Length);

	st_mysql* m_pkMySQL;
	EntityHolder m_kHolder;

};

VeSmartPointer(VeEntityManagerMySQL);

#endif
