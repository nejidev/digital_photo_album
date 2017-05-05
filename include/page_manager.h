#ifndef __PAGE_MANAGER_H
#define __PAGE_MANAGER_H
#include <input_manager.h>
#include <disp_manager.h>
#include <render.h>

/* 页面参数 */
typedef struct PageParams{
	int iPageID; /* 页面的ID */
	char strCurPictureFile[256]; /* 要处理的第一个图片文件 */
}T_PageParams, *PT_PageParams;

/* 页面步局 */
typedef struct PageLayout{
	int iTopLeftX;
	int iTopLeftY;
	int iBottomRightX;
	int iBottomRightY;
	int iBpp;
	int iMaxTotalBytes;
	PT_Layout atLayout;/* icon数组 */
}T_PageLayout, *PT_PageLayout;

/* 页面 */
typedef struct PageAction{
	char *name;
	void (*Run)(PT_PageParams ptPageParams);
	int (*GetInputEvent)(PT_PageLayout ptPageLayout, PT_InputEvent ptInputEvent);
	struct PageAction *ptNext;
}T_PageAction, *PT_PageAction;

/* 页面配置信息 */
typedef struct PageCfg{
	int intvalSceond;
	char strSelectDir[256];
}T_PageCfg, *PT_PageCfg;

//生成 ID 唯一
int ID(char *pcName);
//主页 Page
int MainPageInit(void);
//设置参数的 Page
int SetingPageInit(void);
//定时播放的 Page
int IntevalPageInit(void);
//浏览文件的 Page
int BroserPageInit(void);
//自动播放的 Page
int AutoPageInit(void);
//左侧控制菜单的 Page
int ManualPageInit(void);
//注册 Page
int RegisterPageAction(PT_PageAction ptPageAction);
//总 Page 初始化
int InitPages(void);
//生成一页
int GeneratePage(PT_PageLayout ptPageLayout, PT_VideoMem ptVideoMem);
//通用的获取事件
int GenericGetInputEvent(PT_PageLayout ptPageLayout, PT_InputEvent ptInputEvent);
//根据Page Name 查找 Page
PT_PageAction GetPage(char *pcName);
//比较2个 timeval 相差的毫秒
int TimeMSBetween(struct timeval tTimeStart, struct timeval tTimeEnd);
//得到配置信息
void GetPageCfg(PT_PageCfg ptPagecfg);
//初始化鼠标
int InitMouse(void);
//显示鼠标
int ShowMouse(int x, int y);
//重绘历史鼠标
int ShowHistoryMouse(void);
#endif

