// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
//#define new new( _CLIENT_BLOCK, __FILE__, __LINE__)
#endif

#include <stdio.h>
#include <tchar.h>


// TODO: reference additional headers your program requires here
#include <iostream>
#include <Windows.h>
#include <string>


using namespace std;
