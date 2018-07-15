#include "gl_vkb.h"
#include "../ref_gl/gl_local.h"

#include "q3_png.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>

typedef GLuint imaged_t;
typedef GLuint buffer_t;

#include "vkb_general.c"

static GLint active_texture;
static GLint client_active_texture;
static GLboolean texture2d;
static GLfloat alpha_ref;
static GLint alpha_func;
static GLint blend_src;
static GLint blend_dst;
static GLboolean blend;
static GLboolean alpha_test;
static GLboolean depth_test;
static GLint matrix_mode;
static GLint viewport[4];

static void karinBeginRender2D(void)
{
	if(render_lock)
		return;
	// Get
	{
		texture2d = qglIsEnabled(GL_TEXTURE_2D);
		blend = qglIsEnabled(GL_BLEND);
		alpha_test = qglIsEnabled(GL_ALPHA_TEST);
		depth_test = qglIsEnabled(GL_DEPTH_TEST);
		if(qglActiveTexture)
			qglGetIntegerv(GL_ACTIVE_TEXTURE, &active_texture);
		if(qglClientActiveTexture)
			qglGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &client_active_texture);
		qglGetFloatv(GL_ALPHA_TEST_REF, &alpha_ref);
		qglGetIntegerv(GL_ALPHA_TEST_FUNC, &alpha_func);
		qglGetIntegerv(GL_BLEND_SRC, &blend_src);
		qglGetIntegerv(GL_BLEND_DST, &blend_dst);
		qglGetIntegerv(GL_MATRIX_MODE, &matrix_mode);
		qglGetIntegerv(GL_VIEWPORT, viewport);
	}
	// Set
	{
		if(!alpha_test)
			qglEnable(GL_ALPHA_TEST);
		qglAlphaFunc(GL_GREATER, 0.1);
		if(!blend)
			qglEnable(GL_BLEND);
		qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		if(!texture2d)
			qglEnable(GL_TEXTURE_2D);
		if(depth_test)
			qglDisable(GL_DEPTH_TEST);
		if(qglClientActiveTexture)
			qglClientActiveTexture(GL_TEXTURE0);
		if(qglActiveTexture)
			qglActiveTexture(GL_TEXTURE0);
	}
	// Matrix
	{
		qglViewport(0, 0, HARMATTAN_WIDTH, HARMATTAN_HEIGHT);
		qglMatrixMode(GL_PROJECTION);
		qglPushMatrix();
		qglLoadIdentity();
		qglOrthof(0.0, HARMATTAN_WIDTH, 0.0, HARMATTAN_HEIGHT, -1.0, 1.0);
		qglMatrixMode(GL_MODELVIEW);
		qglPushMatrix();
		qglLoadIdentity();
	}
	render_lock = btrue;
}

static void karinEndRender2D(void)
{
	if(!render_lock)
		return;
	{
		qglMatrixMode(GL_MODELVIEW);
		qglPopMatrix();
		qglMatrixMode(GL_PROJECTION);
		qglPopMatrix();
	}
	{
		if(!texture2d)
			qglDisable(GL_TEXTURE_2D);
		if(qglClientActiveTexture)
			qglClientActiveTexture(client_active_texture);
		if(qglActiveTexture)
			qglActiveTexture(active_texture);
		qglAlphaFunc(alpha_func, alpha_ref);
		if(!alpha_test)
			qglDisable(GL_ALPHA_TEST);
		if(!blend)
			qglDisable(GL_BLEND);
		qglBlendFunc(blend_src, blend_dst);
		if(depth_test)
			qglEnable(GL_DEPTH_TEST);
		qglMatrixMode(matrix_mode);
		qglViewport(viewport[0], viewport[1], viewport[2],viewport[3]);
	}
	render_lock = bfalse;
}

static texture karinNewTexture2D(const char *file)
{
	texture tex;
	memset(&tex, 0, sizeof(texture));
	if(!file)
		return tex;
	int width = 0;
	int height = 0;
	unsigned char *data = NULL;
	karinLoadPNG(file, &data, &width, &height);
	//printf("%s %p %d %d\n", file, data, width, height);
	if(!data)
		return tex;
	GLint active_texture;
	if(qglActiveTexture)
	{
		qglGetIntegerv(GL_ACTIVE_TEXTURE, &active_texture);
		qglActiveTexture(GL_TEXTURE0);
	}

	qglGenTextures(1, &tex.imaged);
	qglBindTexture(GL_TEXTURE_2D, tex.imaged);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	qglBindTexture(GL_TEXTURE_2D, 0);
	free(data);

	tex.width = width;
	tex.height = height;
	tex.format = GL_RGBA;

	if(qglActiveTexture)
	{
		qglActiveTexture(active_texture);
	}
	return tex;
}

static buffer_t karinNewBuffer(uenum buffer, sizei size, const void *data, uenum solution)
{
	buffer_t bufid;
	qglGenBuffers(1, &bufid);
	qglBindBuffer(GL_ARRAY_BUFFER, bufid);
	qglBufferData(buffer, size, data, solution);
	qglBindBuffer(GL_ARRAY_BUFFER, 0);
	return bufid;
}

void karinNewGLVKB(float x, float y, float z, float w, float h)
{
	if(the_vkb.inited)
		return;
	the_vkb.x = x;
	the_vkb.y = y;
	the_vkb.z = z;
	the_vkb.width = w;
	the_vkb.height = h;
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

void karinDeleteGLVKB(void)
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
			if(qglIsBuffer(the_vkb.vb[i].base.buffers[j]))
				qglDeleteBuffers(1, the_vkb.vb[i].base.buffers + j);
			the_vkb.vb[i].base.buffers[j] = 0;
		}
	}
	for(i = 0; i < VKB_TEX_COUNT; i++)
	{
		if(qglIsTexture(the_vkb.tex[i].imaged))
			qglDeleteTextures(1, &the_vkb.tex[i].imaged);
		memset(the_vkb.tex + i, 0, sizeof(texture));
	}
	the_vkb.inited = bfalse;
	render_lock = bfalse;
}

void karinRenderGLVKB(unsigned state)
{
	if(!the_vkb.inited)
	{
		return;
	}
	karinBeginRender2D();
	{
		qglEnableClientState(GL_VERTEX_ARRAY);
		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
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
			qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
			float vv[] = {
				(b->cursor.base.x), (b->cursor.base.y),
				(b->cursor.base.x + b->cursor.base.width), (b->cursor.base.y),
				(b->cursor.base.x), (b->cursor.base.y + b->cursor.base.height),
				(b->cursor.base.x + b->cursor.base.width), (b->cursor.base.y + b->cursor.base.height),

				(b->cursor.base.e_min_x), (b->cursor.base.e_min_y),
				(b->cursor.base.e_max_x), (b->cursor.base.e_min_y),
				(b->cursor.base.e_min_x), (b->cursor.base.e_max_y),
				(b->cursor.base.e_max_x), (b->cursor.base.e_max_y),
			};
			qglBindTexture(GL_TEXTURE_2D, 0);
			qglVertexPointer(2, GL_FLOAT, 0, vv);
			qglColor4f(1,0,0,0.5);
			qglDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			qglColor4f(0,1,0,0.2);
			qglDrawArrays(GL_TRIANGLE_STRIP, 4, 4);
			qglColor4f(1,1,1,1);
			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
#endif
		}
		qglBindTexture(GL_TEXTURE_2D, 0);
		qglDisableClientState(GL_VERTEX_ARRAY);
		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	karinEndRender2D();
}

unsigned karinGLVKBMouseMotionEvent(int b, int p, int x, int y, int dx, int dy, VKB_Key_Action_Function f)
{
	return karinVKBMouseMotionEvent(b, p, x, y, dx, dy, f);
}

unsigned karinGLVKBMouseEvent(int b, int p, int x, int y, VKB_Key_Action_Function f)
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

	float jw = b->cursor.base.width * d->joy_r / 2.0;
	float jh = b->cursor.base.height * d->joy_r / 2.0;
	float vertex[] = {
		// joy
		-jw, -jh,
		jw, -jh,
		-jw, jh,
		jw, jh,
		// bg
		b->cursor.base.x, b->cursor.base.y,
		b->cursor.base.x + b->cursor.base.width, b->cursor.base.y,
		b->cursor.base.x, b->cursor.base.y + b->cursor.base.height,
		b->cursor.base.x + b->cursor.base.width, b->cursor.base.y + b->cursor.base.height
	};
	float texcoord[] = {
		// joy
		GET_TEX_S(TEX_FULL_WIDTH, d->joy_tx, 0), GET_TEX_T(TEX_FULL_HEIGHT, d->joy_ty, 0),
		GET_TEX_S(TEX_FULL_WIDTH, d->joy_tx, d->joy_tw), GET_TEX_T(TEX_FULL_HEIGHT, d->joy_ty, 0),
		GET_TEX_S(TEX_FULL_WIDTH, d->joy_tx, 0), GET_TEX_T(TEX_FULL_HEIGHT, d->joy_ty, d->joy_tw),
		GET_TEX_S(TEX_FULL_WIDTH, d->joy_tx, d->joy_tw), GET_TEX_T(TEX_FULL_HEIGHT, d->joy_ty, d->joy_tw),

		// joy pressed
		GET_TEX_S(TEX_FULL_WIDTH, d->joy_ptx, 0), GET_TEX_T(TEX_FULL_HEIGHT, d->joy_pty, 0),
		GET_TEX_S(TEX_FULL_WIDTH, d->joy_ptx, d->joy_ptw), GET_TEX_T(TEX_FULL_HEIGHT, d->joy_pty, 0),
		GET_TEX_S(TEX_FULL_WIDTH, d->joy_ptx, 0), GET_TEX_T(TEX_FULL_HEIGHT, d->joy_pty, d->joy_ptw),
		GET_TEX_S(TEX_FULL_WIDTH, d->joy_ptx, d->joy_ptw), GET_TEX_T(TEX_FULL_HEIGHT, d->joy_pty, d->joy_ptw),

		// bg
		GET_TEX_S(TEX_FULL_WIDTH, d->tx, 0), GET_TEX_T(TEX_FULL_HEIGHT, d->ty, 0),
		GET_TEX_S(TEX_FULL_WIDTH, d->tx, d->tw), GET_TEX_T(TEX_FULL_HEIGHT, d->ty, 0),
		GET_TEX_S(TEX_FULL_WIDTH, d->tx, 0), GET_TEX_T(TEX_FULL_HEIGHT, d->ty, d->tw),
		GET_TEX_S(TEX_FULL_WIDTH, d->tx, d->tw), GET_TEX_T(TEX_FULL_HEIGHT, d->ty, d->tw)
	};
	b->cursor.base.buffers[Position_Coord] = karinNewBuffer(GL_ARRAY_BUFFER, sizeof(float) * 16, vertex, GL_STATIC_DRAW);
	b->cursor.base.buffers[Texture_Coord] = karinNewBuffer(GL_ARRAY_BUFFER, sizeof(float) * 24, texcoord, GL_STATIC_DRAW);
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

	float jw = b->joystick.base.width * d->joy_r / 2.0;
	float jh = b->joystick.base.height * d->joy_r / 2.0;
	float vertex[] = {
		b->joystick.base.x, b->joystick.base.y,
		b->joystick.base.x + b->joystick.base.width, b->joystick.base.y,
		b->joystick.base.x, b->joystick.base.y + b->joystick.base.height,
		b->joystick.base.x + b->joystick.base.width, b->joystick.base.y + b->joystick.base.height,
		// joy
		-jw, -jh,
		jw, -jh,
		-jw, jh,
		jw, jh
	};
	float texcoord[] = {
		GET_TEX_S(TEX_FULL_WIDTH, d->tx, 0), GET_TEX_T(TEX_FULL_HEIGHT, d->ty, 0),
		GET_TEX_S(TEX_FULL_WIDTH, d->tx, d->tw), GET_TEX_T(TEX_FULL_HEIGHT, d->ty, 0),
		GET_TEX_S(TEX_FULL_WIDTH, d->tx, 0), GET_TEX_T(TEX_FULL_HEIGHT, d->ty, d->tw),
		GET_TEX_S(TEX_FULL_WIDTH, d->tx, d->tw), GET_TEX_T(TEX_FULL_HEIGHT, d->ty, d->tw),

		// joy
		GET_TEX_S(TEX_FULL_WIDTH, d->joy_tx, 0), GET_TEX_T(TEX_FULL_HEIGHT, d->joy_ty, 0),
		GET_TEX_S(TEX_FULL_WIDTH, d->joy_tx, d->joy_tw), GET_TEX_T(TEX_FULL_HEIGHT, d->joy_ty, 0),
		GET_TEX_S(TEX_FULL_WIDTH, d->joy_tx, 0), GET_TEX_T(TEX_FULL_HEIGHT, d->joy_ty, d->joy_tw),
		GET_TEX_S(TEX_FULL_WIDTH, d->joy_tx, d->joy_tw), GET_TEX_T(TEX_FULL_HEIGHT, d->joy_ty, d->joy_tw),
	};
	b->joystick.base.buffers[Position_Coord] = karinNewBuffer(GL_ARRAY_BUFFER, sizeof(float) * 16, vertex, GL_STATIC_DRAW);
	b->joystick.base.buffers[Texture_Coord] = karinNewBuffer(GL_ARRAY_BUFFER, sizeof(float) * 16, texcoord, GL_STATIC_DRAW);
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
		float vertex[] = {
			b->swipe.base.x, b->swipe.base.y,
			b->swipe.base.x + b->swipe.base.width, b->swipe.base.y,
			b->swipe.base.x, b->swipe.base.y + b->swipe.base.height,
			b->swipe.base.x + b->swipe.base.width, b->swipe.base.y + b->swipe.base.height
		};
		float texcoord[] = {
			GET_TEX_S(TEX_FULL_WIDTH, d->tx, 0), GET_TEX_T(TEX_FULL_HEIGHT, d->ty, 0),
			GET_TEX_S(TEX_FULL_WIDTH, d->tx, d->tw), GET_TEX_T(TEX_FULL_HEIGHT, d->ty, 0),
			GET_TEX_S(TEX_FULL_WIDTH, d->tx, 0), GET_TEX_T(TEX_FULL_HEIGHT, d->ty, d->tw),
			GET_TEX_S(TEX_FULL_WIDTH, d->tx, d->tw), GET_TEX_T(TEX_FULL_HEIGHT, d->ty, d->tw),
		};
		b->swipe.base.buffers[Position_Coord] = karinNewBuffer(GL_ARRAY_BUFFER, sizeof(float) * 8, vertex, GL_STATIC_DRAW);
		b->swipe.base.buffers[Texture_Coord] = karinNewBuffer(GL_ARRAY_BUFFER, sizeof(float) * 8, texcoord, GL_STATIC_DRAW);
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
	float vertex[] = {
		b->button.base.x, b->button.base.y,
		b->button.base.x + b->button.base.width, b->button.base.y,
		b->button.base.x, b->button.base.y + b->button.base.height,
		b->button.base.x + b->button.base.width, b->button.base.y + b->button.base.height
	};
	float texcoord[] = {
		GET_TEX_S(TEX_FULL_WIDTH, d->tx, 0), GET_TEX_T(TEX_FULL_HEIGHT, d->ty, 0),
		GET_TEX_S(TEX_FULL_WIDTH, d->tx, d->tw), GET_TEX_T(TEX_FULL_HEIGHT, d->ty, 0),
		GET_TEX_S(TEX_FULL_WIDTH, d->tx, 0), GET_TEX_T(TEX_FULL_HEIGHT, d->ty, d->th),
		GET_TEX_S(TEX_FULL_WIDTH, d->tx, d->tw), GET_TEX_T(TEX_FULL_HEIGHT, d->ty, d->th),

		GET_TEX_S(TEX_FULL_WIDTH, d->ptx, 0), GET_TEX_T(TEX_FULL_HEIGHT, d->pty, 0),
		GET_TEX_S(TEX_FULL_WIDTH, d->ptx, d->ptw), GET_TEX_T(TEX_FULL_HEIGHT, d->pty, 0),
		GET_TEX_S(TEX_FULL_WIDTH, d->ptx, 0), GET_TEX_T(TEX_FULL_HEIGHT, d->pty, d->pth),
		GET_TEX_S(TEX_FULL_WIDTH, d->ptx, d->ptw), GET_TEX_T(TEX_FULL_HEIGHT, d->pty, d->pth)
	};
	b->button.base.buffers[Position_Coord] = karinNewBuffer(GL_ARRAY_BUFFER, sizeof(float) * 8, vertex, GL_STATIC_DRAW);
	b->button.base.buffers[Texture_Coord] = karinNewBuffer(GL_ARRAY_BUFFER, sizeof(float) * 16, texcoord, GL_STATIC_DRAW);
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
	if(!qglIsTexture(tex[b->base.tex_index].imaged))
		return;
	if(!b->base.visible)
		return;
	if(!qglIsBuffer(b->base.buffers[Texture_Coord]) || !qglIsBuffer(b->base.buffers[Position_Coord]))
		return;
	qglBindTexture(GL_TEXTURE_2D, tex[b->base.tex_index].imaged);
	qglBindBuffer(GL_ARRAY_BUFFER, b->base.buffers[Texture_Coord]);
	GLvoid *ptr = (b->base.pressed && b->base.enabled) ? (float *)NULL + 8 : NULL;
	qglTexCoordPointer(2, GL_FLOAT, 0, ptr);
	qglBindBuffer(GL_ARRAY_BUFFER, b->base.buffers[Position_Coord]);
	qglVertexPointer(2, GL_FLOAT, 0, NULL);
	qglDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	qglBindBuffer(GL_ARRAY_BUFFER, 0);
	qglBindTexture(GL_TEXTURE_2D, 0);
}

void karinRenderVKBSwipe(const virtual_swipe *b, const texture const tex[])
{
	if(!b)
		return;
	if(b->base.type != vkb_swipe_type)
		return;
	if(!qglIsTexture(tex[b->base.tex_index].imaged))
		return;
	if(!b->base.visible)
		return;
	if(!qglIsBuffer(b->base.buffers[Texture_Coord]) || !qglIsBuffer(b->base.buffers[Position_Coord]))
		return;
	qglBindTexture(GL_TEXTURE_2D, tex[b->base.tex_index].imaged);
	qglBindBuffer(GL_ARRAY_BUFFER, b->base.buffers[Texture_Coord]);
	qglTexCoordPointer(2, GL_FLOAT, 0, NULL);
	qglBindBuffer(GL_ARRAY_BUFFER, b->base.buffers[Position_Coord]);
	qglVertexPointer(2, GL_FLOAT, 0, NULL);
	qglDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	qglBindBuffer(GL_ARRAY_BUFFER, 0);
	qglBindTexture(GL_TEXTURE_2D, 0);
}

void karinRenderVKBJoystick(const virtual_joystick *b, const texture const tex[])
{
	if(!b)
		return;
	if(b->base.type != vkb_joystick_type)
		return;
	if(!qglIsTexture(tex[b->base.tex_index].imaged))
		return;
	if(!b->base.visible)
		return;
	if(!qglIsBuffer(b->base.buffers[Texture_Coord]) || !qglIsBuffer(b->base.buffers[Position_Coord]))
		return;
	qglBindTexture(GL_TEXTURE_2D, tex[b->base.tex_index].imaged);
	qglBindBuffer(GL_ARRAY_BUFFER, b->base.buffers[Texture_Coord]);
	qglTexCoordPointer(2, GL_FLOAT, 0, NULL);
	qglBindBuffer(GL_ARRAY_BUFFER, b->base.buffers[Position_Coord]);
	qglVertexPointer(2, GL_FLOAT, 0, NULL);
	qglDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	float cx = b->base.x + b->base.width / 2;
	float cy = b->base.y + b->base.height / 2;
	qglPushMatrix();
	{
		qglTranslatef(cx + b->pos_x, cy + b->pos_y, 0.0f);
		qglDrawArrays(GL_TRIANGLE_STRIP, 4, 4);
	}
	qglPopMatrix();

	qglBindBuffer(GL_ARRAY_BUFFER, 0);
	qglBindTexture(GL_TEXTURE_2D, 0);
}

void karinRenderVKBCursor(const virtual_cursor *b, const texture tex[])
{
	if(!b)
		return;
	if(b->base.type != vkb_cursor_type)
		return;
	if(!qglIsTexture(tex[b->base.tex_index].imaged))
		return;
	if(!b->base.visible)
		return;
	if(!qglIsBuffer(b->base.buffers[Texture_Coord]) || !qglIsBuffer(b->base.buffers[Position_Coord]))
		return;
	qglBindTexture(GL_TEXTURE_2D, tex[b->base.tex_index].imaged);
	if(b->render_bg)
	{
		qglBindBuffer(GL_ARRAY_BUFFER, b->base.buffers[Texture_Coord]);
		qglTexCoordPointer(2, GL_FLOAT, 0, (float *)NULL + 16);
		qglBindBuffer(GL_ARRAY_BUFFER, b->base.buffers[Position_Coord]);
		qglVertexPointer(2, GL_FLOAT, 0, (float *)NULL + 8);

		qglDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	qglBindBuffer(GL_ARRAY_BUFFER, b->base.buffers[Texture_Coord]);
	GLvoid *ptr = (b->base.pressed && b->base.enabled) ? (float *)NULL + 8 : NULL;
	qglTexCoordPointer(2, GL_FLOAT, 0, ptr);
	qglBindBuffer(GL_ARRAY_BUFFER, b->base.buffers[Position_Coord]);
	qglVertexPointer(2, GL_FLOAT, 0, NULL);

	float cx = b->base.x + b->base.width / 2;
	float cy = b->base.y + b->base.height / 2;
	qglPushMatrix();
	{
		qglTranslatef(cx + b->pos_x, cy + b->pos_y, 0.0f);
		qglDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
	qglPopMatrix();

	qglBindBuffer(GL_ARRAY_BUFFER, 0);
	qglBindTexture(GL_TEXTURE_2D, 0);
}

