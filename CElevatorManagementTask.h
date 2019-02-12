#pragma once
#include "threadtask.h"
#include "Command\Command0x0060xxxx.h"
#include "KCMOS_Model.h"


class CElevatorManagementTask : public ThreadTask
{
public:
	CElevatorManagementTask(void);
	virtual ~CElevatorManagementTask(void);

	void DoTask(void* lptask);
	BOOL InitTask(ICommandPackage* const task);

private:
	BOOL EnterDoTaskLogic(Command0x00600001 const * ptask);
	BOOL EnterDoTaskLogic(Command0x00600002 const * ptask);
	BOOL EnterDoTaskLogic(Command0x00600003 const * ptask);
	BOOL EnterDoTaskLogic(Command0x00600004 const * ptask);

private:
	void __Command0x00600001Response(Command0x00600001 const * ptask,CString &strCtrllist,CErrorCode errCode);
	void __Command0x00600002Response(Command0x00600002 const * ptask,UCHAR Param1[] ,UCHAR Param2[] ,CErrorCode errCode);
	void __Command0x00600003Response(Command0x00600003 const * ptask,CErrorCode errCode);
	void __Command0x00600004Response(Command0x00600004 const * ptask,CErrorCode errCode);

};

