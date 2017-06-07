#ifndef __DISP_MANAGER_H
#define __DISP_MANAGER_H
#include <pic_operation.h>

/* ��ʾ����,������������Ͻ�/���½�����
 * �����ͼ��,������ͼ����ļ���
 */
typedef struct Layout {
	int iTopLeftX;
	int iTopLeftY;
	int iBottomRightX;
	int iBottomRightY;
	char *strIconName;
}T_Layout, *PT_Layout;

//״̬
typedef enum{
	VMS_FREE = 0,
	VMS_USED_FOR_PREPARE,
	VMS_USED_FOR_CUR,
} E_VideoMemState;

typedef enum{
	PS_BLANK = 0,
	PS_GENERATING,
	PS_GENERATED,
} E_PicState;

typedef struct VideoMem{
	int              iID;
	int              bDevFrameBuffer;
	E_VideoMemState  eVideoMemState;
	E_PicState       ePicState;
	T_PixelDatas     tPixelDatas;
	struct VideoMem *ptNext;
} T_VideoMem, *PT_VideoMem;

typedef struct DispOpr{
	char *name;
	int iXres;
	int iYres;
	int iBpp;
	int iLineWidth;
	unsigned char *pucDispMem;
	int (*DeviceInit)(void);
	int (*DeviceExit)(void);
	int (*ShowPixel)(int iPenX, int iPenY, unsigned int dwColor);
	int (*CleanScreen)(unsigned int dwBackColor);
	//��ʾһҳ
	int (*ShowPage)(PT_VideoMem ptVideoMem);
	struct DispOpr *ptNext;
} T_DispOpr, *PT_DispOpr;

//����VideoMem ��ʾһ��

int RegisterDispOpr(PT_DispOpr ptDispOpr);
void ShowDispOpr(void);
int DisplayInit(void);
int FBInit(void);
PT_DispOpr GetDispOpr(char *pcName);
void SelectDefaultDispDev(char *pcName);
PT_DispOpr GetDefaultDispDev(void);
int GetDispResolution(int *piXres, int *piYres, int *piBpp);
int AllocVideoMem(int iNum);
PT_VideoMem GetVideoMem(int iID, int bCur);
void PutVideoMem(PT_VideoMem ptVideoMem);
void FlushVideoMemToDevSync(PT_VideoMem ptVideoMem);
void FlushVideoMemToDev(PT_VideoMem ptVideoMem);
int ShowPixelPixelDatasMem(PT_PixelDatas ptPixelDatas, int iPenX, int iPenY, unsigned int dwColor);
void ClearPixelDatasMem(PT_PixelDatas ptPixelDatas, unsigned int dwColor);
void ClearVideoMem(PT_VideoMem ptVideoMem, unsigned int dwColor);
int ConvertColorBpp(unsigned int *pDwColor, int bpp);
#endif


