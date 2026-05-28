#pragma once
#include "targetver.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "framework.h"
#ifndef _DEBUG
#if __cplusplus
extern "C"
#endif
int _fltused = 1;
#endif
#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))