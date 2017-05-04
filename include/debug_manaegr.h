#ifndef __DEBUGMANAGER_H
#define __DEBUGMANAGER_H
typedef struct DebugOpr{
	int (*DeviceInit)(void);
} T_DebugOpr, *PT_DebugOpr;
#endif

