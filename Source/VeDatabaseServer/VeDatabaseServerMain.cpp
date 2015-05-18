////////////////////////////////////////////////////////////////////////////
//
//  Venus Server Source File.
//  Copyright (C), Venus Interactive Entertainment.2012
// -------------------------------------------------------------------------
//  File name:   VeDatabaseServerMain.cpp
//  Version:     v1.00
//  Created:     23/10/2014 by Napoleon
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//  http://www.venusie.com
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

void main(VeInt32 i32Argc, VeChar8* apcArgv[])
{
	printf("================================================================================");
	printf("=                        Venus Interactive Entertainment                       =");
	printf("=                                                                              =");
	printf("=                                Database Server                               =");
	printf("=                                                                              =");
	printf("=                          Archived By Venus_Napoleon                          =");
	printf("=                                                                              =");
	printf("=                       CopyRight (C)  VenusIE  2012-2018                      =");
	printf("================================================================================");
	
	VeChar8** ppcArgv = apcArgv;

	if(VeStrcmp(*apcArgv, "-dlua") == 0)
	{
		VeConsoleInit(true);
		++ppcArgv;
	}
	else
	{
		VeConsoleInit(false);
	}

	{
		VeDatabaseServer::InitData kData;
		kData.m_kSuperData.m_kName = (*++ppcArgv);
		kData.m_kSuperData.m_kPath = (*++ppcArgv);
		kData.m_kSuperData.m_u16Port = VeAtoi(*++ppcArgv);
		kData.m_kSuperData.m_u16Max = VeAtoi(*++ppcArgv);
		kData.m_kSuperData.m_u32TimeOut = VeAtoi(*++ppcArgv);
		kData.m_kSuperData.m_kPassword = (*++ppcArgv);
		kData.m_kHost = (*++ppcArgv);
		kData.m_kUser = (*++ppcArgv);
		kData.m_kPassword = (*++ppcArgv);
		kData.m_kDatabase = (*++ppcArgv);
		kData.m_u32Port = VeAtoi(*++ppcArgv);

		g_pServerManager->StartServer(VE_NEW VeDatabaseServer(kData));
		while(g_pServerManager->GetServerNumber())
		{
			g_pTime->Update();
			g_pURL->Update();
			g_pResourceManager->Update();
			g_pServerManager->Update();
		}
	}
	
	VeConsoleTerm();
}
