////////////////////////////////////////////////////////////////////////////
//
//  Venus Server Header File.
//  Copyright (C), Venus Interactive Entertainment.2012
// -------------------------------------------------------------------------
//  File name:   VeBaseEntity.h
//  Version:     v1.00
//  Created:     13/11/2014 by Napoleon
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//  http://www.venusie.com
////////////////////////////////////////////////////////////////////////////

#pragma once

namespace RakNet
{
	class BitStream;
}

class VeEntityManagerBase : public VeNetEntityManager
{
public:
	struct EntityCache : public VeNetEntity::DataHolder
	{
		EntityCache();

		virtual ~EntityCache();

		virtual Type GetType() const;

		virtual void NotifyDirty(VeRefNode<VeNetEntity::Data*>& kNode);

		VeRefList<VeNetEntity::Data*> m_kDirtyList;
	};


	VeEntityManagerBase();

	virtual ~VeEntityManagerBase();

	VeNetEntity::DataEntityPtr CreateEntity(VeNetEntity& kEntity, RakNet::BitStream& kStream);

	EntityCache m_kDirtyCache;

};

VeSmartPointer(VeEntityManagerBase);
