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
	//打开时设置非阻塞
	struct termios ttystate;
    //get the terminal state
    tcgetattr(STDIN_FILENO, &ttystate);
    //turn off canonical mode
    ttystate.c_lflag &= ~ICANON;
    //minimum of number input read.
    ttystate.c_cc[VMIN] = 1;
    //set the terminal attributes.
    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
	return 0;
}

static int StdDeviceExit(void)
{
	//关闭时设置阻塞
	struct termios ttystate;
    //get the terminal state
    tcgetattr(STDIN_FILENO, &ttystate);
	ttystate.c_lflag |= ICANON;
    //set the terminal attributes.
    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
	return 0;
}

static int StdGetInputEvent(PT_InputEvent ptInputEvent)
{
    char cmd;
    cmd = fgetc(stdin);
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

