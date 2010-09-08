#pragma once
#include "FarPluginBase.hpp"
#include "SystemApi.hpp"
#include "strmix.hpp"
#include "debug.h"
#include "makeguid.h"

//observer
#include "Observer/ModuleDef.h"

struct ProgressContextEx {
	ProgressContext ctx;
	HANDLE filler;
	HANDLE hArchive;
};

//newarc
#include "../../API/module.hpp"

class ObserverArchive;
class ObserverModule;
class ObserverPlugin;

#include "observer.Archive.h"
#include "observer.Plugin.h"
#include "observer.Module.h"