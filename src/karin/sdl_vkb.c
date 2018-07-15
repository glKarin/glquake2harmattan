#include "sdl_vkb.h"
#include <SDL/SDL_image.h>

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>

#define PRINT_SDLERR printf("[SDLerr]: %s\n", SDL_GetError())

enum {
	SDL_RGB = 1,
	SDL_RGBA,
};

typedef SDL_Surface * imaged_t;
typedef short * buffer_t;

#include "vkb_general.c"

extern SDL_Surface *surface; // set externlly for sharing to vkb surface.
static SDL_Surface *vkb_surface = NULL;
static SDL_Rect vkb_rect = {
	0, 0,
	0, 0
};

static void karinBeginRender2D(void)
{
	if(render_lock)
		return;
	if(SDL_MUSTLOCK(vkb_surface))
		SDL_LockSurface(vkb_surface);
	SDL_FillRect(vkb_surface, &vkb_rect, SDL_MapRGBA(vkb_surface->format, 255, 255, 255, 0));
	SDL_BlitSurface(surface, &vkb_rect, vkb_surface, &vkb_rect);
	render_lock = btrue;
}

static void karinEndRender2D(void)
{
	if(!render_lock)
		return;
	SDL_UpdateRect(vkb_surface, 0, 0, 0, 0);
	//SDL_Flip(vkb_surface);
	SDL_BlitSurface(vkb_surface, &vkb_rect, surface, &vkb_rect);
	if(SDL_MUSTLOCK(vkb_surface))
		SDL_UnlockSurface(vkb_surface);
	render_lock = bfalse;
}

static texture karinNewTexture2D(const char *file)
{
	texture tex;
	memset(&tex, 0, sizeof(texture));
	if(!file)
		return tex;
	
	SDL_Surface *img = IMG_Load(file);
	if(!img)
		return tex;
#if 1
	if(vkb_surface)
	{
		SDL_Surface *s = SDL_ConvertSurface(img, vkb_surface->format, vkb_surface->flags);
		if(s)
		{
			/*
			SDL_SetColorKey(s, SDL_SRCCOLORKEY, SDL_MapRGB(s->format, 0, 0, 0));
			SDL_SetAlpha(s, SDL_SRCALPHA|SDL_RLEACCEL, 0);
			*/
			tex.imaged = s;
			SDL_FreeSurface(img);
		}
		else
			tex.imaged = img;
	}
	else
#endif
		tex.imaged = img;
	tex.width = img->w;
	tex.height = img->h;
	tex.format = SDL_RGBA;
	
	return tex;
}

static buffer_t karinNewBuffer(sizei size, const void *data)
{
	buffer_t ptr = malloc(size);
	memset(ptr, 0, size);
	if(data)
		memcpy(ptr, data, size);
	return ptr;
}

void karinNewSDLVKB(float x, float y, float z, float w, float h)
{
	if(the_vkb.inited)
		return;
	the_vkb.x = x;
	the_vkb.y = y;
	the_vkb.z = z;
	the_vkb.width = w;
	the_vkb.height = h;
	 // 32
	uint rmask = 0xff000000;
	uint gmask = 0x00ff0000;
	uint bmask = 0x0000ff00;
	uint amask = 0x000000ff;
	/*
	 // 16
	uint rmask = 0xf000;
	uint gmask = 0x0f00;
	uint bmask = 0x00f0;
	uint amask = 0x000f;
	// 8
	uint rmask = ((~0) & 0xff) << 6;
	uint gmask = ((~0) & 0xff) << 6 >> 2;
	uint bmask = ((~0) & 0xff) << 6 >> 4;
	uint amask = ((~0) & 0xff) << 6 >> 6;
	*/
	vkb_surface = SDL_CreateRGBSurface(SDL_SWSURFACE | SDL_DOUBLEBUF, the_vkb.width, the_vkb.height, 32, rmask, gmask, bmask, amask);

	vkb_rect.w = the_vkb.width;
	vkb_rect.h = the_vkb.height;

	int k;
	for(k = 0; k < VKB_TEX_COUNT; k++)
		the_vkb.tex[k] = karinNewTexture2D(Tex_Files[k]);

	int i;
	int j = 0;

	// btn
	for(i = 0; i < VKB_COUNT; i++)
	{
		karinMakeButton(the_vkb.vb + j, VKB_Button + i, the_vkb.width, the_vkb.height);
		j++;
	}

	// joy
	for(i = 0; i < JOYSTICK_COUNT; i++)
	{
		karinMakeJoystick(the_vkb.vb + j, VKB_Joystick + i, the_vkb.width, the_vkb.height);
		j++;
	}

	// cursor
	for(i = 0; i < CURSOR_COUNT; i++)
	{
		karinMakeCursor(the_vkb.vb + j, VKB_Cursor + i, the_vkb.width, the_vkb.height);
		j++;
	}

	// swipe
	for(i = 0; i < SWIPE_COUNT; i++)
	{
		karinMakeSwipe(the_vkb.vb + j, VKB_Swipe + i, the_vkb.width, the_vkb.height);
		j++;
	}

	the_vkb.inited = btrue;
}

void karinDeleteSDLVKB(void)
{
	if(!the_vkb.inited)
		return;
	int count = TOTAL_VKB_COUNT;
	int i;
	for(i = 0; i < count; i++)
	{
		int j;
		for(j = Position_Coord; j < Total_Coord; j++)
		{
			free(the_vkb.vb[i].base.buffers[j]);
			the_vkb.vb[i].base.buffers[j] = 0;
		}
	}
	for(i = 0; i < VKB_TEX_COUNT; i++)
	{
		if(the_vkb.tex[i].imaged)
			SDL_FreeSurface(the_vkb.tex[i].imaged);
		memset(the_vkb.tex + i, 0, sizeof(texture));
	}
	SDL_FreeSurface(vkb_surface);
	vkb_surface = NULL;

	vkb_rect.w = 0;
	vkb_rect.h = 0;

	the_vkb.inited = bfalse;
	render_lock = bfalse;
}

void karinRenderSDLVKB(unsigned state)
{
	if(!the_vkb.inited)
	{
		return;
	}
	if(!surface || !vkb_surface)
		return;
	karinBeginRender2D();
	{
		int i;
		int count = TOTAL_VKB_COUNT;
		for(i = 0; i < count; i++)
		{
			const virtual_control_item *b = the_vkb.vb + i;
			if((b->base.show_mask & VKB_States[state]) == 0)
				continue;
			switch(b->type)
			{
				case vkb_button_type:
					karinRenderVKBButton(&b->button, the_vkb.tex);
					break;
				case vkb_joystick_type:
					karinRenderVKBJoystick(&b->joystick, the_vkb.tex);
					break;
				case vkb_swipe_type:
					karinRenderVKBSwipe(&b->swipe, the_vkb.tex);
					break;
				case vkb_cursor_type:
					karinRenderVKBCursor(&b->cursor, the_vkb.tex);
					break;
				default:
					continue;
			}
			//#define RENDER_CI_RANGE
#ifdef RENDER_CI_RANGE
			SDL_Rect vertex = {
				b->base.x, the_vkb.height - b->base.y - b->base.height,
				b->base.width, b->base.height
			};
			SDL_Rect event = {
				b->base.e_min_x, b->base.height - b->base.e_min_y - (b->base.e_max_y - b->base.e_min_y),
				(b->base.e_max_x - b->base.e_min_x), (b->base.e_max_y - b->base.e_min_y),
			};
			SDL_FillRect(vkb_surface, &vertex, SDL_MapRGBA(vkb_surface->format, 0, 255, 0, 255));
			SDL_FillRect(vkb_surface, &event, SDL_MapRGBA(vkb_surface->format, 255, 0, 0, 128));
#endif
		}
	}
	//PRINT_SDLERR;
	karinEndRender2D();
}

//void karinResizeVKB(float w, float h);
unsigned karinSDLVKBMouseMotionEvent(int b, int p, int x, int y, int dx, int dy, VKB_Key_Action_Function f)
{
	return karinVKBMouseMotionEvent(b, p, x, y, dx, dy, f);
}

unsigned karinSDLVKBMouseEvent(int b, int p, int x, int y, VKB_Key_Action_Function f)
{
	return karinVKBMouseEvent(b, p, x, y, f);
}

#define GET_JOY_XY(r, v, s, w, b) \
	switch(b) \
{ \
	case opengl_mf_base : \
	case opengl_s_base : \
											 r = v + (1.0 - s) * w / 2.0; \
	break; \
	case opengl_mb_base : \
	case opengl_e_base : \
											 r = v - (1.0 - s) * w / 2.0; \
	break; \
	default : \
						r = v; \
	break; \
}

#define GET_VB_XY(r, t, v, b) \
	switch(b) \
{ \
	case opengl_mf_base : \
												r = t / 2 + v; \
	break; \
	case opengl_mb_base : \
												r = t / 2 - v; \
	break; \
	case opengl_e_base : \
											 r = t - v; \
	break; \
	case opengl_s_base : \
	default : \
						r = v; \
	break; \
}

#define GET_TEX_S(t, v, w) (float)(v + w) / (float)t
#define GET_TEX_T(t, v, h) (float)(v - h) / (float)t

void karinMakeCursor(virtual_control_item *b, struct vkb_cursor *d, unsigned int width, unsigned int height)
{
	if(!b || !d)
		return;
	b->cursor.base.type = vkb_cursor_type;
	b->cursor.base.pressed = bfalse;
	b->cursor.base.enabled = d->ava;
	b->cursor.base.tex_index = d->tex_index;
	b->cursor.base.z = d->z;
	b->cursor.ignore_distance = d->ignore_distance;
	b->cursor.angle = -1.0f;
	b->cursor.distance = 0.0f;
	b->cursor.render_bg = d->render;
	b->cursor.pos_x = 0.0f;
	b->cursor.pos_y = 0.0f;
	b->cursor.base.visible = d->ava;
	b->cursor.actions[0] = d->action0;
	b->cursor.actions[1] = d->action1;
	b->cursor.actions[2] = d->action2;
	b->cursor.actions[3] = d->action3;
	b->cursor.action = d->action;
	b->cursor.base.show_mask = d->mask;
	if(!b->cursor.base.enabled)
	{
		b->cursor.actions[0] = Total_Action;
		b->cursor.actions[1] = Total_Action;
		b->cursor.actions[2] = Total_Action;
		b->cursor.actions[3] = Total_Action;
		b->cursor.action = Total_Action;
		return;
	}

	GET_VB_XY(b->cursor.base.x, width, d->x, d->x_base)
		GET_VB_XY(b->cursor.base.y, height, d->y, d->y_base)
		b->cursor.base.width = d->r;
	b->cursor.base.height = d->r;
	b->cursor.e_radius = d->r * d->eminr;
	float ex;
	float ey;
	GET_JOY_XY(ex, d->x, d->emaxr, d->r, d->x_base)
		GET_JOY_XY(ey, d->y, d->emaxr, d->r, d->y_base)
		GET_VB_XY(b->cursor.base.e_min_x, width, ex, d->x_base)
		GET_VB_XY(b->cursor.base.e_min_y, height, ey, d->y_base)
		b->cursor.base.e_max_x = b->cursor.base.e_min_x + d->r * d->emaxr;
	b->cursor.base.e_max_y = b->cursor.base.e_min_y + d->r * d->emaxr;

	short jw = (short)(b->cursor.base.width * d->joy_r / 2.0);
	short jh = (short)(b->cursor.base.height * d->joy_r / 2.0);
	short vertex[] = {
		// joy
		width, // unused
		height, 2 * jw, 2 * jh,
		// bg
		b->cursor.base.x, height - b->cursor.base.y - b->cursor.base.height,
		b->cursor.base.width,  b->cursor.base.height,
	};
	short texcoord[] = {
		// joy
		d->joy_tx, d->joy_ty - d->joy_tw,
		d->joy_tw, d->joy_tw,

		// joy pressed
		d->joy_ptx, d->joy_pty - d->joy_ptw,
		d->joy_ptw, d->joy_ptw,

		// bg
		 d->tx, d->ty - d->tw,
		 d->tw, d->tw,
	};
	b->cursor.base.buffers[Position_Coord] = karinNewBuffer(sizeof(short) * 8, vertex);
	b->cursor.base.buffers[Texture_Coord] = karinNewBuffer(sizeof(short) * 12, texcoord);
}

void karinMakeJoystick(virtual_control_item *b, struct vkb_joystick *d, unsigned int width, unsigned int height)
{
	if(!b || !d)
		return;
	b->joystick.base.type = vkb_joystick_type;
	b->joystick.base.pressed = bfalse;
	b->joystick.base.enabled = d->ava;
	b->joystick.base.tex_index = 1;
	b->joystick.base.z = d->z;
	b->joystick.angle = -1.0f;
	b->joystick.percent = 0.0f;
	b->joystick.pos_x = 0.0f;
	b->joystick.pos_y = 0.0f;
	b->joystick.base.visible = d->ava;
	b->joystick.actions[0] = d->action0;
	b->joystick.actions[1] = d->action1;
	b->joystick.actions[2] = d->action2;
	b->joystick.actions[3] = d->action3;
	b->joystick.base.show_mask = d->mask;
	if(!b->joystick.base.enabled)
	{
		b->joystick.actions[0] = Total_Action;
		b->joystick.actions[1] = Total_Action;
		b->joystick.actions[2] = Total_Action;
		b->joystick.actions[3] = Total_Action;
		return;
	}

	GET_VB_XY(b->joystick.base.x, width, d->x, d->x_base)
		GET_VB_XY(b->joystick.base.y, height, d->y, d->y_base)
		b->joystick.base.width = d->r;
	b->joystick.base.height = d->r;
	b->joystick.e_ignore_radius = d->r * d->eminr;
	b->joystick.e_radius = d->r * d->emaxr;
	float ex;
	float ey;
	GET_JOY_XY(ex, d->x, d->emaxr, d->r, d->x_base)
		GET_JOY_XY(ey, d->y, d->emaxr, d->r, d->y_base)
		GET_VB_XY(b->joystick.base.e_min_x, width, ex, d->x_base)
		GET_VB_XY(b->joystick.base.e_min_y, height, ey, d->y_base)
		b->joystick.base.e_max_x = b->joystick.base.e_min_x + d->r * d->emaxr;
	b->joystick.base.e_max_y = b->joystick.base.e_min_y + d->r * d->emaxr;

	short jw = (short)(b->joystick.base.width * d->joy_r / 2.0);
	short jh = (short)(b->joystick.base.height * d->joy_r / 2.0);
	short vertex[] = {
		b->joystick.base.x, height - b->joystick.base.y - b->joystick.base.height, 
		b->joystick.base.width, b->joystick.base.height,
		width, // unused
		height, 2 * jw, 2 * jh
	};
	short texcoord[] = {
		d->tx, d->ty - d->tw, 
		d->tw, d->tw,

		// joy
		d->joy_tx, d->joy_ty - d->joy_tw, 
		d->joy_tw, d->joy_tw,
	};
	b->joystick.base.buffers[Position_Coord] = karinNewBuffer(sizeof(short) * 8, vertex);
	b->joystick.base.buffers[Texture_Coord] = karinNewBuffer(sizeof(short) * 8, texcoord);
}

void karinMakeSwipe(virtual_control_item *b, struct vkb_swipe *d, unsigned int width, unsigned int height)
{
	if(!b || !d)
		return;
	b->swipe.base.type = vkb_swipe_type;
	b->swipe.base.pressed = bfalse;
	b->swipe.base.tex_index = 1;
	b->swipe.base.enabled = d->ava;
	b->swipe.base.visible = d->ava;
	b->swipe.base.z = d->z;
	b->swipe.ignore_distance = 1.5f;
	b->swipe.angle = -1.0f;
	b->swipe.distance = 0.0f;
	b->swipe.action = d->action;
	b->swipe.actions[0] = d->action0;
	b->swipe.actions[1] = d->action1;
	b->swipe.actions[2] = d->action2;
	b->swipe.actions[3] = d->action3;
	b->swipe.base.show_mask = d->mask;
	if(!b->swipe.base.enabled)
	{
		b->swipe.actions[0] = Total_Action;
		b->swipe.actions[1] = Total_Action;
		b->swipe.actions[2] = Total_Action;
		b->swipe.actions[3] = Total_Action;
		return;
	}

	GET_VB_XY(b->swipe.base.x, width, d->x, d->x_base)
		GET_VB_XY(b->swipe.base.y, height, d->y, d->y_base)
		b->swipe.base.width = d->w;
	b->swipe.base.height = d->h;
	GET_VB_XY(b->swipe.base.e_min_x, width, d->ex, d->x_base)
		GET_VB_XY(b->swipe.base.e_min_y, height, d->ey, d->y_base)
		b->swipe.base.e_max_x = b->swipe.base.e_min_x + d->ew;
	b->swipe.base.e_max_y = b->swipe.base.e_min_y + d->eh;
	if(d->render)
	{
		short vertex[] = {
			b->swipe.base.x, height - b->swipe.base.y - b->swipe.base.height,
			b->swipe.base.width, b->swipe.base.height,
		};
		short texcoord[] = {
			 d->tx, d->ty - d->tw, 
			 d->tw, d->tw,
		};
		b->swipe.base.buffers[Position_Coord] = karinNewBuffer(sizeof(short) * 4, vertex);
		b->swipe.base.buffers[Texture_Coord] = karinNewBuffer(sizeof(short) * 4, texcoord);
	}
}

void karinMakeButton(virtual_control_item *b, struct vkb_button *d, unsigned int width, unsigned int height)
{
	if(!b || !d)
		return;
	b->button.base.type = vkb_button_type;
	b->button.base.pressed = bfalse;
	b->button.base.enabled = d->ava;
	b->button.base.visible = d->ava;
	b->button.base.z = d->z;
	b->button.action = d->action;
	b->button.base.show_mask = d->mask;
	b->button.base.tex_index = d->tex_index;
	if(!b->button.base.enabled)
	{
		b->button.action = Total_Action;
		return;
	}

	GET_VB_XY(b->button.base.x, width, d->x, d->x_base)
		GET_VB_XY(b->button.base.y, height, d->y, d->y_base)
		b->button.base.width = d->w;
	b->button.base.height = d->h;
	GET_VB_XY(b->button.base.e_min_x, width, d->ex, d->x_base)
		GET_VB_XY(b->button.base.e_min_y, height, d->ey, d->y_base)
		b->button.base.e_max_x = b->button.base.e_min_x + d->ew;
	b->button.base.e_max_y = b->button.base.e_min_y + d->eh;
	short vertex[] = {
		b->button.base.x, height - b->button.base.y - b->button.base.height,
		b->button.base.width, b->button.base.height
	};
	short sth = d->th > 0 ? d->th : -d->th;
	short spth = d->pth > 0 ? d->pth : -d->pth;
	short sthm = d->th > 0 ? 1 : 0;
	short spthm = d->pth > 0 ? 1 : 0;
	short texcoord[] = {
		d->tx, d->ty - sth * sthm,
		d->tw, sth,
		d->ptx, d->pty - spth * spthm,
		d->ptw, spth
	};
	b->button.base.buffers[Position_Coord] = karinNewBuffer(sizeof(short) * 4, vertex);
	b->button.base.buffers[Texture_Coord] = karinNewBuffer(sizeof(short) * 8, texcoord);
}
#undef GET_VB_XY
#undef GET_TEX_S
#undef GET_TEX_T

void karinRenderVKBButton(const virtual_button *b, const texture const tex[])
{
	if(!b)
		return;
	if(b->base.type != vkb_button_type)
		return;
	if(!b->base.visible)
		return;
	if(!tex[b->base.tex_index].imaged)
		return;
	if(!b->base.buffers[Position_Coord] || !b->base.buffers[Texture_Coord])
		return;
	SDL_Rect src = {
		(b->base.pressed && b->base.enabled) ? b->base.buffers[Texture_Coord][4] : b->base.buffers[Texture_Coord][0], 
		(b->base.pressed && b->base.enabled) ? b->base.buffers[Texture_Coord][5] : b->base.buffers[Texture_Coord][1],
		(b->base.pressed && b->base.enabled) ? b->base.buffers[Texture_Coord][6] : b->base.buffers[Texture_Coord][2],
		(b->base.pressed && b->base.enabled) ? b->base.buffers[Texture_Coord][7] : b->base.buffers[Texture_Coord][3]
	};
	SDL_Rect dst = {
		b->base.buffers[Position_Coord][0], 
		b->base.buffers[Position_Coord][1], 
		b->base.buffers[Position_Coord][2], 
		b->base.buffers[Position_Coord][3]
	};
	SDL_SoftStretch(tex[b->base.tex_index].imaged, &src, vkb_surface, &dst);
}

void karinRenderVKBSwipe(const virtual_swipe *b, const texture const tex[])
{
	if(!b)
		return;
	if(b->base.type != vkb_swipe_type)
		return;
	if(!b->base.visible)
		return;
	if(!tex[b->base.tex_index].imaged)
		return;
	if(!b->base.buffers[Position_Coord] || !b->base.buffers[Texture_Coord])
		return;
	SDL_Rect src = {
		b->base.buffers[Texture_Coord][0], 
		b->base.buffers[Texture_Coord][1], 
		b->base.buffers[Texture_Coord][2], 
		b->base.buffers[Texture_Coord][3]
	};
	SDL_Rect dst = {
		b->base.buffers[Position_Coord][0], 
		b->base.buffers[Position_Coord][1], 
		b->base.buffers[Position_Coord][2], 
		b->base.buffers[Position_Coord][3]
	};
	SDL_SoftStretch(tex[b->base.tex_index].imaged, &src, vkb_surface, &dst);
}

void karinRenderVKBJoystick(const virtual_joystick *b, const texture const tex[])
{
	if(!b)
		return;
	if(b->base.type != vkb_joystick_type)
		return;
	if(!b->base.visible)
		return;
	if(!b->base.buffers[Position_Coord] || !b->base.buffers[Texture_Coord])
		return;
	if(!tex[b->base.tex_index].imaged)
		return;
	SDL_Rect src = {
		b->base.buffers[Texture_Coord][0], 
		b->base.buffers[Texture_Coord][1], 
		b->base.buffers[Texture_Coord][2], 
		b->base.buffers[Texture_Coord][3]
	};
	SDL_Rect dst = {
		b->base.buffers[Position_Coord][0], 
		b->base.buffers[Position_Coord][1], 
		b->base.buffers[Position_Coord][2], 
		b->base.buffers[Position_Coord][3]
	};
	//SDL_FillRect(surface, &dst, 0xff0000ff);
	SDL_SoftStretch(tex[b->base.tex_index].imaged, &src, vkb_surface, &dst);

	short cx = (short)(b->base.x + b->base.width / 2 - b->base.buffers[Position_Coord][6] / 2);
	short cy = (short)(b->base.y + b->base.height / 2 + b->base.buffers[Position_Coord][7] / 2);
	SDL_Rect src_joy = {
		b->base.buffers[Texture_Coord][4], 
		b->base.buffers[Texture_Coord][5], 
		b->base.buffers[Texture_Coord][6], 
		b->base.buffers[Texture_Coord][7]
	};
	SDL_Rect dst_joy = {
		cx + b->pos_x, b->base.buffers[Position_Coord][5] - cy - b->pos_y, 
		b->base.buffers[Position_Coord][6], 
		b->base.buffers[Position_Coord][7]
	};
	SDL_SoftStretch(tex[b->base.tex_index].imaged, &src_joy, vkb_surface, &dst_joy);
}

void karinRenderVKBCursor(const virtual_cursor *b, const texture tex[])
{
	if(!b)
		return;
	if(b->base.type != vkb_cursor_type)
		return;
	if(!b->base.visible)
		return;
	if(!tex[b->base.tex_index].imaged)
		return;
	if(!b->base.buffers[Position_Coord] || !b->base.buffers[Texture_Coord])
		return;
	if(b->render_bg)
	{
		SDL_Rect src = {
			b->base.buffers[Texture_Coord][8], 
			b->base.buffers[Texture_Coord][9], 
			b->base.buffers[Texture_Coord][10], 
			b->base.buffers[Texture_Coord][11]
		};
		SDL_Rect dst = {
			b->base.buffers[Position_Coord][4], 
			b->base.buffers[Position_Coord][5], 
			b->base.buffers[Position_Coord][6], 
			b->base.buffers[Position_Coord][7]
		};
		SDL_SoftStretch(tex[b->base.tex_index].imaged, &src, vkb_surface, &dst);
	}
	short cx = (short)(b->base.x + b->base.width / 2 - b->base.buffers[Position_Coord][2] / 2);
	short cy = (short)(b->base.y + b->base.height / 2 + b->base.buffers[Position_Coord][3] / 2);
	SDL_Rect src_joy = {
		(b->base.pressed && b->base.enabled) ? b->base.buffers[Texture_Coord][4] : b->base.buffers[Texture_Coord][0], 
		(b->base.pressed && b->base.enabled) ? b->base.buffers[Texture_Coord][5] : b->base.buffers[Texture_Coord][1], 
		(b->base.pressed && b->base.enabled) ? b->base.buffers[Texture_Coord][6] : b->base.buffers[Texture_Coord][2], 
		(b->base.pressed && b->base.enabled) ? b->base.buffers[Texture_Coord][7] : b->base.buffers[Texture_Coord][3]
	};
	SDL_Rect dst_joy = {
		cx + b->pos_x, b->base.buffers[Position_Coord][1] - cy - b->pos_y, 
		b->base.buffers[Position_Coord][2], 
		b->base.buffers[Position_Coord][3]
	};
	//SDL_FillRect(surface, &dst, 0xff0000ff);
	SDL_SoftStretch(tex[b->base.tex_index].imaged, &src_joy, vkb_surface, &dst_joy);

}
