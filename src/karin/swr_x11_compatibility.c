#include "sdl_vkb.h"

void karinNewSDLVKB(float x, float y, float z, float w, float h)
{
	printf("\n[karin]: Do nothing. Only for X11 software renderer compatibility.\n");
}

void karinDeleteSDLVKB(void)
{
	printf("\n[karin]: Do nothing. Only for X11 software renderer compatibility.\n");
}

void karinRenderSDLVKB(unsigned state)
{
	//printf("\n[karin]: Do nothing. Only for X11 software renderer compatibility.\n");
}

unsigned karinSDLVKBMouseMotionEvent(int b, int p, int x, int y, int dx, int dy, VKB_Key_Action_Function f)
{
	//printf("\n[karin]: Do nothing. Only for X11 software renderer compatibility.\n");
	return 0;
}

unsigned karinSDLVKBMouseEvent(int b, int p, int x, int y, VKB_Key_Action_Function f)
{
	//printf("\n[karin]: Do nothing. Only for X11 software renderer compatibility.\n");
	return 0;
}
//void karinResizeVKB(const vkb *v, float w, float h);
