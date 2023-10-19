#pragma once

#include <winternl.h>

typedef struct _SYSTEM_PROCESS_INFO
{
    SYSTEM_PROCESS_INFORMATION pi;
    SYSTEM_THREAD_INFORMATION ti[1];
} SYSTEM_PROCESS_INFO, * PSYSTEM_PROCESS_INFO;


typedef enum
{
	ThreadStateInitialized,
	ThreadStateReady,
	ThreadStateRunning,
	ThreadStateStandby,
	ThreadStateTerminated,
	ThreadStateWaiting,
	ThreadStateTransition,
	ThreadStateDeferredReady
} THREAD_STATE;

const WCHAR* ThreadStateValueNames[] =
{
  L"Initialized",
  L"Ready",
  L"Running",
  L"Standby",
  L"Terminated",
  L"Waiting",
  L"Transition",
  L"DeferredReady"
};


typedef enum
{
	ThreadWaitReasonExecutive,
	ThreadWaitReasonFreePage,
	ThreadWaitReasonPageIn,
	ThreadWaitReasonPoolAllocation,
	ThreadWaitReasonDelayExecution,
	ThreadWaitReasonSuspended,
	ThreadWaitReasonUserRequest,
	ThreadWaitReasonWrExecutive,
	ThreadWaitReasonWrFreePage,
	ThreadWaitReasonWrPageIn,
	ThreadWaitReasonWrPoolAllocation,
	ThreadWaitReasonWrDelayExecution,
	ThreadWaitReasonWrSuspended,
	ThreadWaitReasonWrUserRequest,
	ThreadWaitReasonWrEventPair,
	ThreadWaitReasonWrQueue,
	ThreadWaitReasonWrLpcReceive,
	ThreadWaitReasonWrLpcReply,
	ThreadWaitReasonWrVirtualMemory,
	ThreadWaitReasonWrPageOut,
	ThreadWaitReasonWrRendezvous,
	ThreadWaitReasonWrKeyedEvent,
	ThreadWaitReasonWrTerminated,
	ThreadWaitReasonWrProcessInSwap,
	ThreadWaitReasonWrCpuRateControl,
	ThreadWaitReasonWrCalloutStack,
	ThreadWaitReasonWrKernel,
	ThreadWaitReasonWrResource,
	ThreadWaitReasonWrPushLock,
	ThreadWaitReasonWrMutex,
	ThreadWaitReasonWrQuantumEnd,
	ThreadWaitReasonWrDispatchInt,
	ThreadWaitReasonWrPreempted,
	ThreadWaitReasonWrYieldExecution,
	ThreadWaitReasonWrFastMutex,
	ThreadWaitReasonWrGuardedMutex,
	ThreadWaitReasonWrRundown,
	ThreadWaitReasonMaximumWaitReason
} THREAD_WAIT_REASON;

const WCHAR* ThreadWaitReasonValueNames[] =
{
	L"Executive",
	L"FreePage",
	L"PageIn",
	L"PoolAllocation",
	L"DelayExecution",
	L"Suspended",
	L"UserRequest",
	L"WrExecutive ",
	L"WrFreePage",
	L"WrPageIn",
	L"WrPoolAllocation",
	L"WrDelayExecution",
	L"WrSuspended",
	L"WrUserRequest",
	L"WrEventPair",
	L"WrQueue",
	L"WrLpcReceive",
	L"WrLpcReply",
	L"WrVirtualMemory",
	L"WrPageOut",
	L"WrRendezvous",
	L"WrKeyedEvent",
	L"WrTerminated",
	L"WrProcessInSwap",
	L"WrCpuRateControl",
	L"WrCalloutStack",
	L"WrKernel",
	L"WrResource",
	L"WrPushLock",
	L"WrMutex",
	L"WrQuantumEnd",
	L"WrDispatchInt",
	L"WrPreempted",
	L"WrYieldExecution",
	L"WrFastMutex",
	L"WrGuardedMutex",
	L"WrRundown",
	L"MaximumWaitReason"
};
