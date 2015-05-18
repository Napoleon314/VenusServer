////////////////////////////////////////////////////////////////////////////
//
//  Venus Server Source File.
//  Copyright (C), Venus Interactive Entertainment.2012
// -------------------------------------------------------------------------
//  File name:   VeDatabaseEntity.cpp
//  Version:     v1.00
//  Created:     10/11/2014 by Napoleon
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//  http://www.venusie.com
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

//--------------------------------------------------------------------------
VeEntityManagerDatabase::VeEntityManagerDatabase()
	: VeNetEntityManager(TYPE_DATABASE)
{

}
//--------------------------------------------------------------------------
VeEntityManagerDatabase::~VeEntityManagerDatabase()
{

}
//--------------------------------------------------------------------------
#ifdef VE_ENABLE_MYSQL
//--------------------------------------------------------------------------
#include <my_global.h>
#include <mysql.h>
//--------------------------------------------------------------------------
#define VE_QUERY(con,query) \
	if(mysql_query(con, query)) \
	{ \
		printf("Error %u: %s\n%s\n", mysql_errno(con), mysql_error(con), query); \
	} \
	else \
	{ \
		printf("Succeed: %s\n", query); \
	}
//--------------------------------------------------------------------------
static const VeChar8* s_apcSQLValueType[VeNetEntity::VALUE_VeString] =
{
	"int unsigned",
	"tinyint unsigned",
	"smallint unsigned",
	"int unsigned",
	"bigint unsigned",
	"tynyint",
	"smallint",
	"int",
	"bigint",
	"float",
	"double"
};
//--------------------------------------------------------------------------
static const VeChar8* s_apcStoreType[] =
{
	"innodb",
	"myisam",
	"memory"
};
//--------------------------------------------------------------------------
VeEntityManagerMySQL::VeEntityManagerMySQL() : m_pkMySQL(NULL)
{
	m_kHolder.m_pkEntityMgr = this;
}
//--------------------------------------------------------------------------
VeEntityManagerMySQL::~VeEntityManagerMySQL()
{
	if(m_pkMySQL)
	{
		mysql_close(m_pkMySQL);
		m_pkMySQL = NULL;
	}
}
//--------------------------------------------------------------------------
bool VeEntityManagerMySQL::ConnectDatabase(const VeChar8* pcHost,
	const VeChar8* pcUser, const VeChar8* pcPasswd,
	const VeChar8* pcDatabase, VeUInt32 u32Port)
{
	m_pkMySQL = mysql_init(NULL);
	if(m_pkMySQL)
	{
		if(mysql_real_connect(m_pkMySQL, pcHost, pcUser, pcPasswd, pcDatabase, u32Port, NULL, 0) == m_pkMySQL)
		{
			return true;
		}
		else
		{
			mysql_close(m_pkMySQL);
			m_pkMySQL = NULL;
		}
	}
	return false;
}
//--------------------------------------------------------------------------
void VeEntityManagerMySQL::InitDatabase()
{
	for(VeVector<VeNetEntityPtr>::iterator it = m_kEntityArray.Begin();
		it != m_kEntityArray.End(); ++it)
	{
		InitEntity(**it);
	}
}
//--------------------------------------------------------------------------
void VeEntityManagerMySQL::InitEntity(VeNetEntity& kEntity)
{
	VE_ASSERT(kEntity.m_kProperties.Size()
		&& kEntity.m_kProperties.Front()->GetType() == VeNetEntity::Property::TYPE_VALUE);
	VeChar8 acBuffer[2048];
	kEntity.m_kTableArray.Clear();
	kEntity.m_kTableArray.Increase();
	kEntity.m_kTableArray.Back().m_kName = kEntity.m_kName + "_db";
	kEntity.m_kTableArray.Back().m_u32Reference = 0;
	kEntity.m_kTableArray.Back().m_bNeedToLink = false;
	bool bExist = HasTable(kEntity.m_kTableArray.Back().m_kName, kEntity.m_kTableArray.Back().m_kName.Length());
	if(bExist)
	{
		VeStringMap<VeUInt32> kColumMap;
		{
			VeChar8* pcPointer(acBuffer);
			pcPointer += VeSprintf(pcPointer, 1024, "show columns from ");
			pcPointer += mysql_real_escape_string(m_pkMySQL, pcPointer, kEntity.m_kTableArray.Back().m_kName, kEntity.m_kTableArray.Back().m_kName.Length());
			VE_QUERY(m_pkMySQL, acBuffer);
			MYSQL_RES* pkRes = mysql_store_result(m_pkMySQL);
			VeUInt32 i(0);
			for(MYSQL_ROW row = mysql_fetch_row(pkRes); row; row =  mysql_fetch_row(pkRes))
			{
				kColumMap[row[0]] = i++;
			}
			mysql_free_result(pkRes);
		}
		VeUInt32 u32PreColumNum = kColumMap.Size();
		VeChar8* pcPointer(acBuffer);
		pcPointer += VeSprintf(pcPointer, 1024, "alter table ");
		pcPointer += mysql_real_escape_string(m_pkMySQL, pcPointer, kEntity.m_kTableArray.Back().m_kName, kEntity.m_kTableArray.Back().m_kName.Length());
		pcPointer += VeSprintf(pcPointer, 1024, " add (");
		for(VeUInt32 i(0); i < kEntity.m_kProperties.Size(); ++i)
		{
			if(kEntity.m_kProperties[i]->HasFlag(VeNetEntity::Property::FLAG_DATABASE))
			{
				pcPointer += ColumProperty(pcPointer, kEntity, *(kEntity.m_kProperties[i]), kColumMap, 0);
			}
		}
		if(u32PreColumNum < kColumMap.Size())
		{
			*(pcPointer-1) = ')';
			VE_QUERY(m_pkMySQL, acBuffer);
		}
	}
	else
	{
		VeChar8* pcPointer(acBuffer);
		pcPointer += VeSprintf(pcPointer, 1024, "create table ");
		pcPointer += mysql_real_escape_string(m_pkMySQL, pcPointer, kEntity.m_kTableArray.Back().m_kName, kEntity.m_kTableArray.Back().m_kName.Length());
		pcPointer += VeSprintf(pcPointer, 1024, " (");
		VeUInt32 u32Count(0);
		for(VeUInt32 i(0); i < kEntity.m_kProperties.Size(); ++i)
		{
			if(kEntity.m_kProperties[i]->HasFlag(VeNetEntity::Property::FLAG_DATABASE))
			{
				pcPointer += ColumProperty(pcPointer, kEntity, *(kEntity.m_kProperties[i]), u32Count, 0);
			}
		}
		pcPointer += VeSprintf(pcPointer, 1024, "primary key(");
		pcPointer += mysql_real_escape_string(m_pkMySQL, pcPointer, kEntity.m_kProperties.Front()->m_kName, kEntity.m_kProperties.Front()->m_kName.Length());
		pcPointer += VeSprintf(pcPointer, 1024, "_db)");
		pcPointer += VeSprintf(pcPointer, 1024, ")engine=%s charset=utf8", s_apcStoreType[kEntity.m_eStore]);
		VE_QUERY(m_pkMySQL, acBuffer);
	}
	for(VeUInt32 i(1); i < kEntity.m_kTableArray.Size(); ++i)
	{
		if(!(kEntity.m_kTableArray[i].m_bNeedToLink)) continue;
		kEntity.m_kTableArray[i].m_bNeedToLink = false;
		VeString& kTable = kEntity.m_kTableArray[i].m_kName;
		VeString& kRefTable = kEntity.m_kTableArray[kEntity.m_kTableArray[i].m_u32Reference].m_kName;
		VeChar8* pcPointer(acBuffer);
		pcPointer += VeSprintf(pcPointer, 1024, "alter table ");
		pcPointer += mysql_real_escape_string(m_pkMySQL, pcPointer, kTable, kTable.Length());
		if(kEntity.m_kTableArray[i].m_u32Reference)
		{
			pcPointer += VeSprintf(pcPointer, 1024, " add foreign key(foreign_id) references ");
			pcPointer += mysql_real_escape_string(m_pkMySQL, pcPointer, kRefTable, kRefTable.Length());
			pcPointer += VeSprintf(pcPointer, 1024, "(");
			pcPointer += mysql_real_escape_string(m_pkMySQL, pcPointer, kRefTable, kRefTable.Length());
			pcPointer += VeSprintf(pcPointer, 1024, "_id) on delete cascade");
		}
		else
		{
			pcPointer += VeSprintf(pcPointer, 1024, " add foreign key(");
			pcPointer += mysql_real_escape_string(m_pkMySQL, pcPointer, kEntity.m_kProperties.Front()->m_kName, kEntity.m_kProperties.Front()->m_kName.Length());
			pcPointer += VeSprintf(pcPointer, 1024, "_db) references ");
			pcPointer += mysql_real_escape_string(m_pkMySQL, pcPointer, kRefTable, kRefTable.Length());
			pcPointer += VeSprintf(pcPointer, 1024, "(");
			pcPointer += mysql_real_escape_string(m_pkMySQL, pcPointer, kEntity.m_kProperties.Front()->m_kName, kEntity.m_kProperties.Front()->m_kName.Length());
			pcPointer += VeSprintf(pcPointer, 1024, "_db) on delete cascade");
		}
		VE_QUERY(m_pkMySQL, acBuffer);
	}
}
//--------------------------------------------------------------------------
VeSizeT VeEntityManagerMySQL::ColumProperty(VeChar8* pcDesc,
	VeNetEntity& kEntity, VeNetEntity::Property& kProperty,
	VeUInt32& u32Count, VeUInt32 u32Table)
{
	VeChar8* pcPointer = pcDesc;
	switch(kProperty.GetType())
	{
	case VeNetEntity::Property::TYPE_VALUE:
		pcPointer += ColumValue(pcPointer, (VeNetEntity::Value&)kProperty, u32Count, u32Table);
		break;
	case VeNetEntity::Property::TYPE_ENUM:
		pcPointer += ColumEnum(pcPointer, (VeNetEntity::Enum&)kProperty, u32Count, u32Table);
		break;
	case VeNetEntity::Property::TYPE_STRUCT:
		pcPointer += ColumStruct(pcPointer, kEntity, (VeNetEntity::Struct&)kProperty, u32Count, u32Table);
		break;
	case VeNetEntity::Property::TYPE_ARRAY:
		ColumArray(kEntity, (VeNetEntity::Array&)kProperty, u32Table);
		break;
	default:
		break;
	}
	return pcPointer - pcDesc;
}
//--------------------------------------------------------------------------
VeSizeT VeEntityManagerMySQL::ColumProperty(VeChar8* pcDesc,
	VeNetEntity& kEntity, VeNetEntity::Property& kProperty,
	VeStringMap<VeUInt32>& kColumMap, VeUInt32 u32Table)
{
	VeChar8* pcPointer = pcDesc;
	switch(kProperty.GetType())
	{
		case VeNetEntity::Property::TYPE_VALUE:
			pcPointer += ColumValue(pcPointer, (VeNetEntity::Value&)kProperty, kColumMap, u32Table);
			break;
		case VeNetEntity::Property::TYPE_ENUM:
			pcPointer += ColumEnum(pcPointer, (VeNetEntity::Enum&)kProperty, kColumMap, u32Table);
			break;
		case VeNetEntity::Property::TYPE_STRUCT:
			pcPointer += ColumStruct(pcPointer, kEntity, (VeNetEntity::Struct&)kProperty, kColumMap, u32Table);
		break;
	case VeNetEntity::Property::TYPE_ARRAY:
		ColumArray(kEntity, (VeNetEntity::Array&)kProperty, u32Table);
		break;
	default:
		break;
	}
	return pcPointer - pcDesc;
}
//--------------------------------------------------------------------------
VeSizeT VeEntityManagerMySQL::ColumValue(VeChar8* pcDesc,
	VeNetEntity::Value& kValue, VeUInt32& u32Count, VeUInt32 u32Table)
{
	kValue.m_kIndex.m_tFirst = u32Table;
	kValue.m_kIndex.m_tSecond = u32Count++;
	VeChar8* pcPointer = pcDesc;
	if(kValue.m_kField.Length())
	{
		pcPointer += mysql_real_escape_string(m_pkMySQL, pcPointer, kValue.m_kField, kValue.m_kField.Length());
		pcPointer += VeSprintf(pcPointer, 1024, "_");
	}
	pcPointer += mysql_real_escape_string(m_pkMySQL, pcPointer, kValue.m_kName, kValue.m_kName.Length());
	if(kValue.m_eType < VeNetEntity::VALUE_VeString)
	{
		pcPointer += VeSprintf(pcPointer, 1024, "_db %s", s_apcSQLValueType[kValue.m_eType]);
	}
	else if(kValue.m_eType == VeNetEntity::VALUE_VeString)
	{
		if((*(VeUInt16*)kValue.m_pbyBuffer) <= 255)
		{
			pcPointer += VeSprintf(pcPointer, 1024, "_db varchar(%d)", (*(VeUInt16*)kValue.m_pbyBuffer));
		}
		else
		{
			pcPointer += VeSprintf(pcPointer, 1024, "_db text");
		}
	}
	pcPointer += VeSprintf(pcPointer, 1024, " not null");
	if(kValue.m_eType == VeNetEntity::VALUE_AUTO)
	{
		pcPointer += VeSprintf(pcPointer, 1024, " auto_increment");
	}
	else if(kValue.m_eType < VeNetEntity::VALUE_VeInt8)
	{
		pcPointer += VeSprintf(pcPointer, 1024, " default %llu", kValue.m_u64Default);
	}
	else if(kValue.m_eType < VeNetEntity::VALUE_VeFloat32)
	{
		pcPointer += VeSprintf(pcPointer, 1024, " default %ll", kValue.m_i64Default);
	}
	else if(kValue.m_eType == VeNetEntity::VALUE_VeString)
	{
		pcPointer += VeSprintf(pcPointer, 1024, " default \'");
		const VeChar8* pcDefault = (const VeChar8*)(kValue.m_pbyBuffer + 2);
		pcPointer += mysql_real_escape_string(m_pkMySQL, pcPointer, pcDefault, (VeUInt32)VeStrlen(pcDefault));
		pcPointer += VeSprintf(pcPointer, 1024, "\'");
	}
	else
	{
		pcPointer += VeSprintf(pcPointer, 1024, " default %f", kValue.m_f64Default);
	}
	pcPointer += VeSprintf(pcPointer, 1024, ",");
	return pcPointer - pcDesc;
}
//--------------------------------------------------------------------------
VeSizeT VeEntityManagerMySQL::ColumValue(VeChar8* pcDesc,
	VeNetEntity::Value& kValue)
{
	VeChar8* pcPointer = pcDesc;
	if(kValue.m_kField.Length())
	{
		pcPointer += mysql_real_escape_string(m_pkMySQL, pcPointer, kValue.m_kField, kValue.m_kField.Length());
		pcPointer += VeSprintf(pcPointer, 1024, "_");
	}
	pcPointer += mysql_real_escape_string(m_pkMySQL, pcPointer, kValue.m_kName, kValue.m_kName.Length());
	if(kValue.m_eType < VeNetEntity::VALUE_VeString)
	{
		pcPointer += VeSprintf(pcPointer, 1024, "_db %s", s_apcSQLValueType[kValue.m_eType]);
	}
	else if(kValue.m_eType == VeNetEntity::VALUE_VeString)
	{
		if((*(VeUInt16*)kValue.m_pbyBuffer) <= 255)
		{
			pcPointer += VeSprintf(pcPointer, 1024, "_db varchar(%d)", (*(VeUInt16*)kValue.m_pbyBuffer));
		}
		else
		{
			pcPointer += VeSprintf(pcPointer, 1024, "_db text");
		}
	}
	pcPointer += VeSprintf(pcPointer, 1024, " not null,");
	return pcPointer - pcDesc;
}
//--------------------------------------------------------------------------
VeSizeT VeEntityManagerMySQL::ColumValue(VeChar8* pcDesc,
	VeNetEntity::Value& kValue, VeStringMap<VeUInt32>& kColumMap,
	VeUInt32 u32Table)
{
	kValue.m_kIndex.m_tFirst = u32Table;
	VeString kName;
	if(kValue.m_kField.Length())
	{
		kName += kValue.m_kField;
		kName += "_";
	}
	kName += kValue.m_kName;
	kName += "_db";
	VeChar8* pcPointer = pcDesc;
	VeUInt32* pu32Iter = kColumMap.Find(kName);
	if(!pu32Iter)
	{
		pu32Iter = &kColumMap[kName];
		*pu32Iter = kColumMap.Size() - 1;
		pcPointer += mysql_real_escape_string(m_pkMySQL, pcPointer, kName, kName.Length());
		if(kValue.m_eType < VeNetEntity::VALUE_VeString)
		{
			pcPointer += VeSprintf(pcPointer, 1024, " %s", s_apcSQLValueType[kValue.m_eType]);
		}
		else if(kValue.m_eType == VeNetEntity::VALUE_VeString)
		{
			if((*(VeUInt16*)kValue.m_pbyBuffer) <= 255)
			{
				pcPointer += VeSprintf(pcPointer, 1024, " varchar(%d)", (*(VeUInt16*)kValue.m_pbyBuffer));
			}
			else
			{
				pcPointer += VeSprintf(pcPointer, 1024, " text");
			}
		}
		pcPointer += VeSprintf(pcPointer, 1024, " not null");
		if(kValue.m_eType == VeNetEntity::VALUE_AUTO)
		{
			pcPointer += VeSprintf(pcPointer, 1024, " auto_increment");
		}
		else if(kValue.m_eType < VeNetEntity::VALUE_VeInt8)
		{
			pcPointer += VeSprintf(pcPointer, 1024, " default %llu", kValue.m_u64Default);
		}
		else if(kValue.m_eType < VeNetEntity::VALUE_VeFloat32)
		{
			pcPointer += VeSprintf(pcPointer, 1024, " default %ll", kValue.m_i64Default);
		}
		else if(kValue.m_eType == VeNetEntity::VALUE_VeString)
		{
			pcPointer += VeSprintf(pcPointer, 1024, " default \'");
			const VeChar8* pcDefault = (const VeChar8*)(kValue.m_pbyBuffer + 2);
			pcPointer += mysql_real_escape_string(m_pkMySQL, pcPointer, pcDefault, (VeUInt32)VeStrlen(pcDefault));
			pcPointer += VeSprintf(pcPointer, 1024, "\'");
		}
		else
		{
			pcPointer += VeSprintf(pcPointer, 1024, " default %f", kValue.m_f64Default);
		}
		pcPointer += VeSprintf(pcPointer, 1024, ",");
	}
	kValue.m_kIndex.m_tSecond = *pu32Iter;
	return pcPointer - pcDesc;
}
//--------------------------------------------------------------------------
VeSizeT VeEntityManagerMySQL::ColumEnum(VeChar8* pcDesc,
	VeNetEntity::Enum& kEnum, VeUInt32& u32Count, VeUInt32 u32Table)
{
	kEnum.m_kIndex.m_tFirst = u32Table;
	kEnum.m_kIndex.m_tSecond = u32Count++;
	VeChar8* pcPointer = pcDesc;
	if(kEnum.m_kField.Length())
	{
		pcPointer += mysql_real_escape_string(m_pkMySQL, pcPointer, kEnum.m_kField, kEnum.m_kField.Length());
		pcPointer += VeSprintf(pcPointer, 1024, "_");
	}
	pcPointer += mysql_real_escape_string(m_pkMySQL, pcPointer, kEnum.m_kName, kEnum.m_kName.Length());
	pcPointer += VeSprintf(pcPointer, 1024, "_db tinyint unsigned not null default %u,", kEnum.m_u8Default);
	return pcPointer - pcDesc;
}
//--------------------------------------------------------------------------
VeSizeT VeEntityManagerMySQL::ColumEnum(VeChar8* pcDesc,
	VeNetEntity::Enum& kEnum, VeStringMap<VeUInt32>& kColumMap,
	VeUInt32 u32Table)
{
	kEnum.m_kIndex.m_tFirst = u32Table;
	VeString kName;
	if(kEnum.m_kField.Length())
	{
		kName += kEnum.m_kField;
		kName += "_";
	}
	kName += kEnum.m_kName;
	kName += "_db";
	VeChar8* pcPointer = pcDesc;
	VeUInt32* pu32Iter = kColumMap.Find(kName);
	if(!pu32Iter)
	{
		pu32Iter = &kColumMap[kName];
		*pu32Iter = kColumMap.Size() - 1;
		pcPointer += mysql_real_escape_string(m_pkMySQL, pcPointer, kName, kName.Length());
		pcPointer += VeSprintf(pcPointer, 1024, " tinyint unsigned not null default %u,", kEnum.m_u8Default);
	}
	kEnum.m_kIndex.m_tSecond = *pu32Iter;
	return pcPointer - pcDesc;
}
//--------------------------------------------------------------------------
VeSizeT VeEntityManagerMySQL::ColumStruct(VeChar8* pcDesc,
	VeNetEntity& kEntity, VeNetEntity::Struct& kStruct,
	VeUInt32& u32Count, VeUInt32 u32Table)
{
	VeChar8* pcPointer = pcDesc;
	for(VeUInt32 i(0); i < kStruct.m_kProperties.Size(); ++i)
	{
		switch(kStruct.m_kProperties[i]->GetType())
		{
		case VeNetEntity::Property::TYPE_VALUE:
			pcPointer += ColumValue(pcPointer, (VeNetEntity::Value&)*(kStruct.m_kProperties[i]), u32Count, u32Table);
			break;
		case VeNetEntity::Property::TYPE_ENUM:
			pcPointer += ColumEnum(pcPointer, (VeNetEntity::Enum&)*(kStruct.m_kProperties[i]), u32Count, u32Table);
			break;
		case VeNetEntity::Property::TYPE_STRUCT:
			pcPointer += ColumStruct(pcPointer, kEntity, (VeNetEntity::Struct&)*(kStruct.m_kProperties[i]), u32Count, u32Table);
			break;
		case VeNetEntity::Property::TYPE_ARRAY:
			ColumArray(kEntity, (VeNetEntity::Array&)*(kStruct.m_kProperties[i]), u32Table);
			break;
		default:
			break;
		}
	}
	return pcPointer - pcDesc;
}
//--------------------------------------------------------------------------
VeSizeT VeEntityManagerMySQL::ColumStruct(VeChar8* pcDesc,
	VeNetEntity& kEntity, VeNetEntity::Struct& kStruct,
	VeStringMap<VeUInt32>& kColumMap, VeUInt32 u32Table)
{
	VeChar8* pcPointer = pcDesc;
	for(VeUInt32 i(0); i < kStruct.m_kProperties.Size(); ++i)
	{
		switch(kStruct.m_kProperties[i]->GetType())
		{
		case VeNetEntity::Property::TYPE_VALUE:
			pcPointer += ColumValue(pcPointer, (VeNetEntity::Value&)*(kStruct.m_kProperties[i]), kColumMap, u32Table);
			break;
		case VeNetEntity::Property::TYPE_ENUM:
			pcPointer += ColumEnum(pcPointer, (VeNetEntity::Enum&)*(kStruct.m_kProperties[i]), kColumMap, u32Table);
			break;
		case VeNetEntity::Property::TYPE_STRUCT:
			pcPointer += ColumStruct(pcPointer, kEntity, (VeNetEntity::Struct&)*(kStruct.m_kProperties[i]), kColumMap, u32Table);
			break;
		case VeNetEntity::Property::TYPE_ARRAY:
			ColumArray(kEntity, (VeNetEntity::Array&)*(kStruct.m_kProperties[i]), u32Table);
			break;
		default:
			break;
		}
	}
	return pcPointer - pcDesc;
}
//--------------------------------------------------------------------------
void VeEntityManagerMySQL::ColumArray(VeNetEntity& kEntity,
	VeNetEntity::Array& kArray, VeUInt32 u32Table)
{
	VeUInt32 u32NewTable = kEntity.m_kTableArray.Size();
	kArray.m_u32Table = kEntity.m_kTableArray.Size();
	kEntity.m_kTableArray.Increase();
	VeString& kTableName = kEntity.m_kTableArray.Back().m_kName;
	kTableName = kEntity.m_kName;
	if(kArray.m_kField.Length())
	{
		kTableName += "_";
		kTableName += kArray.m_kField;
	}
	kTableName += "_";
	kTableName += kArray.m_kName;
	kTableName += "_db";
	kEntity.m_kTableArray.Back().m_u32Reference = u32Table;
	VeChar8 acBuffer[2048];
	bool bExist = HasTable(kTableName, kTableName.Length());
	if(bExist)
	{
		kEntity.m_kTableArray.Back().m_bNeedToLink = false;
		VeStringMap<VeUInt32> kColumMap;
		{
			VeChar8* pcPointer(acBuffer);
			pcPointer += VeSprintf(pcPointer, 2048, "show columns from ");
			pcPointer += mysql_real_escape_string(m_pkMySQL, pcPointer, kTableName, kTableName.Length());
			VE_QUERY(m_pkMySQL, acBuffer);
			MYSQL_RES* pkRes = mysql_store_result(m_pkMySQL);
			VeUInt32 i(0);
			for(MYSQL_ROW row = mysql_fetch_row(pkRes); row; row =  mysql_fetch_row(pkRes))
			{
				kColumMap[row[0]] = i++;
			}
			mysql_free_result(pkRes);
		}
		VeUInt32 u32PreColumNum = kColumMap.Size();
		VeChar8* pcPointer(acBuffer);
		pcPointer += VeSprintf(pcPointer, 1024, "alter table ");
		pcPointer += mysql_real_escape_string(m_pkMySQL, pcPointer, kTableName, kTableName.Length());
		pcPointer += VeSprintf(pcPointer, 1024, " add (");
		pcPointer += ColumProperty(pcPointer, kEntity, *kArray.m_spContent, kColumMap, u32NewTable);
		if(u32PreColumNum < kColumMap.Size())
		{
			*(pcPointer-1) = ')';
			VE_QUERY(m_pkMySQL, acBuffer);
		}
	}
	else
	{
		kEntity.m_kTableArray.Back().m_bNeedToLink = true;
		VeChar8* pcPointer(acBuffer);
		pcPointer += VeSprintf(pcPointer, 1024, "create table ");
		pcPointer += mysql_real_escape_string(m_pkMySQL, pcPointer, kTableName, kTableName.Length());
		pcPointer += VeSprintf(pcPointer, 1024, " (");
		pcPointer += mysql_real_escape_string(m_pkMySQL, pcPointer, kTableName, kTableName.Length());
		pcPointer += VeSprintf(pcPointer, 1024, "_id int unsigned not null auto_increment,");
		if(u32Table)
		{
			pcPointer += VeSprintf(pcPointer, 1024, "foreign_id int unsigned not null,");
		}
		else
		{
			pcPointer += ColumValue(pcPointer, (VeNetEntity::Value&)*(kEntity.m_kProperties.Front()));
		}
		VeUInt32 u32Count(2);
		pcPointer += ColumProperty(pcPointer, kEntity, *kArray.m_spContent, u32Count, u32NewTable);
		pcPointer += VeSprintf(pcPointer, 1024, "primary key(");
		pcPointer += mysql_real_escape_string(m_pkMySQL, pcPointer, kTableName, kTableName.Length());
		pcPointer += VeSprintf(pcPointer, 1024, "_id))engine=%s charset=utf8", s_apcStoreType[kEntity.m_eStore]);
		VE_QUERY(m_pkMySQL, acBuffer);
	}
}
//--------------------------------------------------------------------------
bool VeEntityManagerMySQL::HasTable(const VeChar8* pcName,
	VeUInt32 u32Length)
{
	bool bRes(false);
	VeChar8 acBuffer[2048];
	VeChar8* pcPointer(acBuffer);
	pcPointer += VeSprintf(pcPointer, 1024, "show tables like \'");
	pcPointer += mysql_real_escape_string(m_pkMySQL, pcPointer, pcName, u32Length);
	pcPointer += VeSprintf(pcPointer, 1024, "\'");
	printf("Succeed: %s\n", acBuffer);
	if(mysql_query(m_pkMySQL, acBuffer))
	{
		printf("Error %u: %s\n", mysql_errno(m_pkMySQL), mysql_error(m_pkMySQL));
		return false;
	}
	MYSQL_RES* pkRes = mysql_store_result(m_pkMySQL);
	bRes = mysql_num_rows(pkRes) == 1;
	mysql_free_result(pkRes);
	return bRes;
}
//--------------------------------------------------------------------------
static VeNetEntity::DataPtr CreateDataValue(MYSQL* pkMySQL,
	MYSQL_ROW ppcRow, VeUInt32 u32Table, VeNetEntity::Value& kValue)
{
	VE_ASSERT(u32Table == kValue.m_kIndex.m_tFirst);
	VeChar8* pcData = ppcRow[kValue.m_kIndex.m_tSecond];
	VeNetEntity::DataValue* pkValue = VE_NEW VeNetEntity::DataValue;
	pkValue->m_pkObject = &kValue;
	if(kValue.m_eType < VeNetEntity::VALUE_VeInt8)
	{
		pkValue->m_u64Value = strtoull(pcData, NULL, 10);
	}
	else if(kValue.m_eType < VeNetEntity::VALUE_VeFloat32)
	{
		pkValue->m_i64Value = strtoll(pcData, NULL, 10);
	}
	else if(kValue.m_eType < VeNetEntity::VALUE_VeString)
	{
		pkValue->m_f64Value = VeAtof(pcData);
	}
	else
	{
		VE_ASSERT(kValue.m_eType == VeNetEntity::VALUE_VeString);
		VeUInt32 u32Len = (VeUInt32)VeStrlen(pcData);
		VE_ASSERT(u32Len <= (VeUInt32)(*(VeUInt16*)kValue.m_pbyBuffer));
		pkValue->m_pbyBuffer = VeAlloc(VeByte, u32Len + 3);
		*(VeUInt16*)pkValue->m_pbyBuffer = u32Len;
		VeMemcpy(pkValue->m_pbyBuffer + 2, pcData, u32Len);
		pkValue->m_pbyBuffer[u32Len + 2] = 0;
	}
	return pkValue;
}
//--------------------------------------------------------------------------
static VeNetEntity::DataPtr CreateDataEnum(MYSQL* pkMySQL,
	MYSQL_ROW ppcRow, VeUInt32 u32Table, VeNetEntity::Enum& kEnum)
{
	VE_ASSERT(u32Table == kEnum.m_kIndex.m_tFirst);
	VeChar8* pcData = ppcRow[kEnum.m_kIndex.m_tSecond];
	VeNetEntity::DataEnum* pkEnum = VE_NEW VeNetEntity::DataEnum;
	pkEnum->m_pkObject = &kEnum;
	pkEnum->m_u8Value = VeAtoi(pcData);
	VE_ASSERT(pkEnum->m_u8Value < kEnum.m_kDesc.m_kIntToStr.Size());
	return pkEnum;
}
//--------------------------------------------------------------------------
static VeNetEntity::DataPtr CreateData(MYSQL* pkMySQL, MYSQL_ROW ppcRow,
	VeUInt32 u32Table, const VeNetEntity::PropertyPtr& spProperty,
	const VeNetEntityPtr& spEntity, VeNetEntity::DataValue* pkKey);
//--------------------------------------------------------------------------
static VeNetEntity::DataPtr CreateDataStruct(MYSQL* pkMySQL,
	MYSQL_ROW ppcRow, VeUInt32 u32Table, VeNetEntity::Struct& kStruct,
	const VeNetEntityPtr& spEntity, VeNetEntity::DataValue* pkKey)
{
	VE_ASSERT(pkKey);
	VeNetEntity::DataStruct* pkStruct = VE_NEW VeNetEntity::DataStruct;
	pkStruct->m_pkObject = &kStruct;
	for(VeUInt32 i(0); i < kStruct.m_kProperties.Size(); ++i)
	{
		VeNetEntity::DataPtr spData = CreateData(pkMySQL, ppcRow,
			u32Table, kStruct.m_kProperties[i], spEntity, pkKey);
		VE_ASSERT(spData);
		spData->m_pkHolder = pkStruct;
		pkStruct->m_kDataArray.PushBack(spData);
	}
	return pkStruct;
}
//--------------------------------------------------------------------------
static VeNetEntity::DataPtr CreateDataArray(MYSQL* pkMySQL,
	MYSQL_ROW ppcRow, VeUInt32 u32Table, VeNetEntity::Array& kArray,
	const VeNetEntityPtr& spEntity, VeNetEntity::DataValue* pkKey)
{
	VE_ASSERT(pkKey);
	VeNetEntity::DataArray* pkArray = VE_NEW VeNetEntity::DataArray;
	pkArray->m_pkObject = &kArray;

	VE_ASSERT(kArray.m_u32Table && kArray.m_u32Table < spEntity->m_kTableArray.Size());
	VeString& kTableName = spEntity->m_kTableArray[kArray.m_u32Table].m_kName;
	
	VeChar8 acBuffer[1024];
	VeChar8* pcPointer(acBuffer);
	pcPointer += VeSprintf(pcPointer, 512, "select * from ");
	pcPointer += mysql_real_escape_string(pkMySQL, pcPointer, kTableName, kTableName.Length());
	pcPointer += VeSprintf(pcPointer, 512, " where ");
	if(u32Table)
	{
		pcPointer += VeSprintf(pcPointer, 512, "foreign_id=%u", (VeSizeT)(void*)pkKey);
	}
	else
	{
		VeNetEntity::Value& kValue = (VeNetEntity::Value&)*(pkKey->m_pkObject);
		pcPointer += mysql_real_escape_string(pkMySQL, pcPointer, kValue.m_kName, kValue.m_kName.Length());
		pcPointer += VeSprintf(pcPointer, 512, "_db=");
		if(kValue.m_eType < VeNetEntity::VALUE_VeInt8)
		{
			pcPointer += VeSprintf(pcPointer, 512, "%llu", pkKey->m_u64Value);
		}
		else if(kValue.m_eType < VeNetEntity::VALUE_VeFloat32)
		{
			pcPointer += VeSprintf(pcPointer, 512, "%ll", pkKey->m_i64Value);
		}
		else
		{
			VE_ASSERT(kValue.m_eType == VeNetEntity::VALUE_VeString);
			pcPointer += VeSprintf(pcPointer, 512, "\'");
			VeMemcpy(pcPointer, pkKey->m_pbyBuffer + 2, *(VeUInt16*)pkKey->m_pbyBuffer);
			pcPointer += *(VeUInt16*)pkKey->m_pbyBuffer;
			*pcPointer = 0;
			pcPointer += VeSprintf(pcPointer, 512, "\'");
		}

	}
	pcPointer += VeSprintf(pcPointer, 512, " order by ");
	pcPointer += mysql_real_escape_string(pkMySQL, pcPointer, kTableName, kTableName.Length());
	pcPointer += VeSprintf(pcPointer, 512, "_id");
	VE_QUERY(pkMySQL, acBuffer);
	MYSQL_RES* pkRes = mysql_store_result(pkMySQL);
	MYSQL_ROW row;
	while(row = mysql_fetch_row(pkRes))
	{
		pkArray->m_kDataArray.Increase();
		VePair<VeUInt32,VeNetEntity::DataPtr>& kNew = pkArray->m_kDataArray.Back();
		kNew.m_tFirst = VeAtoi(row[0]);
		kNew.m_tSecond = CreateData(pkMySQL, row, kArray.m_u32Table, kArray.m_spContent,
			spEntity, (VeNetEntity::DataValue*)(void*)VeSizeT(kNew.m_tFirst));
		VE_ASSERT(kNew.m_tSecond);
		kNew.m_tSecond->m_pkHolder = pkArray;
	}
	mysql_free_result(pkRes);
	return pkArray;
}
//--------------------------------------------------------------------------
static VeNetEntity::DataPtr CreateData(MYSQL* pkMySQL, MYSQL_ROW ppcRow,
	VeUInt32 u32Table, const VeNetEntity::PropertyPtr& spProperty,
	const VeNetEntityPtr& spEntity, VeNetEntity::DataValue* pkKey)
{
	VE_ASSERT(spProperty);
	if(pkKey)
	{
		switch(spProperty->GetType())
		{
		case VeNetEntity::Property::TYPE_VALUE:
			return CreateDataValue(pkMySQL, ppcRow, u32Table, (VeNetEntity::Value&)*spProperty);
		case VeNetEntity::Property::TYPE_ENUM:
			return CreateDataEnum(pkMySQL, ppcRow, u32Table, (VeNetEntity::Enum&)*spProperty);
		case VeNetEntity::Property::TYPE_STRUCT:
			return CreateDataStruct(pkMySQL, ppcRow, u32Table, (VeNetEntity::Struct&)*spProperty, spEntity, pkKey);
		case VeNetEntity::Property::TYPE_ARRAY:
			return CreateDataArray(pkMySQL, ppcRow, u32Table, (VeNetEntity::Array&)*spProperty, spEntity, pkKey);
		default:
			break;
		}
	}
	else
	{
		VE_ASSERT(spProperty->GetType() == VeNetEntity::Property::TYPE_VALUE);
		return CreateDataValue(pkMySQL, ppcRow, u32Table, (VeNetEntity::Value&)*spProperty);
	}
	return NULL;
}
//--------------------------------------------------------------------------
static VeNetEntity::DataEntityPtr CreateData(MYSQL* pkMySQL,
	MYSQL_ROW ppcRow, const VeNetEntityPtr& spEntity)
{
	VeNetEntity::DataEntityPtr spDataEnt = VE_NEW VeNetEntity::DataEntity;
	spEntity->m_kFreeList.AttachBack(spDataEnt->m_kNode);
	spDataEnt->m_pkObject = spEntity;

	for(VeUInt32 i(0); i < spEntity->m_kProperties.Size(); ++i)
	{
		if(spEntity->m_kProperties[i]->HasFlag(VeNetEntity::Property::FLAG_DATABASE))
		{
			VeNetEntity::DataPtr spData;
			spData = CreateData(pkMySQL, ppcRow, 0, spEntity->m_kProperties[i], spEntity,
				i ? (VeNetEntity::DataValue*)&(*spDataEnt->m_kDataArray.Front()) : NULL);
			VE_ASSERT(spData);
			spData->m_pkHolder = spDataEnt;
			spDataEnt->m_kDataArray.PushBack(spData);
		}
	}

	return spDataEnt;
}
//--------------------------------------------------------------------------
VeNetEntity::DataEntityPtr VeEntityManagerMySQL::GetEntityData(
	VeNetEntity& kEntity)
{
	VE_ASSERT(kEntity.m_eType == VeNetEntity::TYPE_GLOBAL);
	if(kEntity.m_kDataArray.Size())
	{
		return kEntity.m_kDataArray.Front();
	}
	return NULL;
}
//--------------------------------------------------------------------------
VeNetEntity::DataEntityPtr VeEntityManagerMySQL::GetEntityData(
	VeNetEntity& kEntity, VeString& kKey)
{
	VE_ASSERT(kEntity.m_eType != VeNetEntity::TYPE_GLOBAL);
	VE_ASSERT(kEntity.m_kProperties.Front()->GetType() == VeNetEntity::Property::TYPE_VALUE);
	VeNetEntity::Value& kValue = (VeNetEntity::Value&)*kEntity.m_kProperties.Front();
	VE_ASSERT(kValue.m_spKeyMap && kValue.m_spKeyMap->GetType() == VeNetEntity::KeyMap::TYPE_STRING);
	VeStringMap<VeUInt32>& kMap = ((VeNetEntity::StrMap&)*kValue.m_spKeyMap).m_kMap;
	VeUInt32* pu32Iter = kMap.Find(kKey);
	if(pu32Iter && (*pu32Iter) < kEntity.m_kDataArray.Size())
	{
		return kEntity.m_kDataArray[*pu32Iter];
	}
	return NULL;
}
//--------------------------------------------------------------------------
static void Insert(MYSQL* pkMySQL, VeString& kTable)
{
	VeChar8 acBuffer[1024];
	VeChar8* pcPointer(acBuffer);
	pcPointer += VeSprintf(pcPointer, 512, "insert into ");
	pcPointer += mysql_real_escape_string(pkMySQL, pcPointer, kTable, kTable.Length());
	pcPointer += VeSprintf(pcPointer, 512, " values()");
	VE_QUERY(pkMySQL, acBuffer);
}
//--------------------------------------------------------------------------
VeNetEntity::DataEntityPtr VeEntityManagerMySQL::LoadEntityData(
	VeNetEntity& kEntity)
{
	VE_ASSERT(kEntity.m_eType == VeNetEntity::TYPE_GLOBAL);
	if(kEntity.m_kDataArray.Empty())
	{
		VeString& kTableName = kEntity.m_kTableArray.Front().m_kName;
		MYSQL_RES* pkRes(NULL);
		VeChar8 acBuffer[1024];
		VeChar8* pcPointer(acBuffer);
		pcPointer += VeSprintf(pcPointer, 512, "select * from ");
		pcPointer += mysql_real_escape_string(m_pkMySQL, pcPointer, kTableName, kTableName.Length());
		VE_QUERY(m_pkMySQL, acBuffer);
		pkRes = mysql_store_result(m_pkMySQL);
		if(!mysql_num_rows(pkRes))
		{
			mysql_free_result(pkRes);
			pkRes = NULL;
		}
		if(!pkRes)
		{
			Insert(m_pkMySQL, kTableName);
			VE_QUERY(m_pkMySQL, acBuffer);
			pkRes = mysql_store_result(m_pkMySQL);
			if(!mysql_num_rows(pkRes))
			{
				mysql_free_result(pkRes);
				pkRes = NULL;
			}
		}
		if(pkRes)
		{
			VeNetEntity::DataEntityPtr spData = CreateData(m_pkMySQL, mysql_fetch_row(pkRes), &kEntity);
			spData->m_pkHolder = &m_kHolder;
			kEntity.m_kDataArray.PushBack(spData);
			mysql_free_result(pkRes);
		}
	}
	if(kEntity.m_kDataArray.Size() && kEntity.m_kDataArray.Front()->m_kNode.IsAttach(kEntity.m_kFreeList))
	{
		return kEntity.m_kDataArray.Front();
	}
	return NULL;
}
//--------------------------------------------------------------------------
static void Insert(MYSQL* pkMySQL, VeString& kTable, VeString& kName,
	VeString& kKey)
{
	VeChar8 acBuffer[1024];
	VeChar8* pcPointer(acBuffer);
	pcPointer += VeSprintf(pcPointer, 512, "insert into ");
	pcPointer += mysql_real_escape_string(pkMySQL, pcPointer, kTable, kTable.Length());
	pcPointer += VeSprintf(pcPointer, 512, "(");
	pcPointer += mysql_real_escape_string(pkMySQL, pcPointer, kName, kName.Length());
	pcPointer += VeSprintf(pcPointer, 512, "_db) values(\'");
	pcPointer += mysql_real_escape_string(pkMySQL, pcPointer, kKey, kKey.Length());
	pcPointer += VeSprintf(pcPointer, 512, "\')");
	VE_QUERY(pkMySQL, acBuffer);
}
//--------------------------------------------------------------------------
VeNetEntity::DataEntityPtr VeEntityManagerMySQL::LoadEntityData(
	VeNetEntity& kEntity, VeString& kKey)
{
	VE_ASSERT(kEntity.m_eType != VeNetEntity::TYPE_GLOBAL);
	VE_ASSERT(kEntity.m_kProperties.Front()->GetType() == VeNetEntity::Property::TYPE_VALUE);
	VeNetEntity::Value& kValue = (VeNetEntity::Value&)*kEntity.m_kProperties.Front();
	VE_ASSERT(kValue.m_spKeyMap && kValue.m_spKeyMap->GetType() == VeNetEntity::KeyMap::TYPE_STRING);
	VeStringMap<VeUInt32>& kMap = ((VeNetEntity::StrMap&)*kValue.m_spKeyMap).m_kMap;
	VeUInt32* pu32Iter = kMap.Find(kKey);
	if(!pu32Iter)
	{
		VeString& kTableName = kEntity.m_kTableArray.Front().m_kName;
		MYSQL_RES* pkRes(NULL);
		VeChar8 acBuffer[1024];
		VeChar8* pcPointer(acBuffer);
		pcPointer += VeSprintf(pcPointer, 512, "select * from ");
		pcPointer += mysql_real_escape_string(m_pkMySQL, pcPointer, kTableName, kTableName.Length());
		pcPointer += VeSprintf(pcPointer, 512, " where ");
		pcPointer += mysql_real_escape_string(m_pkMySQL, pcPointer, kValue.m_kName, kValue.m_kName.Length());
		pcPointer += VeSprintf(pcPointer, 512, "_db=\'");
		pcPointer += mysql_real_escape_string(m_pkMySQL, pcPointer, kKey, kKey.Length());
		pcPointer += VeSprintf(pcPointer, 512, "\'");
		VE_QUERY(m_pkMySQL, acBuffer);
		pkRes = mysql_store_result(m_pkMySQL);
		if(!mysql_num_rows(pkRes))
		{
			mysql_free_result(pkRes);
			pkRes = NULL;
		}
		if(!pkRes)
		{
			Insert(m_pkMySQL, kTableName, kValue.m_kName, kKey);
			VE_QUERY(m_pkMySQL, acBuffer);
			pkRes = mysql_store_result(m_pkMySQL);
			if(!mysql_num_rows(pkRes))
			{
				mysql_free_result(pkRes);
				pkRes = NULL;
			}
		}
		if(pkRes)
		{
			VeNetEntity::DataEntityPtr spData = CreateData(m_pkMySQL, mysql_fetch_row(pkRes), &kEntity);
			spData->m_pkHolder = &m_kHolder;
			pu32Iter = &kMap[kKey];
			*pu32Iter = kEntity.m_kDataArray.Size();
			kEntity.m_kDataArray.PushBack(spData);
			mysql_free_result(pkRes);
		}
	}
	if(pu32Iter && (*pu32Iter) < kEntity.m_kDataArray.Size()
		&& kEntity.m_kDataArray[*pu32Iter]->m_kNode.IsAttach(kEntity.m_kFreeList))
	{
		return kEntity.m_kDataArray[*pu32Iter];
	}
	return NULL;
}
//--------------------------------------------------------------------------
VeEntityManagerMySQL::EntityHolder::EntityHolder()
{

}
//--------------------------------------------------------------------------
VeEntityManagerMySQL::EntityHolder::~EntityHolder()
{

}
//--------------------------------------------------------------------------
VeNetEntity::Data::Type VeEntityManagerMySQL::EntityHolder::GetType() const
{
	return VeNetEntity::Data::TYPE_MAX;
}
//--------------------------------------------------------------------------
class EntitySaveEvent : public VeTimeEvent
{
public:
	EntitySaveEvent(st_mysql* pkSQL, VeRefNode<VeNetEntity::Data*>& kNode);

	VeDeclTimeDelegate(EntitySaveEvent, OnEvent);
	st_mysql* m_pkMySQL;
	VeRefList<VeNetEntity::Data*> m_kEntityList;

};
//--------------------------------------------------------------------------
EntitySaveEvent::EntitySaveEvent(st_mysql* pkSQL,
	VeRefNode<VeNetEntity::Data*>& kNode) : m_pkMySQL(pkSQL)
{
	VeInitDelegate(EntitySaveEvent, OnEvent);
	AddDelegate(VeDelegate(OnEvent));
	m_kEntityList.AttachBack(kNode);
}
//--------------------------------------------------------------------------
static VeSizeT Save(MYSQL* pkMySQL, VeChar8* pcDesc,
	VeNetEntity::DataValue& kData, VeUInt32 u32Table)
{
	VeNetEntity::Value& kValue = *(VeNetEntity::Value*)kData.m_pkObject;
	VE_ASSERT(kValue.m_kIndex.m_tFirst == u32Table && kValue.m_eType < VeNetEntity::VALUE_TYPE_MAX);
	VeChar8* pcPointer = pcDesc;
	if(kValue.m_kField.Length())
	{
		pcPointer += mysql_real_escape_string(pkMySQL, pcPointer, kValue.m_kField, kValue.m_kField.Length());
		pcPointer += VeSprintf(pcPointer, 1024, "_");
	}
	pcPointer += mysql_real_escape_string(pkMySQL, pcPointer, kValue.m_kName, kValue.m_kName.Length());
	if(kValue.m_eType < VeNetEntity::VALUE_VeInt8)
	{
		pcPointer += VeSprintf(pcPointer, 1024, "_db=%llu", kData.m_u64Value);
	}
	else if(kValue.m_eType < VeNetEntity::VALUE_VeFloat32)
	{
		pcPointer += VeSprintf(pcPointer, 1024, "_db=%ll", kData.m_i64Value);
	}
	else if(kValue.m_eType < VeNetEntity::VALUE_VeString)
	{
		pcPointer += VeSprintf(pcPointer, 1024, "_db=%f", kData.m_f64Value);
	}
	else
	{
		pcPointer += VeSprintf(pcPointer, 1024, "_db=\'");
		pcPointer += mysql_real_escape_string(pkMySQL, pcPointer,
			(const VeChar8*)(kData.m_pbyBuffer + 2), *(VeUInt16*)kData.m_pbyBuffer);
		pcPointer += VeSprintf(pcPointer, 1024, "\'");
	}
	pcPointer += VeSprintf(pcPointer, 1024, ",");
	return pcPointer - pcDesc;
}
//--------------------------------------------------------------------------
static VeSizeT Save(MYSQL* pkMySQL, VeChar8* pcDesc,
	VeNetEntity::DataEnum& kData, VeUInt32 u32Table)
{
	VeNetEntity::Enum& kEnum = *(VeNetEntity::Enum*)kData.m_pkObject;
	VE_ASSERT(kEnum.m_kIndex.m_tFirst == u32Table);
	VeChar8* pcPointer = pcDesc;
	if(kEnum.m_kField.Length())
	{
		pcPointer += mysql_real_escape_string(pkMySQL, pcPointer, kEnum.m_kField, kEnum.m_kField.Length());
		pcPointer += VeSprintf(pcPointer, 1024, "_");
	}
	pcPointer += mysql_real_escape_string(pkMySQL, pcPointer, kEnum.m_kName, kEnum.m_kName.Length());
	pcPointer += VeSprintf(pcPointer, 1024, "_db=%u,", kData.m_u8Value);
	return pcPointer - pcDesc;
}
//--------------------------------------------------------------------------
static VeSizeT Save(MYSQL* pkMySQL, VeChar8* pcDesc,
	VeNetEntity::Data& kData, VeNetEntity::DataValue* pkKey,
	VeUInt32 u32Table, VeNetEntity& kEntity);
//--------------------------------------------------------------------------
static VeSizeT Save(MYSQL* pkMySQL, VeChar8* pcDesc,
	VeNetEntity::DataStruct& kData, VeNetEntity::DataValue* pkKey,
	VeUInt32 u32Table, VeNetEntity& kEntity)
{
	VeChar8* pcPointer = pcDesc;
	kData.m_kDirtyList.BeginIterator();
	while(!kData.m_kDirtyList.IsEnd())
	{
		VeNetEntity::Data* pkData = kData.m_kDirtyList.GetIterNode()->m_tContent;
		kData.m_kDirtyList.Next();
		pcPointer += Save(pkMySQL, pcPointer, *pkData, pkKey, u32Table, kEntity);
	}
	return pcPointer - pcDesc;
}
//--------------------------------------------------------------------------
static VeUInt32 GetTableAutoIncrement(MYSQL* pkMySQL, VeString& kTable)
{
	VeUInt32 u32Res(0);
	VeChar8 acBuffer[256];
	VeChar8* pcPointer(acBuffer);
	pcPointer += VeSprintf(pcPointer, 128, "show table status like \'");
	pcPointer += mysql_real_escape_string(pkMySQL, pcPointer, kTable, kTable.Length());
	pcPointer += VeSprintf(pcPointer, 128, "\'");
	VE_QUERY(pkMySQL, acBuffer);
	MYSQL_RES* pkRes = mysql_store_result(pkMySQL);
	VE_ASSERT(pkRes);
	MYSQL_ROW row = mysql_fetch_row(pkRes);
	u32Res = strtoul(row[10], NULL, 10);
	mysql_free_result(pkRes);
	return u32Res;
}
//--------------------------------------------------------------------------
static VeSizeT SetKey(MYSQL* pkMySQL, VeChar8* pcDesc,
	VeNetEntity::DataValue* pkKey, VeUInt32 u32Table)
{
	VeChar8* pcPointer = pcDesc;
	if(u32Table)
	{
		pcPointer += VeSprintf(pcPointer, 1024, "foreign_id=%llu", (VeUInt64)(void*)pkKey);
	}
	else
	{
		pcPointer += Save(pkMySQL, pcPointer, *pkKey, 0);
		*(--pcPointer) = 0;
	}
	return pcPointer - pcDesc;
}
//--------------------------------------------------------------------------
static VeSizeT SetKeyName(MYSQL* pkMySQL, VeChar8* pcDesc,
	VeNetEntity::DataValue* pkKey, VeUInt32 u32Table)
{
	VeChar8* pcPointer = pcDesc;
	if(u32Table)
	{
		pcPointer += VeSprintf(pcPointer, 1024, "foreign_id,");
	}
	else
	{
		VeNetEntity::Value& kValue = *(VeNetEntity::Value*)pkKey->m_pkObject;
		if(kValue.m_kField.Length())
		{
			pcPointer += mysql_real_escape_string(pkMySQL, pcPointer, kValue.m_kField, kValue.m_kField.Length());
			pcPointer += VeSprintf(pcPointer, 1024, "_");
		}
		pcPointer += mysql_real_escape_string(pkMySQL, pcPointer, kValue.m_kName, kValue.m_kName.Length());
		pcPointer += VeSprintf(pcPointer, 1024, "_db,");
	}
	return pcPointer - pcDesc;
}
//--------------------------------------------------------------------------
static VeSizeT SetKeyValue(MYSQL* pkMySQL, VeChar8* pcDesc,
	VeNetEntity::DataValue* pkKey, VeUInt32 u32Table)
{
	VeChar8* pcPointer = pcDesc;
	if(u32Table)
	{
		pcPointer += VeSprintf(pcPointer, 1024, "%llu", (VeUInt64)(void*)pkKey);
	}
	else
	{
		VeNetEntity::Value& kValue = *(VeNetEntity::Value*)pkKey->m_pkObject;
		if(kValue.m_eType < VeNetEntity::VALUE_VeInt8)
		{
			pcPointer += VeSprintf(pcPointer, 1024, "%llu", pkKey->m_u64Value);
		}
		else if(kValue.m_eType < VeNetEntity::VALUE_VeFloat32)
		{
			pcPointer += VeSprintf(pcPointer, 1024, "%ll", pkKey->m_i64Value);
		}
		else if(kValue.m_eType < VeNetEntity::VALUE_VeString)
		{
			pcPointer += VeSprintf(pcPointer, 1024, "%f", pkKey->m_f64Value);
		}
		else
		{
			pcPointer += VeSprintf(pcPointer, 1024, "\'");
			pcPointer += mysql_real_escape_string(pkMySQL, pcPointer,
				(const VeChar8*)(pkKey->m_pbyBuffer + 2), *(VeUInt16*)pkKey->m_pbyBuffer);
			pcPointer += VeSprintf(pcPointer, 1024, "\'");
		}
	}
	pcPointer += VeSprintf(pcPointer, 1024, ",");
	return pcPointer - pcDesc;
}
//--------------------------------------------------------------------------
static VeSizeT SetPropertyName(MYSQL* pkMySQL, VeChar8* pcDesc,
	VeNetEntity::Property& kProperty);
//--------------------------------------------------------------------------
static VeSizeT SetPropertyName(MYSQL* pkMySQL, VeChar8* pcDesc,
	VeNetEntity::Struct& kProperty)
{
	VeChar8* pcPointer = pcDesc;
	for(VeUInt32 i(0); i < kProperty.m_kProperties.Size(); ++i)
	{
		pcPointer += SetPropertyName(pkMySQL, pcPointer, *kProperty.m_kProperties[i]);
	}
	return pcPointer - pcDesc;
}
//--------------------------------------------------------------------------
static VeSizeT SetPropertyName(MYSQL* pkMySQL, VeChar8* pcDesc,
	VeNetEntity::Property& kProperty)
{
	VeChar8* pcPointer = pcDesc;
	switch(kProperty.GetType())
	{
	case VeNetEntity::Property::TYPE_VALUE:
	case VeNetEntity::Property::TYPE_ENUM:
		{
			if(kProperty.m_kField.Length())
			{
				pcPointer += mysql_real_escape_string(pkMySQL, pcPointer, kProperty.m_kField, kProperty.m_kField.Length());
				pcPointer += VeSprintf(pcPointer, 1024, "_");
			}
			pcPointer += mysql_real_escape_string(pkMySQL, pcPointer, kProperty.m_kName, kProperty.m_kName.Length());
			pcPointer += VeSprintf(pcPointer, 1024, "_db,");
		}
		break;
	case VeNetEntity::Property::TYPE_STRUCT:
		pcPointer += SetPropertyName(pkMySQL, pcDesc, (VeNetEntity::Struct&)kProperty);
		break;
	default:
		break;
	}
	return pcPointer - pcDesc;
}
//--------------------------------------------------------------------------
static VeSizeT SetData(MYSQL* pkMySQL, VeChar8* pcDesc,
	VeNetEntity::DataValue& kData, VeUInt32 u32Table)
{
	VeNetEntity::Value& kValue = *(VeNetEntity::Value*)kData.m_pkObject;
	VE_ASSERT(kValue.m_kIndex.m_tFirst == u32Table && kValue.m_eType < VeNetEntity::VALUE_TYPE_MAX);
	VeChar8* pcPointer = pcDesc;
	if(kValue.m_eType < VeNetEntity::VALUE_VeInt8)
	{
		pcPointer += VeSprintf(pcPointer, 1024, "%llu", kData.m_u64Value);
	}
	else if(kValue.m_eType < VeNetEntity::VALUE_VeFloat32)
	{
		pcPointer += VeSprintf(pcPointer, 1024, "%ll", kData.m_i64Value);
	}
	else if(kValue.m_eType < VeNetEntity::VALUE_VeString)
	{
		pcPointer += VeSprintf(pcPointer, 1024, "%f", kData.m_f64Value);
	}
	else
	{
		pcPointer += VeSprintf(pcPointer, 1024, "\'", kData.m_f64Value);
		pcPointer += mysql_real_escape_string(pkMySQL, pcPointer,
			(const VeChar8*)(kData.m_pbyBuffer + 2), *(VeUInt16*)kData.m_pbyBuffer);
		pcPointer += VeSprintf(pcPointer, 1024, "\'", kData.m_f64Value);
	}
	pcPointer += VeSprintf(pcPointer, 1024, ",");
	return pcPointer - pcDesc;
}
//--------------------------------------------------------------------------
static VeSizeT SetData(MYSQL* pkMySQL, VeChar8* pcDesc,
	VeNetEntity::DataEnum& kData, VeUInt32 u32Table)
{
	VeNetEntity::Enum& kEnum = *(VeNetEntity::Enum*)kData.m_pkObject;
	VE_ASSERT(kEnum.m_kIndex.m_tFirst == u32Table);
	VeChar8* pcPointer = pcDesc;
	pcPointer += VeSprintf(pcPointer, 1024, "%u,", kData.m_u8Value);
	return pcPointer - pcDesc;
}
//--------------------------------------------------------------------------
static VeSizeT SetData(MYSQL* pkMySQL, VeChar8* pcDesc,
	VeNetEntity::Data& kData, VeUInt32 u32Table);
//--------------------------------------------------------------------------
static VeSizeT SetData(MYSQL* pkMySQL, VeChar8* pcDesc,
	VeNetEntity::DataStruct& kData, VeUInt32 u32Table)
{
	VeChar8* pcPointer = pcDesc;
	for(VeUInt32 i(0); i < kData.m_kDataArray.Size(); ++i)
	{
		pcPointer += SetData(pkMySQL, pcPointer, *kData.m_kDataArray[i], u32Table);
	}
	return pcPointer - pcDesc;
}
//--------------------------------------------------------------------------
static VeSizeT SetData(MYSQL* pkMySQL, VeChar8* pcDesc,
	VeNetEntity::Data& kData, VeUInt32 u32Table)
{
	switch(kData.GetType())
	{
	case VeNetEntity::Data::TYPE_VALUE:
		return SetData(pkMySQL, pcDesc, (VeNetEntity::DataValue&)kData, u32Table);
	case VeNetEntity::Data::TYPE_ENUM:
		return SetData(pkMySQL, pcDesc, (VeNetEntity::DataEnum&)kData, u32Table);
	case VeNetEntity::Data::TYPE_STRUCT:
		return SetData(pkMySQL, pcDesc, (VeNetEntity::DataStruct&)kData, u32Table);
	default:
		break;
	}
	return 0;
}
//--------------------------------------------------------------------------
static void Save(MYSQL* pkMySQL, VeNetEntity::DataArray& kData,
	VeNetEntity::DataValue* pkKey, VeUInt32 u32Table, VeNetEntity& kEntity);
//--------------------------------------------------------------------------
static void SaveArray(MYSQL* pkMySQL, VeNetEntity::Data& kData,
	VeNetEntity::DataValue* pkKey, VeUInt32 u32Table, VeNetEntity& kEntity);
//--------------------------------------------------------------------------
static void SaveArray(MYSQL* pkMySQL, VeNetEntity::DataStruct& kData,
	VeNetEntity::DataValue* pkKey, VeUInt32 u32Table, VeNetEntity& kEntity)
{
	for(VeUInt32 i(0); i < kData.m_kDataArray.Size(); ++i)
	{
		SaveArray(pkMySQL, *kData.m_kDataArray[i], pkKey, u32Table, kEntity);
	}
}
//--------------------------------------------------------------------------
static void SaveArray(MYSQL* pkMySQL, VeNetEntity::Data& kData,
	VeNetEntity::DataValue* pkKey, VeUInt32 u32Table, VeNetEntity& kEntity)
{
	switch(kData.GetType())
	{
	case VeNetEntity::Data::TYPE_STRUCT:
		SaveArray(pkMySQL, (VeNetEntity::DataStruct&)kData, pkKey, u32Table, kEntity);
		break;
	case VeNetEntity::Data::TYPE_ARRAY:
		Save(pkMySQL, (VeNetEntity::DataArray&)kData, pkKey, u32Table, kEntity);
		break;
	default:
		break;
	}
}
//--------------------------------------------------------------------------
static void Save(MYSQL* pkMySQL, VeNetEntity::DataArray& kData,
	VeNetEntity::DataValue* pkKey, VeUInt32 u32Table, VeNetEntity& kEntity)
{
	VeNetEntity::Array& kArray = *(VeNetEntity::Array*)kData.m_pkObject;
	VeString& kTable = kEntity.m_kTableArray[kArray.m_u32Table].m_kName;
	VeUInt32 u32AutoInc = GetTableAutoIncrement(pkMySQL, kTable);
	VeChar8 acBuffer[4096];
	VeChar8* pcPointer(acBuffer);
	pcPointer += VeSprintf(pcPointer, 1024, "delete from ");
	pcPointer += mysql_real_escape_string(pkMySQL, pcPointer, kTable, kTable.Length());
	pcPointer += VeSprintf(pcPointer, 1024, " where ");
	pcPointer += SetKey(pkMySQL, pcPointer, pkKey, u32Table);
	VE_QUERY(pkMySQL, acBuffer);
	if(kData.m_kDataArray.Size())
	{
		pcPointer = acBuffer;
		pcPointer += VeSprintf(pcPointer, 1024, "insert into ");
		pcPointer += mysql_real_escape_string(pkMySQL, pcPointer, kTable, kTable.Length());
		pcPointer += VeSprintf(pcPointer, 1024, "(");
		pcPointer += SetKeyName(pkMySQL, pcPointer, pkKey, u32Table);
		pcPointer += SetPropertyName(pkMySQL, pcPointer, *kArray.m_spContent);
		*(pcPointer - 1) = ')';
		pcPointer += VeSprintf(pcPointer, 1024, " values");
		for(VeUInt32 i(0); i < kData.m_kDataArray.Size(); ++i)
		{
			kData.m_kDataArray[i].m_tFirst = u32AutoInc + i;
			pcPointer += VeSprintf(pcPointer, 1024, i ? ",(" : "(");
			pcPointer += SetKeyValue(pkMySQL, pcPointer, pkKey, u32Table);
			pcPointer += SetData(pkMySQL, pcPointer, *kData.m_kDataArray[i].m_tSecond, kArray.m_u32Table);
			*(pcPointer - 1) = ')';
		}
	}
	VE_QUERY(pkMySQL, acBuffer);
	for(VeUInt32 i(0); i < kData.m_kDataArray.Size(); ++i)
	{
		SaveArray(pkMySQL, *kData.m_kDataArray[i].m_tSecond,
			(VeNetEntity::DataValue*)kData.m_kDataArray[i].m_tFirst, kArray.m_u32Table, kEntity);
	}
}
//--------------------------------------------------------------------------
static VeSizeT Save(MYSQL* pkMySQL, VeChar8* pcDesc,
	VeNetEntity::Data& kData, VeNetEntity::DataValue* pkKey,
	VeUInt32 u32Table, VeNetEntity& kEntity)
{
	switch(kData.GetType())
	{
	case VeNetEntity::Data::TYPE_VALUE:
		return Save(pkMySQL, pcDesc, (VeNetEntity::DataValue&)kData, u32Table);
	case VeNetEntity::Data::TYPE_ENUM:
		return Save(pkMySQL, pcDesc, (VeNetEntity::DataEnum&)kData, u32Table);
	case VeNetEntity::Data::TYPE_STRUCT:
		return Save(pkMySQL, pcDesc, (VeNetEntity::DataStruct&)kData, pkKey, u32Table, kEntity);
	case VeNetEntity::Data::TYPE_ARRAY:
		Save(pkMySQL, (VeNetEntity::DataArray&)kData, pkKey, u32Table, kEntity);
		break;
	default:
		break;
	}
	return 0;
}
//--------------------------------------------------------------------------
static void Save(MYSQL* pkMySQL, VeNetEntity::DataEntity& kData)
{
	VeNetEntity& kEntity = *(VeNetEntity*)kData.m_pkObject;
	VeChar8 acBuffer[4096];
	VeChar8* pcPointer(acBuffer);
	pcPointer += VeSprintf(pcPointer, 1024, "update ");
	pcPointer += mysql_real_escape_string(pkMySQL, pcPointer,
		kEntity.m_kTableArray[0].m_kName,
		kEntity.m_kTableArray[0].m_kName.Length());
	pcPointer += VeSprintf(pcPointer, 2048, " set ");
	VeUInt32 u32Count(0);
	VE_ASSERT(kData.m_kDataArray.Front()->GetType() == VeNetEntity::Data::TYPE_VALUE);
	kData.m_kDirtyList.BeginIterator();
	while(!kData.m_kDirtyList.IsEnd())
	{
		VeNetEntity::Data* pkData = kData.m_kDirtyList.GetIterNode()->m_tContent;
		kData.m_kDirtyList.Next();
		VeSizeT stOffset = Save(pkMySQL, pcPointer, *pkData, (VeNetEntity::DataValue*)&*kData.m_kDataArray.Front(), 0, kEntity);
		if(stOffset)
		{
			++u32Count;
			pcPointer += stOffset;
		}
	}
	VeNetEntity::ClearDirty(kData);
	VE_ASSERT(kData.m_kDirtyList.Empty());
	if(u32Count)
	{
		*(pcPointer - 1) = ' ';
		pcPointer += VeSprintf(pcPointer, 2048, "where ");
		pcPointer += Save(pkMySQL, pcPointer, (VeNetEntity::DataValue&)*kData.m_kDataArray.Front(), 0);
		*(pcPointer - 1) = 0;
		VE_QUERY(pkMySQL, acBuffer);
	}
}
//--------------------------------------------------------------------------
VeImplTimeDelegate(EntitySaveEvent, OnEvent, spEvent)
{
	m_kEntityList.BeginIterator();
	while(!m_kEntityList.IsEnd())
	{
		VeNetEntity::Data* pkData = m_kEntityList.GetIterNode()->m_tContent;
		m_kEntityList.Next();
		VE_ASSERT(pkData && pkData->GetType() == VeNetEntity::Data::TYPE_ENTITY);
		Save(m_pkMySQL, *(VeNetEntity::DataEntity*)pkData);
	}
	VE_ASSERT(m_kEntityList.Empty());
}
//--------------------------------------------------------------------------
void VeEntityManagerMySQL::EntityHolder::NotifyDirty(
	VeRefNode<VeNetEntity::Data*>& kNode)
{
	VE_ASSERT(m_pkEntityMgr);
	VeNetEntity& kEntity = *(VeNetEntity*)kNode.m_tContent->m_pkObject;
	g_pTime->Attach(VE_NEW EntitySaveEvent(m_pkEntityMgr->m_pkMySQL, kNode), kEntity.m_f32Period);
}
//--------------------------------------------------------------------------
#endif
//--------------------------------------------------------------------------
