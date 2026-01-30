#include "pch.h"
#include "Core_Global.h"
#include "ThreadManager.h"

unique_ptr<ThreadManager> GThreadManager = make_unique<ThreadManager>();

