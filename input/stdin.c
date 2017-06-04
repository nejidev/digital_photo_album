#include <input_manager.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>

static int StdDeviceInit(void);
static int StdDeviceExit(void);
static int StdGetInputEvent(PT_InputEvent ptInputEvent);
static T_InputOpr g_tStdinInputOpr = {
	.name = "std",
	.DeviceInit = StdDeviceInit,
	.DeviceExit = StdDeviceExit,
	.GetInputEvent = StdGetInputEvent,
};
static int StdDeviceInit(void)
{
	return 0;
}

static int StdDeviceExit(void)
{
	return 0;
}

static int StdGetInputEvent(PT_InputEvent ptInputEvent)
{
    char cmd;
    cmd = getchar();
	ptInputEvent->iType = INPUT_TYPE_STDIN;
	gettimeofday(&ptInputEvent->tTime, NULL); 
 	switch(cmd)
 	{
 		case '1' : ptInputEvent->iKey = INPUT_VALUE_UP;      break;
 		case '2' : ptInputEvent->iKey = INPUT_VALUE_DOWN;    break;
 		case 'q' : ptInputEvent->iKey = INPUT_VALUE_EXIT;    break;
 		default : ptInputEvent->iKey  = INPUT_VALUE_UNKNOWN; break;
 	}
	return 0;
}

int StdInit(void)
{
	return RegisterInputOpr(&g_tStdinInputOpr);
}

