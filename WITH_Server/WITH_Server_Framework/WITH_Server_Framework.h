#pragma once

#include "Component.h"
#include "ComponentStorage.h"

#include "Entity.h"
#include "EntityManager.h"

#include "System.h"
#include "SystemManager.h"

#include "BasicView.h"
#include "StorageRegistry.h"

#include "ECSCore.h"
#include "ECSView.h"


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244 4251 4267 4996)
#elif defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include "ProtocolLib.h"

#ifdef _MSC_VER
#pragma warning(pop)
#elif defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif


#pragma comment(lib, "ProtocolLib.lib")
#pragma comment(lib, "Detour.lib")
#pragma comment(lib, "Recast.lib")