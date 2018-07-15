#ifndef KARIN_SDL_VKB_H
#define KARIN_SDL_VKB_H

#include "vkb.h"

void karinNewSDLVKB(float x, float y, float z, float w, float h);
void karinDeleteSDLVKB(void);
void karinRenderSDLVKB(unsigned state);
unsigned karinSDLVKBMouseMotionEvent(int b, int p, int x, int y, int dx, int dy, VKB_Key_Action_Function f);
unsigned karinSDLVKBMouseEvent(int b, int p, int x, int y, VKB_Key_Action_Function f);
//void karinResizeVKB(const vkb *v, float w, float h);

#endif
