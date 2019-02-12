#include "StdAfx.h"
#include "CElevatorManagementTask.h"
#include "Common.h"
#include "DB_POOL\DBConnect.h"
#include "KCMOS_Model.h"
#include "CoreLogic.h"
#include "TKDll.h"
#include "Elevator_Model.h"
#include "ElevatorCommFunc.h"

extern map<UINT,CString>    g_m_arRtn;
extern CDBConnetPool		g_db_pool;
extern CRITICAL_SECTION g_cs_TK_list;
extern CString strServer_Contrller_IP;

CElevatorManagementTask::CElevatorManagementTask(void)
{
}


CElevatorManagementTask::~CElevatorManagementTask(void)
{
}
BOOL CElevatorManagementTask::InitTask(ICommandPackage * lptask)
{
	return  true;
}
/********************************************************************
	Copyright:Kuang-chi
	创建者：	qingyao.wang
	创建日期：   2016/03/08
    方法名称：   
    参数说明：   
    描述: 
*********************************************************************/
void  CElevatorManagementTask::DoTask(void* lptask)
{
	ICommandPackage *p =(ICommandPackage*)lptask;

	switch(p->TaskID)
	{
	case C0x00600001:
		{
			EnterDoTaskLogic((Command0x00600001*)p);
			break;
		}
	case C0x00600002:
		{
			EnterCriticalSection(&g_cs_TK_list);
			EnterDoTaskLogic((Command0x00600002*)p);
			LeaveCriticalSection(&g_cs_TK_list);
			break;
		}
	case C0x00600003:
		{
			EnterDoTaskLogic((Command0x00600003*)p);
			break;
		}
	case C0x00600004:
		{
			EnterDoTaskLogic((Command0x00600004*)p);
			break;
		}
	default:
		break;
	}

	return ;
}	
/********************************************************************
	Copyright:Kuang-chi
	创建者：	qingyao.wang
	创建日期：   2016/03/08
    方法名称：   
    参数说明：   
    描述:  
*********************************************************************/
BOOL CElevatorManagementTask::EnterDoTaskLogic(Command0x00600001 const * ptask)
{
	if (NULL == ptask) return FALSE;
	try
	{
		CString    strCtrllist                      = _T("");
		char        szSearchResult[2048] = {0x00};
		KKTK_SetSearchIP(strServer_Contrller_IP);
		UINT        uRet = KKTK_SearchControls(szSearchResult, 2048);
		

		//strCtrllist    ="1|12.02.21.00.08.3D|023300002109|192.168.254.9|6000|255.255.255.0|192.168.254.1|" /*szSearchResult*/;
		strCtrllist.Format("%s",szSearchResult);
		if(strCtrllist!="")
		{
			__Command0x00600001Response(ptask,strCtrllist,_OK_);
		}
		else
		{
			__Command0x00600001Response(ptask,strCtrllist,PARAMERR);
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
		writeLog(pLogger,"EnterDoTaskLogic Command0x00600001 ERROR!",ERROR_LOG);
	}
	return FALSE;
}
void CElevatorManagementTask::__Command0x00600001Response(Command0x00600001 const * ptask,CString &strCtrllist, CErrorCode errCode)
{
	//返回给客户端
	Command0x00600001 *pReCmd = new Command0x00600001;
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
	pReCmd->strCtrllist=strCtrllist;

	CCoreLogic::GetInstance()->AddResponseTask(pReCmd);
}
/********************************************************************
	Copyright:Kuang-chi
	创建者：	qingyao.wang
	创建日期：   2016/03/08
    方法名称：   
    参数说明：   
    描述:  
*********************************************************************/
BOOL CElevatorManagementTask::EnterDoTaskLogic(Command0x00600002 const * ptask)
{
	if (NULL == ptask) return FALSE;
	try
	{
		UINT nCommID=0;
		CString strControlName = ptask->m_ContrllerName;
		CString strParam1 = ptask->m_strParam;

		CString strResult;
		switch(ptask->m_CmdType)
		{
// 		case 0:                     //修改控制器密码
// 			nCommID = KKTK_UpdatePassword((LPCSTR)strControlName.GetBuffer(0),strParam1);	 
// 			break;

		case SentTime:                     //发送时钟
			nCommID = KKTK_SendClock((LPCSTR)strControlName.GetBuffer(0));
			break;

		case InitControl:                     //系统初始化
			nCommID = KKTK_SystemInit((LPCSTR)strControlName.GetBuffer(0));
			break;

		case SentHardParam:                     //发送硬件配置参数
			//strParam1 = "3|7|8|30|0|0|20|0|0|0|0|";
			nCommID = KKTK_SendHardwareParam(strControlName, strParam1);
			break;

// 		case 4:                     //发送楼层   0   /对讲配置   1  表
// 			//strParam1 = "48|47|46|45|44|43|42|41|40|39|38|37|36|35|34|33|32|31|30|29|28|27|26|25|24|23|22|21|20|19|18|17|16|15|14|13|12|11|10|09|08|07|06|05|04|03|02|01|";
// 			nCommID = KKTK_SendCongfigTable(strControlName, 0, strParam1);
// 			break;

		case SentHoliday:						//发送节假日表
			//strParam1 = "48|47|46|45|44|43|42|41|40|39|38|37|36|35|34|33|32|31|30|29|28|27|26|25|24|23|22|21|20|19|18|17|16|15|14|13|12|11|10|9|8|7|6|5|4|3|";
			nCommID = KKTK_SendHolidayTable(strControlName, strParam1);
			break;

		case SentFloorOpenTimeSpan:						//发送楼层开放时区
			//strParam1 = "06|00|00|23|59|00|00|00|00|00|00|23|59|00|00|00|00|";
			nCommID = KKTK_SendFloorOpenTimezone(strControlName, strParam1);
			break;

		case SentElevatorOpenTimeSpan:						//发送电梯开放时区
			//strParam1 = "05|02|00|00|23|59|00|00|00|00|00|00|00|00|00|00|00|00|00|00|00|00|";
			nCommID = KKTK_SendElevatorOpenTimezone(strControlName, strParam1);
			break;

		case SentBlackWriteList:						//发送黑白名单
			{
				//strParam1 = "0@1|2|2842939630|2837002542|";
				int ntype;
				CString stype;
				CString strBlacklist;
				AfxExtractSubString(stype,strParam1,0,'@');
				ntype=atoi(stype);
				AfxExtractSubString(strBlacklist,strParam1,1,'@');
				nCommID = KKTK_SendBlacklist(strControlName, ntype, strBlacklist, FALSE);
				break;
			}
		case ReceiveTime:                    //接收时钟
			nCommID = KKTK_RecvClock((LPCSTR)strControlName.GetBuffer(0)); //0表示接收时钟
			break;

		case ReceiveHardParam: 
			{//接收配置参数

			//#define     ALARMCOUNT				0x31       //报警剩余次数
			//#define     ELEVATORPARAM				0x32       //电梯上行、下行参数
			//#define     FLOORSUM					0x33       //楼层总数
			//#define     CKACTTIME					0x34       //层控继电器动作时间
			//#define     ZDACTTIME					0x35       //直达继电器动作时间
			//#define     DJACTTIME					0x36       //对讲继电器动作时间
			//UINT nCommID1 = 0,nCommID2 = 0;
			int ntype = atoi(strParam1);
			nCommID = KKTK_RecvParam((LPCSTR)strControlName.GetBuffer(0),ntype);
			//nCommID1 = KKTK_RecvParam((LPCSTR)strControlName.GetBuffer(0),0x36);
			//nCommID2 = KKTK_RecvParam((LPCSTR)strControlName.GetBuffer(0),0x36);
			break;
			}

// 		case 11:                    //接收配置表
// 			nCommID = KKTK_RecvConfigTbale((LPCSTR)strControlName.GetBuffer(0), 0); 
// 			break;
// 
// 		case 12:                    //接收状态
// 			//		nCommID = KKTK_RecvStatus((LPCSTR)strControlName.GetBuffer(0)); 
// 			break;

		case ReceiveControlCommucateParam:                    //接收通信参数
			nCommID = KKTK_RecvControlCommParam((LPCSTR)strControlName.GetBuffer(0));
			break;

		case SentControlCommucateParam:     
			{//发送通信参数
			//strParam1.Format(_T("%d|%s|%s|%d|%s|%s|%s|"), dlg7.m_nAddress, dlg7.m_strMac,dlg7.m_strIP, dlg7.m_nPort, dlg7.m_strSubMask, dlg7.m_strGate,dlg7.m_strSerial);
		//	nCommID = KKTK_SendControlCommParam((LPCSTR)strControlName.GetBuffer(0), (LPCSTR)strParam1.GetBuffer(0), FALSE);
			//	1|12.02.21.00.07.E0|023300002016|192.168.3.172|6000|0.0.0.0|192.168.3.251|
			//CString ttt ="1|1|192.168.254.38|6000|255.255.255.0|192.168.254.254|12.02.21.00.07.E0|";
		   // printf("%s\n",strParam1);
			//EnterCriticalSection(&g_cs_TK_list);
 			nCommID = KKTK_UpdateControl((LPCSTR)strParam1.GetBuffer(0));
			//LeaveCriticalSection(&g_cs_TK_list);
			}
			break;

		case ReceiveControlVision:                    //接收版本
			nCommID = KKTK_RecvVer((LPCSTR)strControlName.GetBuffer(0));
			break;
// 		case 16:
// 			//strParam1.Format(_T("%d|%s|%s|%d|%s|%s|%s|"), 1, "023400001234","192.168.3.12", 6000, "255.255.255.0", "192.168.3.251",dlg8.m_strSerial);
// 			nCommID = KKTK_SendControlCommParam((LPCSTR)strControlName.GetBuffer(0), (LPCSTR)strParam1.GetBuffer(0), TRUE);
// 			break;
		case SentAddControlLoad:         //发送添加授权 17
			nCommID = KKTK_SendAddID((LPCSTR)strControlName.GetBuffer(0),(LPCSTR)strParam1.GetBuffer(0));
			break;


		case SentDeleteControlLoad:                    //发送删除授权
			nCommID = KKTK_SendDelID((LPCSTR)strControlName.GetBuffer(0),(LPCSTR)strParam1.GetBuffer(0));
			break;

		case ReceiveRecord:                    //接收记录
			nCommID = KKTK_RecvVer((LPCSTR)strControlName.GetBuffer(0));
			break;


		case SentDeploy:
			{
				int ntype;
				CString stype;
				CString strParam;
				AfxExtractSubString(stype,strParam1,0,'@');
				ntype=atoi(stype);
				AfxExtractSubString(strParam,strParam1,1,'@');
				nCommID = KKTK_SendCongfigTable(strControlName, ntype, strParam);
				break;
			}
			

		case ReceiveDeploy:
			nCommID = KKTK_RecvConfigTbale((LPCSTR)strControlName.GetBuffer(0), 0);
			break;

		}

		UINT nRtn=0;
		if (ptask->m_CmdType!=SentControlCommucateParam && nCommID>=0x00000001 && nCommID<=0x7fffffff)
		{
			strResult = GetCommResult(nCommID,nRtn);
		}
		else if(ptask->m_CmdType==SentControlCommucateParam)
		{
			strResult = "";
			nRtn=0x00;
			CString strLog;
			strLog.Format(_T("CommID=0x%08x %s %s \r\n"), nCommID, "", strResult);
			printf(strLog);
		}
		else
		{
			strResult = g_m_arRtn.at(nCommID);
			nRtn=nCommID;
			CString strLog;
			strLog.Format(_T("CommID=0x%08x %s %s \r\n"), nCommID, "", strResult);
			printf(strLog);
		}



		//返回给客户端
		Command0x00600002 *pReCmd = new Command0x00600002;
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
		pReCmd->m_CmdType = ptask->m_CmdType;
		pReCmd->m_ContrllerName = ptask->m_ContrllerName;
		pReCmd->m_nResultCode = nRtn;
		pReCmd->m_strResult = strResult;

		CCoreLogic::GetInstance()->AddResponseTask(pReCmd);

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
		writeLog(pLogger,"EnterDoTaskLogic Command0x00600002 ERROR!",ERROR_LOG);
	}
	return FALSE;
}

void CElevatorManagementTask::__Command0x00600002Response(Command0x00600002 const * ptask,UCHAR Param1[] ,UCHAR Param2[] ,CErrorCode errCode)
{
	//返回给客户端
	Command0x00600002 *pReCmd = new Command0x00600002;
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
}

/********************************************************************
	Copyright:Kuang-chi
	创建者：	qingyao.wang
	创建日期：   2016/03/08
    方法名称：   
    参数说明：   
    描述:  
*********************************************************************/
BOOL CElevatorManagementTask::EnterDoTaskLogic(Command0x00600003 const * ptask)
{
	if (NULL == ptask) return FALSE;
	try
	{
		CElevatorCommFunc::GetInstance()->LoadCtrollerNameList();
		__Command0x00600003Response(ptask,_OK_);

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
		writeLog(pLogger,"EnterDoTaskLogic Command0x00600003 ERROR!",ERROR_LOG);
	}
	return FALSE;
}

BOOL CElevatorManagementTask::EnterDoTaskLogic(Command0x00600004 const * ptask)
{
	if (NULL == ptask) return FALSE;
	try
	{
		
		CElevatorCommFunc::GetInstance()->setAuthorizationevent() ;

		__Command0x00600004Response(ptask,_OK_);
		
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
		writeLog(pLogger,"EnterDoTaskLogic Command0x00600004 ERROR!",ERROR_LOG);
	}
	return FALSE;
}

void CElevatorManagementTask::__Command0x00600003Response(Command0x00600003 const * ptask,CErrorCode errCode)
{
	//返回给客户端
	Command0x00600003 *pReCmd = new Command0x00600003;
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
}
void CElevatorManagementTask::__Command0x00600004Response(Command0x00600004 const * ptask,CErrorCode errCode)
{
	//返回给客户端
	Command0x00600004 *pReCmd = new Command0x00600004;
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
}
