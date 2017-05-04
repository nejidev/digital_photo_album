#include <conf.h>
#include <disp_manager.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <string.h>

static int FBDeviceInit(void);
static int FBShowPixel(int iX, int iY, unsigned int dwColor);
static int FBCleanScreen(unsigned int dwBackColor);

static int g_fb_fd;

static struct fb_var_screeninfo g_tFBVar;
static struct fb_fix_screeninfo g_tFBFix;
static unsigned char *g_putcFBMem;
static unsigned int g_dwScreenSize;

static unsigned int g_dwLineWidth;
static unsigned int g_dwPixelWidth;

static T_DispOpr g_tFBDispOpr = {
	.name = "fb",
	.DeviceInit  = FBDeviceInit,
	.ShowPixel   = FBShowPixel,
	.CleanScreen = FBCleanScreen,
};

static int FBDeviceInit(void)
{
	int ret;
	g_fb_fd = open(FB_DEVICE, O_RDWR);
	if(0 > g_fb_fd)
	{
		DEBUG_PRINTF("cat't open fb device \n");
		return -1;
	}
	ret = ioctl(g_fb_fd, FBIOGET_VSCREENINFO, &g_tFBVar);
	if(0 > ret)
	{
		DEBUG_PRINTF("cat't get fb var \n");
		return -1;
	}
	ret = ioctl(g_fb_fd, FBIOGET_FSCREENINFO, &g_tFBFix);
	if(0 > ret)
	{
		DEBUG_PRINTF("cat't get fb fix \n");
		return -1;
	}

	g_tFBDispOpr.iXres = g_tFBVar.xres;
	g_tFBDispOpr.iYres = g_tFBVar.yres;
	g_tFBDispOpr.iBpp  = g_tFBVar.bits_per_pixel;

	g_dwPixelWidth  = g_tFBVar.bits_per_pixel / 8;
	g_dwLineWidth   = g_tFBVar.bits_per_pixel / 8 * g_tFBVar.xres;
	g_dwScreenSize  = g_tFBVar.bits_per_pixel / 8 * g_tFBVar.xres * g_tFBVar.yres;
	g_tFBDispOpr.iLineWidth = g_dwLineWidth;

	g_putcFBMem = (unsigned char *)mmap(NULL, g_dwScreenSize, PROT_READ | PROT_WRITE, MAP_SHARED, g_fb_fd, 0);
	if(MAP_FAILED == g_putcFBMem)
	{
		DEBUG_PRINTF("cat't mmap fb \n");
		return -1;
	}
	g_tFBDispOpr.pucDispMem = g_putcFBMem;
	return 0;
}

static int FBShowPixel(int iX, int iY, unsigned int dwColor)
{
	unsigned char *pucFB;
	unsigned short *pucFB16bbp;
	
	int r,g,b;
	if(iX > g_tFBVar.xres || iY > g_tFBVar.yres)
	{
		DEBUG_PRINTF("out of screen region \n");
		return -1;
	}
	pucFB = g_putcFBMem + iY * g_dwLineWidth + iX * g_dwPixelWidth;
	pucFB16bbp = (unsigned short *)pucFB;

	switch(g_tFBVar.bits_per_pixel)
	{
		case 16:
		{
			//תΪ565
			r = (dwColor >>(16+3)) & 0x1f;
			g = (dwColor >>(8+2))  & 0x3f;
			b = (dwColor >>3)  & 0x1f;
			*pucFB16bbp = (unsigned short) (r<<11) | (g<<5) | (b<<0);
		}
		break;
		default:
			DEBUG_PRINTF("cat't support %d bpp \n", g_tFBVar.bits_per_pixel);
			return -1;
	}
	return 0;
}

static int FBCleanScreen(unsigned int dwBackColor)
{
	unsigned char *pucFB;
	unsigned short *pucFB16bbp;
	
	int r,g,b;
	
	pucFB = g_putcFBMem;
	pucFB16bbp = (unsigned short *)pucFB;

	switch(g_tFBVar.bits_per_pixel)
	{
		case 16:
		{
			while(pucFB16bbp < (unsigned short *)(g_putcFBMem + g_dwScreenSize))
			{
				//תΪ565
				r = (dwBackColor >>(16+3)) & 0x1f;
				g = (dwBackColor >>(8+2))  & 0x3f;
				b = (dwBackColor >>3)  & 0x1f;
				*pucFB16bbp = (r<<11) | (g<<5) | (b<<0);
				pucFB16bbp++;
			}
		}
		break;
		default:
			DEBUG_PRINTF("cat't support %d bpp \n", g_tFBVar.bits_per_pixel);
			return -1;
	}
	return 0;
}

int FBInit(void)
{
	return RegisterDispOpr(&g_tFBDispOpr);
}
