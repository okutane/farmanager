enum
{
	MOk,
	MCancel,
	MError,

	MPlistPanel,

	MColumnModule,
	MColumnPriority,
	MColumnParentPID,
	MColumnBits,
	MColumnGDI,
	MColumnUSER,
	MColumnTitle,
	MColFullPathname,

	MBits,
	MTitleModule,
	MTitleFullPath,
	MTitlePID,
	MTitleParentPID,
	MTitlePriority,
	MTitleThreads,
	MTitleStarted,
	MTitleUptime,

	MTitleModuleSize,
	MTitleHeapSize,

	MTitleWindow,
	MTitleStyle,
	MTitleExtStyle,

	MDeleteTitle,
	MDeleteProcess,
	MDeleteProcesses,
	MDeleteNumberOfProcesses,
	MDeleteDelete,
	MCannotDelete,

	MCannotDeleteProc,
	MRetryWithDebug,
	MDangerous,
	MYes,
	MNo,

	MSelectComputer,
	MComputer,
	MEmptyForLocal,
	MUsername,
	MPaswd,
	MEmptyForCurrent,

	MConnect,

	MCannotKillRemote,

	MFPriorMinus,
	MFPriorPlus,
	MFKill,
	MFRemote,
	MFLocal,

	MChangePriority,
	MConfirmChangePriority,

	MGDIObjects,
	MUSERObjects,

	MCommandLine,
	MCurDir,
	MEnvironment,

	MTitleFileVersion,
	MTitleFileDesc,

	MTitleModules,
	MColBaseSize,
	MColPathVerDesc,
	MColPathVerDescNotShown,
	MTitleHandleInfo,
	MHandleInfoHdr,

	MConfigTitle,
	MConfigAddToDisksMenu,
	MConfigDisksMenuDigit,
	MConfigAddToPluginMenu,

	MIncludeAdditionalInfo,
	MInclEnvironment,
	MInclModuleInfo,
	MInclModuleVersion,
	MInclPerformance,
	MInclHandles,
	MInclHandlesUnnamed,

	MProcessorTime,
	MPrivilegedTime,
	MUserTime,
	MHandleCount,
	MPageFaults,
	MPageFileBytes,
	MPageFileBytesPeak,
	MPoolNonpagedBytes,
	MPoolPagedBytes,
	MPrivateBytes,
	MWorkingSet,
	MWorkingSetPeak,
	MIODataBytes,
	MIODataOperations,
	MIOOtherBytes,
	MIOOtherOperations,
	MIOReadBytes,
	MIOReadOperations,
	MIOWriteBytes,
	MIOWriteOperations,
	MVirtualBytes,
	MVirtualBytesPeak,

	MColProcessorTime,
	MColProcessorTimeS,
	MColPrivilegedTime,
	MColPrivilegedTimeS,
	MColUserTime,
	MColUserTimeS,
	MColHandleCount,
	MColPageFaults,
	MColPageFaultsS,
	MColPageFileBytes,
	MColPageFileBytesPeak,
	MColPoolNonpagedBytes,
	MColPoolPagedBytes,
	MColPrivateBytes,
	MColWorkingSet,
	MColWorkingSetPeak,
	MColIODataBytes,
	MColIODataBytesS,
	MColIODataOperations,
	MColIODataOperationsS,
	MColIOOtherBytes,
	MColIOOtherBytesS,
	MColIOOtherOperations,
	MColIOOtherOperationsS,
	MColIOReadBytes,
	MColIOReadBytesS,
	MColIOReadOperations,
	MColIOReadOperationsS,
	MColIOWriteBytes,
	MColIOWriteBytesS,
	MColIOWriteOperations,
	MColIOWriteOperationsS,
	MColVirtualBytes,
	MColVirtualBytesPeak,

	MperSec,

	MSortBy,
	MSortByName,
	MSortByExt,
	MSortByTime,
	MSortBySize,
	MSortByUnsorted,
	MSortByDescriptions,
	MSortByOwner,
	MUseSortGroups,
	MShowSelectedFirst,

	MTitleUsername,
	MTitleSessionId,

	MTAccessDenied,
	MTInsufficientPrivilege,
	MTUnknownFailure,

	MViewDDD,
	MViewWithOptions,
};