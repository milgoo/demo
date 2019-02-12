#pragma once
#include "ThreadTask.h"
#include "Command\CProtocolMsgPack.h"
#include "Command\Command0x0014xxxx.h"


class CAuthorityManagementTask : public ThreadTask
{
public:
	void DoTask(void* lptask);

	CAuthorityManagementTask(void);
	virtual ~CAuthorityManagementTask(void);

	BOOL InitTask(ICommandPackage* const task);

private:
	BOOL EnterDoTaskLogic(Command0x00140001 const * ptask);
	BOOL EnterDoTaskLogic(Command0x00140002 const * ptask);
	BOOL EnterDoTaskLogic(Command0x00140003 const * ptask);
	BOOL EnterDoTaskLogic(Command0x00140004 const * ptask);
	BOOL EnterDoTaskLogic(Command0x00140005 const * ptask);

	void _NoticeClient(Command0x00140001 const * ptask,AuthorityData* p_AuthorityData);
	void _NoticeClient_Delete(Command0x00140002 const * ptask);
	void _NoticeClient_Update(Command0x00140003 const * ptask,AuthorityData* p_AuthorityData);
	void _NoticeClient_Download(Command0x00140005 const * ptask);

	void __Command0x00140001Response(Command0x00140001 const * ptask,AuthorityData* p_AuthorityData,CErrorCode errCode);
	void __Command0x00140002Response(Command0x00140002 const * ptask,CErrorCode errCode);
	void __Command0x00140003Response(Command0x00140003 const * ptask,AuthorityData* p_AuthorityData,CErrorCode errCode);
	void __Command0x00140005Response(Command0x00140005 const * ptask,CErrorCode errCode);
//	void getDoorList(AuthorityObj* pAuthorityObj);
};

