#include "StdAfx.h"
#include "CAuthorityManagementTask.h"
#include "Command\Command0x0014xxxx.h"
#include "Common.h"
#include "DB_POOL\ADODB.h"
#include "DB_POOL\DBConnect.h"
#include "CoreLogic.h"
#include "JsonOperator_Authority.h"
#include "RightsManagement.h"
#include "KCMOS_Model.h"
#include <list>
#include "storedProcedure.h"


extern CDBConnetPool		g_db_pool;
extern CRITICAL_SECTION g_critical_sendtaskqueue;//响应任务队列 临界锁
extern std::queue<ICommandPackage*> g_sendtaskqueue;//响应任务队列；
extern HANDLE g_AuthorizationRuningEvent;
extern int 	  g_AuthorizationdownloadAllflag;//授权所有

extern HANDLE g_AuthorizationRuningEvent_KW;
extern int 	  g_AuthorizationdownloadAllflag_KW;//授权所有
/////////////////////////////////////////
CAuthorityManagementTask::CAuthorityManagementTask(void)
{

}
CAuthorityManagementTask::~CAuthorityManagementTask(void)
{

}

BOOL CAuthorityManagementTask::InitTask(ICommandPackage * lptask)
{
	return  true;
}
void  CAuthorityManagementTask::DoTask(void* lptask)
{
	ICommandPackage *p =(ICommandPackage*)lptask;

	switch(p->TaskID)
	{
	case C0x00140001:
		{
			EnterDoTaskLogic((Command0x00140001*)p);
		}
		break;
	case C0x00140002:
		{
			EnterDoTaskLogic((Command0x00140002*)p);
		}
		break;
	case C0x00140003:
		{
			EnterDoTaskLogic((Command0x00140003*)p);
		}
		break;
	case C0x00140004:
		{
			EnterDoTaskLogic((Command0x00140004*)p);
		}
		break;
	case C0x00140005:
		{
			EnterDoTaskLogic((Command0x00140005*)p);
		}
		break;
	default:
		break;
	}

	return ;
}
/********************************************************************
	Copyright:Kuang-chi
	创建者：	qingyao.wang
	创建日期：   2016/01/7
    方法名称：   EnterDoTaskLogic
    参数说明：   
    描述: 处理Command0x00140001命令逻辑
*********************************************************************/
BOOL CAuthorityManagementTask::EnterDoTaskLogic(Command0x00140001 const * ptask)
{
	try
	{
		//解析json
		AuthorityData * p_AuthorityData;
		if (CJsonOperatorAuthority::GetInstance()->parseJson( const_cast<CString&>(ptask->m_strJson),(DataType)ptask->m_DataType,&p_AuthorityData))
		{
			p_AuthorityData->m_dataType = (DataType)ptask->m_DataType;
			switch(ptask->m_DataType)
			{
			case OrgPermission:
				{//保存数据库
// 					COrgPermission * p = (COrgPermission*)p_AuthorityData;
// 					vector<tag_st_door>::iterator it;
// 					for (it = p->m_v_door.begin();it!=p->m_v_door.end();it ++)
// 					{
// 						p->m_ID = CStoredProcedure::GetInstance()->Insert_Update_Authority_Org(p->m_ID,p->m_DptID,p->m_StartTime,p->m_EndTime,it->doorID,it->uTimeBucket,storedpro_add);
// 					}
					break;
				}
			case DoorGroup:
				{
					CDoorGroup * p = (CDoorGroup*)p_AuthorityData;
					p->m_ID = CStoredProcedure::GetInstance()->Insert_Update_GateGroup(p->m_ID,p->m_Name,p->m_sJson,storedpro_add);
					break;
				}
//			case DoorPermission:
//				{
// 					CDoorPermission * p = (CDoorPermission*)p_AuthorityData;
// 					CStoredProcedure::GetInstance()->execPro(p->m_ID,p->m_PersonID,p->m_);
// 					break;
//				}
			default:
				break;
			}
		}

		if (p_AuthorityData->m_ID>0)
			__Command0x00140001Response(ptask,p_AuthorityData,_OK_);
		else
			__Command0x00140001Response(ptask,p_AuthorityData,AUTHORITYDATAADDFAILURE);
		

		return TRUE;
	}
	catch (CMemoryException* e)
	{
		e->ReportError();
		writeLog(pLogger,"MemoryException ERROR!",ERROR_LOG);
	}
	catch (CFileException* e)
	{
		char szError[1024] = {0};
		e->GetErrorMessage(szError,1024);
		writeLog(pLogger,szError,ERROR_LOG);
	}
	catch (...)
	{
		writeLog(pLogger,"EnterDoTaskLogic Command0x00140001 ERROR!",ERROR_LOG);
	}
	
	return FALSE;
}
void CAuthorityManagementTask::__Command0x00140001Response(Command0x00140001 const * ptask,AuthorityData* p_AuthorityData,CErrorCode errCode)
{
	//响应客户端并返回授权记录ID
	Command0x00140001 *pReCmd = new Command0x00140001;
	//head
	pReCmd->m_packhead.Head = ptask->m_packhead.Head;
	pReCmd->m_packhead.Version = ptask->m_packhead.Version;
	pReCmd->m_packhead.DateNO = 0;
	pReCmd->m_packhead.WorkNO = ptask->m_packhead.WorkNO;
	pReCmd->m_packhead.FuncCode = ptask->m_packhead.FuncCode + 0x00010000;
	pReCmd->m_packhead.ErrorCode = errCode;
	//DataLen
	pReCmd->m_packhead.PackageDataLen=0;
	//
	pReCmd->IP = ptask->IP;
	pReCmd->port = ptask->port;
	pReCmd->TaskID = ptask->TaskID;
	pReCmd->MainCmd =ptask->MainCmd;
	//
	pReCmd->m_DataType = ptask->m_DataType;
	if (errCode==_OK_)
		pReCmd->m_AuthorityData = p_AuthorityData;
	else
		pReCmd->m_AuthorityData = NULL;
	

	CCoreLogic::GetInstance()->AddResponseTask(pReCmd);

	//通知其他客户端
	if (errCode==_OK_)
		_NoticeClient(ptask,p_AuthorityData);

}
void CAuthorityManagementTask::_NoticeClient(Command0x00140001 const * ptask,AuthorityData* p_AuthorityData)
{

	if ( ptask == NULL) return;
	try
	{
		map<CString,clientaddinfo>::iterator it;
		map<CString,clientaddinfo> map_online;
		map_online = CCoreLogic::GetInstance()->getClientOnline();
		for (it = map_online.begin();it != map_online.end();it ++)
		{
			Command0x0014F001 *pReCmd = new Command0x0014F001;
			//head
			pReCmd->m_packhead.Head = ptask->m_packhead.Head;
			pReCmd->m_packhead.Version = ptask->m_packhead.Version;
			pReCmd->m_packhead.DateNO = 0;
			pReCmd->m_packhead.WorkNO = -1;
			pReCmd->m_packhead.FuncCode = ptask->m_packhead.FuncCode + 0x0001F000;
			pReCmd->m_packhead.ErrorCode = _OK_;
			//DataLen
			pReCmd->m_packhead.PackageDataLen=0;
			//
			pReCmd->IP = it->second.IP;
			pReCmd->port = it->second.port;
			pReCmd->TaskID = ptask->TaskID;
			pReCmd->MainCmd =ptask->MainCmd;
			//
			pReCmd->m_DataType = ptask->m_DataType;
			pReCmd->m_AuthorityData = p_AuthorityData;

			CCoreLogic::GetInstance()->AddResponseTask(pReCmd);
		}
	}
	catch (...)
	{
		writeLog(pLogger,"Command0x00140001 通知异常",ERROR_LOG);
	}
}
/********************************************************************
	Copyright:Kuang-chi
	创建者：	qingyao.wang
	创建日期：   2016/01/7
    方法名称：   
    参数说明：   
    描述: 处理00020002命令逻辑  
*********************************************************************/
BOOL CAuthorityManagementTask::EnterDoTaskLogic(Command0x00140002 const * ptask)
{
	try
	{
		int nRet=-1;
		nRet = CStoredProcedure::GetInstance()->delete_T_AuthorityObject(ptask->m_ID,(DataType)ptask->m_DataType);

		if (nRet>0)
			__Command0x00140002Response(ptask,_OK_);
		else
			__Command0x00140002Response(ptask,DELETEAUTHORITYDATAFAILURE);

		return TRUE;
	}
	catch (CMemoryException* e)
	{
		e->ReportError();
		writeLog(pLogger,"MemoryException ERROR!",ERROR_LOG);
	}
	catch (CFileException* e)
	{
		char szError[1024] = {0};
		e->GetErrorMessage(szError,1024);
		writeLog(pLogger,szError,ERROR_LOG);
	}
	catch (...)
	{
		writeLog(pLogger,"EnterDoTaskLogic Command0x00140002 ERROR!",ERROR_LOG);
	}

	return FALSE;
}
void CAuthorityManagementTask::__Command0x00140002Response(Command0x00140002 const * ptask,CErrorCode errCode)
{
	Command0x00140002 *pReCmd = new Command0x00140002;
	//head
	pReCmd->m_packhead.Head = ptask->m_packhead.Head;
	pReCmd->m_packhead.Version = ptask->m_packhead.Version;
	pReCmd->m_packhead.DateNO = 0;
	pReCmd->m_packhead.WorkNO = ptask->m_packhead.WorkNO;
	pReCmd->m_packhead.FuncCode = ptask->m_packhead.FuncCode + 0x00010000;
	pReCmd->m_packhead.ErrorCode = errCode;
	//DataLen
	pReCmd->m_packhead.PackageDataLen=0;
	//
	pReCmd->IP = ptask->IP;
	pReCmd->port = ptask->port;
	pReCmd->TaskID = ptask->TaskID;
	pReCmd->MainCmd =ptask->MainCmd;
	//
	pReCmd->m_DataType = ptask->m_DataType;
	pReCmd->m_ID = ptask->m_ID;

	CCoreLogic::GetInstance()->AddResponseTask(pReCmd);

	//通知其他客户端
	if (errCode==_OK_)
		_NoticeClient_Delete(ptask);

}
void CAuthorityManagementTask::_NoticeClient_Delete(Command0x00140002 const * ptask)
{

	if ( ptask == NULL) return;
	try
	{
		map<CString,clientaddinfo>::iterator it;
		map<CString,clientaddinfo> map_online;
		map_online = CCoreLogic::GetInstance()->getClientOnline();
		for (it = map_online.begin();it != map_online.end();it ++)
		{
			Command0x0014F002 *pReCmd = new Command0x0014F002;
			//head
			pReCmd->m_packhead.Head = ptask->m_packhead.Head;
			pReCmd->m_packhead.Version = ptask->m_packhead.Version;
			pReCmd->m_packhead.DateNO = 0;
			pReCmd->m_packhead.WorkNO = -1;
			pReCmd->m_packhead.FuncCode = ptask->m_packhead.FuncCode + 0x0001F000;
			pReCmd->m_packhead.ErrorCode = _OK_;
			//DataLen
			pReCmd->m_packhead.PackageDataLen=0;
			//
			pReCmd->IP = it->second.IP;
			pReCmd->port = it->second.port;
			pReCmd->TaskID = ptask->TaskID;
			pReCmd->MainCmd =ptask->MainCmd;
			//
			pReCmd->m_DataType = ptask->m_DataType;
			pReCmd->m_ID = ptask->m_ID;

			CCoreLogic::GetInstance()->AddResponseTask(pReCmd);
		}
	}
	catch (...)
	{
		writeLog(pLogger,"Command0x0014F002 通知异常",ERROR_LOG);
	}
}
/********************************************************************
	Copyright:Kuang-chi
	创建者：	qingyao.wang
	创建日期：   2016/01/7
    方法名称：   
    参数说明：   
    描述: 处理00020003命令逻辑  

*********************************************************************/
BOOL CAuthorityManagementTask::EnterDoTaskLogic(Command0x00140003 const * ptask)
{
	try
	{
		//解析json
		AuthorityData * p_AuthorityData;
		if (CJsonOperatorAuthority::GetInstance()->parseJson( const_cast<CString&>(ptask->m_strJson),(DataType)ptask->m_DataType,&p_AuthorityData))
		{
			p_AuthorityData->m_dataType = (DataType)ptask->m_DataType;
			switch(ptask->m_DataType)
			{
			case OrgPermission:
				{//保存数据库
				//	COrgPermission * p = (COrgPermission*)p_AuthorityData;
					
				//	p->m_ID = CStoredProcedure::GetInstance()->Insert_Update_Authority_Org(p->m_ID,p->m_DptID,p->m_StartTime,p->m_EndTime,it->doorID,it->uTimeBucket,storedpro_update);
					break;
				}
			case DoorGroup:
				{
					CDoorGroup * p = (CDoorGroup*)p_AuthorityData;
					p->m_ID = CStoredProcedure::GetInstance()->Insert_Update_GateGroup(p->m_ID,p->m_Name,p->m_sJson,storedpro_update);
					break;
				}
			case DoorPermission:
				{
					CDoorPermission * p = (CDoorPermission*)p_AuthorityData;
					vector<tag_st_door>::iterator it;
					for (it=p->m_v_door.begin();it!=p->m_v_door.end();it++)
					{
// 						if (it->bFlag)
// 						{
// 						}
// 						CStoredProcedure::GetInstance()->execPro(it->PersonID,it->StartTime,it->EndTime,it->doorID,it->uTimeBucket);
					}
					
					break;
				}
			default:
				break;
			}
		}

		if (p_AuthorityData->m_ID > 0)
			__Command0x00140003Response(ptask,p_AuthorityData,_OK_);
		else
			__Command0x00140003Response(ptask,p_AuthorityData,UPDATEAUTHORITYDATAFAILURE);

		return TRUE;
	}
	catch (CMemoryException* e)
	{
		e->ReportError();
		writeLog(pLogger,"MemoryException ERROR!",ERROR_LOG);
	}
	catch (CFileException* e)
	{
		char szError[1024] = {0};
		e->GetErrorMessage(szError,1024);
		writeLog(pLogger,szError,ERROR_LOG);
	}
	catch (...)
	{
		writeLog(pLogger,"EnterDoTaskLogic Command0x00140003 ERROR!",ERROR_LOG);
	}

	return FALSE;
}
void CAuthorityManagementTask::__Command0x00140003Response(Command0x00140003 const * ptask,AuthorityData* p_AuthorityData,CErrorCode errCode)
{
	Command0x00140003 *pReCmd = new Command0x00140003;
	//head
	pReCmd->m_packhead.Head = ptask->m_packhead.Head;
	pReCmd->m_packhead.Version = ptask->m_packhead.Version;
	pReCmd->m_packhead.DateNO = 0;
	pReCmd->m_packhead.WorkNO = ptask->m_packhead.WorkNO;
	pReCmd->m_packhead.FuncCode = ptask->m_packhead.FuncCode + 0x00010000;
	pReCmd->m_packhead.ErrorCode = errCode;
	//DataLen
	pReCmd->m_packhead.PackageDataLen=0;
	//
	pReCmd->IP = ptask->IP;
	pReCmd->port = ptask->port;
	pReCmd->TaskID = ptask->TaskID;
	pReCmd->MainCmd =ptask->MainCmd;
	//
	pReCmd->m_DataType = ptask->m_DataType;
	if (errCode==_OK_)
		pReCmd->m_AuthorityData = p_AuthorityData;
	else
		pReCmd->m_AuthorityData = NULL;


	CCoreLogic::GetInstance()->AddResponseTask(pReCmd);

	//通知其他客户端
	if (errCode==_OK_)
		_NoticeClient_Update(ptask,p_AuthorityData);

}
void CAuthorityManagementTask::_NoticeClient_Update(Command0x00140003 const * ptask,AuthorityData* p_AuthorityData)
{

	if ( ptask == NULL) return;
	try
	{
		map<CString,clientaddinfo>::iterator it;
		map<CString,clientaddinfo> map_online;
		map_online = CCoreLogic::GetInstance()->getClientOnline();
		for (it = map_online.begin();it != map_online.end();it ++)
		{
			Command0x0014F003 *pReCmd = new Command0x0014F003;
			//head
			pReCmd->m_packhead.Head = ptask->m_packhead.Head;
			pReCmd->m_packhead.Version = ptask->m_packhead.Version;
			pReCmd->m_packhead.DateNO = 0;
			pReCmd->m_packhead.WorkNO = -1;
			pReCmd->m_packhead.FuncCode = ptask->m_packhead.FuncCode + 0x0001F000;
			pReCmd->m_packhead.ErrorCode = _OK_;
			//DataLen
			pReCmd->m_packhead.PackageDataLen=0;
			//
			pReCmd->IP = ptask->IP;
			pReCmd->port = ptask->port;
			pReCmd->TaskID = ptask->TaskID;
			pReCmd->MainCmd =ptask->MainCmd;
			//
			pReCmd->m_DataType = ptask->m_DataType;
			pReCmd->m_AuthorityData = p_AuthorityData;

			CCoreLogic::GetInstance()->AddResponseTask(pReCmd);
		}
	}
	catch (...)
	{
		writeLog(pLogger,"Command0x00140003 通知异常",ERROR_LOG);
	}
}
/********************************************************************
	Copyright:Kuang-chi
	创建者：	qingyao.wang
	创建日期：  2016/01/7
    方法名称：   
    参数说明：   
    描述: 处理00020004命令逻辑 
*********************************************************************/
BOOL CAuthorityManagementTask::EnterDoTaskLogic(Command0x00140004 const * ptask)
{
	try
	{
		vector<AuthorityData*> p_AuthorityData;
		try
		{
			CString strsql ="";
			strsql.Format("EXEC [Select_GateGroup] 0;");
			CDBConnect* lpcon_pool=g_db_pool.GetCon(TRUE);
			TAGDBPOOL tmpdbpool(&g_db_pool,lpcon_pool);
			CADORecordset& lprec_pool = lpcon_pool->OpenSql(strsql);
			lprec_pool.MoveFirst();
			while(!lprec_pool.IsEOF())
			{
				CDoorGroup* p = new CDoorGroup;
				lprec_pool.GetFieldValue("ID",p->m_ID);
				lprec_pool.GetFieldValue("Name",p->m_Name);
				lprec_pool.GetFieldValue("DoorIDList",p->m_sJson);
				p_AuthorityData.push_back(p);

				lprec_pool.MoveNext();
			}
		}
		catch(...)
		{
			writeLog(pLogger,"EnterDoTaskLogic Command0x00140004 ERROR!",ERROR_LOG);
		}

		//
		Command0x00140004 *pReCmd = new Command0x00140004;
		//head
		pReCmd->m_packhead.Head = ptask->m_packhead.Head;
		pReCmd->m_packhead.Version = ptask->m_packhead.Version;
		pReCmd->m_packhead.DateNO = 0;
		pReCmd->m_packhead.WorkNO = ptask->m_packhead.WorkNO;
		pReCmd->m_packhead.FuncCode = ptask->m_packhead.FuncCode + 0x00010000;
		pReCmd->m_packhead.ErrorCode = _OK_;
		//DataLen
		pReCmd->m_packhead.PackageDataLen=0;
		//
		pReCmd->IP = ptask->IP;
		pReCmd->port = ptask->port;
		pReCmd->TaskID = ptask->TaskID;
		pReCmd->MainCmd =ptask->MainCmd;
		//
		pReCmd->m_DataType = ptask->m_DataType;
		pReCmd->m_v_AuthorityData = p_AuthorityData;

		CCoreLogic::GetInstance()->AddResponseTask(pReCmd);

 		Sleep(2000);
 		//清理数据
		vector<AuthorityData*>::iterator it;
		for (it=p_AuthorityData.begin();it!=p_AuthorityData.end();it++)
		{
			DELETE_T(*it);
		}

		return TRUE;
	}
	catch (CMemoryException* e)
	{
		e->ReportError();
		writeLog(pLogger,"MemoryException ERROR!",ERROR_LOG);
	}
	catch (CFileException* e)
	{
		char szError[1024] = {0};
		e->GetErrorMessage(szError,1024);
		writeLog(pLogger,szError,ERROR_LOG);
	}
	catch (...)
	{
		writeLog(pLogger,"EnterDoTaskLogic Command0x00140004 ERROR!",ERROR_LOG);
	}

	return FALSE;
}
//触发下载
BOOL CAuthorityManagementTask::EnterDoTaskLogic(Command0x00140005 const * ptask)
{
	try
	{
		g_AuthorizationdownloadAllflag=ptask->m_downloadAllflag;//0下载未下载，1下载所有
		SetEvent(g_AuthorizationRuningEvent);

		__Command0x00140005Response(ptask,_OK_);

		return TRUE;
	}
	catch (CMemoryException* e)
	{
		e->ReportError();
		writeLog(pLogger,"MemoryException ERROR!",ERROR_LOG);
	}
	catch (CFileException* e)
	{
		char szError[1024] = {0};
		e->GetErrorMessage(szError,1024);
		writeLog(pLogger,szError,ERROR_LOG);
	}
	catch (...)
	{
		writeLog(pLogger,"EnterDoTaskLogic Command0x00140005 ERROR!",ERROR_LOG);
	}

	return FALSE;
}
void CAuthorityManagementTask::__Command0x00140005Response(Command0x00140005 const * ptask,CErrorCode errCode)
{
	Command0x00140005 *pReCmd = new Command0x00140005;
	//head
	pReCmd->m_packhead.Head = ptask->m_packhead.Head;
	pReCmd->m_packhead.Version = ptask->m_packhead.Version;
	pReCmd->m_packhead.DateNO = 0;
	pReCmd->m_packhead.WorkNO = ptask->m_packhead.WorkNO;
	pReCmd->m_packhead.FuncCode = ptask->m_packhead.FuncCode + 0x00010000;
	pReCmd->m_packhead.ErrorCode = errCode;
	//DataLen
	pReCmd->m_packhead.PackageDataLen=0;
	//
	pReCmd->IP = ptask->IP;
	pReCmd->port = ptask->port;
	pReCmd->TaskID = ptask->TaskID;
	pReCmd->MainCmd =ptask->MainCmd;
	//

	CCoreLogic::GetInstance()->AddResponseTask(pReCmd);

	//通知其他客户端
	if (errCode==_OK_)
		_NoticeClient_Download(ptask);

}
void CAuthorityManagementTask::_NoticeClient_Download(Command0x00140005 const * ptask)
{

	if ( ptask == NULL) return;
	try
	{
		map<CString,clientaddinfo>::iterator it;
		map<CString,clientaddinfo> map_online;
		map_online = CCoreLogic::GetInstance()->getClientOnline();
		for (it = map_online.begin();it != map_online.end();it ++)
		{
			Command0x0014F005 *pReCmd = new Command0x0014F005;
			//head
			pReCmd->m_packhead.Head = ptask->m_packhead.Head;
			pReCmd->m_packhead.Version = ptask->m_packhead.Version;
			pReCmd->m_packhead.DateNO = 0;
			pReCmd->m_packhead.WorkNO = -1;
			pReCmd->m_packhead.FuncCode = ptask->m_packhead.FuncCode + 0x0001F000;
			pReCmd->m_packhead.ErrorCode = _OK_;
			//DataLen
			pReCmd->m_packhead.PackageDataLen=0;
			//
			pReCmd->IP = ptask->IP;
			pReCmd->port = ptask->port;
			pReCmd->TaskID = ptask->TaskID;
			pReCmd->MainCmd =ptask->MainCmd;
			//
			
			CCoreLogic::GetInstance()->AddResponseTask(pReCmd);
		}
	}
	catch (...)
	{
		writeLog(pLogger,"Command0x00140005 通知异常",ERROR_LOG);
	}
}




/*
void CAuthorityManagementTask::getDoorList(AuthorityObj* pAuthorityObj)
{
	
	if (pAuthorityObj==NULL) return;

	try
	{
		CString strsql;
		strsql.Format("SELECT doorID,doorStartTime,doorEndTime,TimeBucket,bFlag  FROM T_Base_Authority where PersonID=%d;",pAuthorityObj->m_nID);
		CDBConnect* lpcon_pool=g_db_pool.GetCon(TRUE);
		TAGDBPOOL tmpdbpool(&g_db_pool,lpcon_pool);
		CADORecordset& lprec_pool = lpcon_pool->OpenSql(strsql);
	
		while(!lprec_pool.IsEOF())
		{
			int ID=0;
			_variant_t val;
			tag_st_door st;
			CString StartTime,EndTime;
			lprec_pool.GetFieldValue("doorID",st.doorID);
			lprec_pool.GetFieldValue("doorStartTime",StartTime);
			lprec_pool.GetFieldValue("doorEndTime",EndTime);
			lprec_pool.GetFieldValue("TimeBucket",st.uTimeBucket);
			lprec_pool.GetFieldValue("bFlag",st.bFlag);
			st.m_StartTime = StartTime.Trim();
			st.m_EndTime = EndTime.Trim();
			pAuthorityObj->m_v_door.push_back(st);
			lprec_pool.MoveNext();
		}
	}
	catch (...)
	{
		writeLog(pLogger,"CAuthorityManagementTask::getDoorList Err",ERROR_LOG);
	}
}*/