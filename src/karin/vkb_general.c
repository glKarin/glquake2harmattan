/*
 * This file is private for gl_vkb1.c and sdl_vkb.c.
 * Only for including.
 * Don't use it single.
 * Struct named _texture and _vkb is different for OpenGL and SDL.
 * if want, using union for the difference.
 * imaged_t: image type, 
 *   OpenGL is a texture ID(GLuint).
 *   SDL is a surface(SDL_Surface *)
 *
 * buffer_t: coord type,
 *   OpenGL is array buffer ID(GLuint)
 *   SDL is short array, length is not sure, for constructing SDL_Rect(short *).
 * */

#define MOUSE_IN_RANGE(b, x, y) \
	((x >= (b).e_min_x && x <= (b).e_max_x ) && (y >= (b).e_min_y && y <= (b).e_max_y))

typedef struct _texture
{
	imaged_t imaged;
	sizei width;
	sizei height;
	uenum format;
} texture;

typedef struct _virtual_control_base
{
	vkb_type type;
	float x;
	float y;
	int z;
	float width;
	float height;
	float e_min_x;
	float e_min_y;
	float e_max_x;
	float e_max_y;
	boolean pressed;
	boolean enabled;
	boolean visible;
	uint tex_index;
	buffer_t buffers[Total_Coord];
	uint show_mask;
} virtual_control_base;

typedef struct _virtual_cursor
{
	virtual_control_base base;

	Game_Action action;
	Game_Action actions[4];
	uint mask[4];
	float angle;
	float distance;
	float ignore_distance;
	float e_radius;
	float pos_x;
	float pos_y;
	boolean render_bg;
} virtual_cursor;

typedef struct _virtual_swipe
{
	virtual_control_base base;

	Game_Action action;
	Game_Action actions[4];
	uint mask[4];
	float angle;
	float distance;
	float ignore_distance;
} virtual_swipe;

typedef struct _virtual_joystick
{
	virtual_control_base base;

	Game_Action actions[4];
	float e_radius;
	float angle;
	clampf percent;
	float e_ignore_radius;
	float pos_x;
	float pos_y;
} virtual_joystick;

typedef struct _virtual_button
{
	virtual_control_base base;

	Game_Action action;
} virtual_button;

typedef union _virtual_control_item
{
	vkb_type type;
	virtual_control_base base;
	virtual_cursor cursor;
	virtual_swipe swipe;
	virtual_joystick joystick;
	virtual_button button;
} virtual_control_item;

typedef struct _vkb
{
	float x;
	float y;
	float z;
	float width;
	float height;
	boolean inited;
	texture tex[VKB_TEX_COUNT];
	virtual_control_item vb[TOTAL_VKB_COUNT];
} vkb;

static void karinMakeJoystick(virtual_control_item *b, struct vkb_joystick *joystick, unsigned int width, unsigned int height);
static void karinMakeSwipe(virtual_control_item *b, struct vkb_swipe *swipe, unsigned int width, unsigned int height);
static void karinMakeButton(virtual_control_item *b, struct vkb_button *btn, unsigned int width, unsigned int height);
static void karinMakeCursor(virtual_control_item *b, struct vkb_cursor *cursor, unsigned int width, unsigned int height);
static void karinRenderVKBCursor(const virtual_cursor *b, const texture tex[]);
static void karinRenderVKBJoystick(const virtual_joystick *b, const texture tex[]);
static void karinRenderVKBSwipe(const virtual_swipe *b, const texture tex[]);
static void karinRenderVKBButton(const virtual_button *b, const texture tex[]);

static vkb the_vkb;

//void karinResizeVKB(float w, float h);
static mouse_motion_button_status karinButtonMouseMotion(const virtual_control_item *b, int nx, int ny, int lx, int ly)
{
	if(!b)
		return all_out_range_status;
	int	l = MOUSE_IN_RANGE(b->base, lx, ly) ? 1 : 0;
	int	n = MOUSE_IN_RANGE(b->base, nx, ny) ? 1 : 0;
	mouse_motion_button_status s = all_out_range_status;
	if(l)
		s |= last_in_now_out_range_status;
	if(n)
		s |= last_out_now_in_range_status;
	return s;
}

static circle_direction karinGetJoystickDirection(int x, int y, float cx, float cy, float ir, float or, float *angle, float *percent)
{
	int oh = or / 2;
	int ih = ir / 2;
	int xl = x - cx;
	int yl = y - cy;
	int xls = xl >= 0 ? xl : -xl;
	int yls = yl >= 0 ? yl : -yl;
	if(xls > oh || yls > oh)
	{
		if(angle)
			*angle = -1.0f;
		if(percent)
			*percent = 0.0f;
		return circle_outside;
	}
	else if(xls < ih && yls < ih)
	{
		if(angle)
			*angle = -1.0f;
		if(percent)
			*percent = 0.0f;
		return circle_center;
	}

	double a = (double)(xl);
	double b = (double)(yl);
	//float ra = karinFormatAngle(atan2(xl, yl) * (180.0 / M_PI));
	float rb = karinFormatAngle(atan2(b, a) * (180.0 / M_PI));
	if(angle)
		*angle = rb;
	if(percent)
	{
		double c = sqrt(a * a + b * b);
		*percent = (float)c / oh;
	}
	//printf("%f - %f\n", ra, rb);
	if(rb >= 60 && rb <= 120)
		return circle_top_direction;
	else if((rb >= 0 && rb <= 30) || (rb >= 330 && rb <= 360))
		return circle_right_direction;
	else if(rb >= 240 && rb <= 300)
		return circle_bottom_direction;
	else if(rb >= 150 && rb <= 210)
		return circle_left_direction;

	else if(rb > 30 && rb < 60)
		return circle_righttop_direction;
	else if(rb > 300 && rb < 330)
		return circle_rightbottom_direction;
	else if(rb > 210 && rb < 240)
		return circle_leftbottom_direction;
	else
		return circle_lefttop_direction;
}

static circle_direction karinGetSwipeDirection(int dx, int dy, float *angle, float *distance)
{
	if(dx == 0 && dy == 0)
	{
		if(distance)
			*distance = 0.0f;
		if(*angle)
			*angle = -1.0f;
		return circle_center;
	}
	double a = (double)(dx);
	double b = (double)(dy);
	double c = sqrt(a * a + b * b);
	if(distance)
		*distance = (float)c;
	//float ra = karinFormatAngle(atan2(xl, yl) * (180.0 / M_PI));
	float rb = karinFormatAngle(atan2(b, a) * (180.0 / M_PI));
	if(*angle)
		*angle = rb;
	//printf("%f - %f\n", ra, rb);
	if(rb >= 60 && rb <= 120)
		return circle_top_direction;
	else if((rb >= 0 && rb <= 30) || (rb >= 330 && rb <= 360))
		return circle_right_direction;
	else if(rb >= 240 && rb <= 300)
		return circle_bottom_direction;
	else if(rb >= 150 && rb <= 210)
		return circle_left_direction;

	else if(rb > 30 && rb < 60)
		return circle_righttop_direction;
	else if(rb > 300 && rb < 330)
		return circle_rightbottom_direction;
	else if(rb > 210 && rb < 240)
		return circle_leftbottom_direction;
	else
		return circle_lefttop_direction;
}

static int karinGetControlItemOnPosition(int x, int y, int *r)
{
	if(!the_vkb.inited)
		return -1;
	int c = 0;
	int i;
	int count = TOTAL_VKB_COUNT;
	for(i = 0; i < count; i++)
	{
		const virtual_control_item *b = the_vkb.vb + i;
		if((b->base.show_mask & VKB_States[client_state]) == 0)
			continue;
		if(!b->base.enabled)
			continue;
		if(MOUSE_IN_RANGE(b->base, x, y))
		{
			if(r)
				r[c] = i;
			c++;
		}
	}
	if(r)
	{
		int j;
		for(j = 0; j < c; j++)
		{
			int k;
			for(k = j + 1; k < c; k++)
			{
				const virtual_control_item *a = the_vkb.vb + r[j];
				const virtual_control_item *b = the_vkb.vb + r[k];
				if(b->base.z > a->base.z)
				{
					int t = r[j];
					r[j] = r[k];
					r[k] = t;
				}
			}
		}
	}
	return c;
}

static int karinGetControlItemOnPosition2(int x, int y, int lx, int ly, int *r, mouse_motion_button_status *s)
{
	if(!the_vkb.inited)
		return -1;
	int c = 0;
	int i;
	int count = TOTAL_VKB_COUNT;
	for(i = 0; i < count; i++)
	{
		const virtual_control_item *b = the_vkb.vb + i;
		if((b->base.show_mask & VKB_States[client_state]) == 0)
			continue;
		if(!b->base.enabled)
			continue;
		mouse_motion_button_status status = karinButtonMouseMotion(b, x, y, lx, ly);
		if(status == all_out_range_status)
			continue;
		if(r)
			r[c] = i;
		if(s)
			s[c] = status;
		c++;
	}
	if(r && s)
	{
		int j;
		for(j = 0; j < c; j++)
		{
			int k;
			for(k = j + 1; k < c; k++)
			{
				const virtual_control_item *a = the_vkb.vb + r[j];
				const virtual_control_item *b = the_vkb.vb + r[k];
				if(b->base.z > a->base.z)
				{
					int t = r[j];
					r[j] = r[k];
					r[k] = t;
					mouse_motion_button_status tt = s[j];
					s[j] = s[k];
					s[k] = tt;
				}
			}
		}
	}
	return c;
}

static unsigned karinVKBMouseMotionEvent(int b, int p, int x, int y, int dx, int dy, VKB_Key_Action_Function f)
{
	if(!the_vkb.inited)
		return 0;
	if(!p)
		return 0;
	int midx = dx * 2;
	int midy = dy * 2;
	int nres = 0;
	int last_x = x - dx;
	int last_y = y - dy;
	int last_nz = INT_MIN;
	int clicked[TOTAL_VKB_COUNT];
	mouse_motion_button_status status[TOTAL_VKB_COUNT];
	int count = karinGetControlItemOnPosition2(x, y, last_x, last_y, clicked, status);
	if(count > 0)
	{
		int i;
		for(i = 0; i < count; i++)
		{
			virtual_control_item *b = the_vkb.vb + clicked[i];
			mouse_motion_button_status s = status[i];
			// normal button
			//printf("%d  %d %d\n", i, b->type, b->base.z);
			if(b->type == vkb_button_type)
			{
				/*
					 if(s == last_out_now_in_range_status)
					 {
					 if(nres && b->button.base.z < last_nz)
					 break;
					 last_nz = b->button.base.z;
					 b->base.pressed = btrue;
					 if(f)
					 f(b->button.action, b->button.base.pressed, 0, 0);
					 nres = 1;
					 }
					 else 
					 */
				if(s == last_in_now_out_range_status)
				{
					b->button.base.pressed = bfalse;
					if(f)
						f(b->button.action, bfalse, 0, 0);
				}
				else if(s == all_in_range_status)
				{
					if(nres && b->button.base.z < last_nz)
						break;
					last_nz = b->button.base.z;
					nres = 1;
				}
			}

			// joystick
			else if(b->type == vkb_joystick_type)
			{
				if(s == last_out_now_in_range_status)
				{
					if(nres && b->joystick.base.z < last_nz)
						break;
					last_nz = b->joystick.base.z;
					b->joystick.base.pressed = btrue;
					float cx = b->joystick.base.x + b->joystick.base.width / 2;
					float cy = b->joystick.base.y + b->joystick.base.height / 2;
					circle_direction d = karinGetJoystickDirection(x, y, cx, cy, b->joystick.e_ignore_radius, b->joystick.e_radius, &b->joystick.angle, &b->joystick.percent);
					if(d != circle_outside)
					{
						b->joystick.pos_x = x - cx;
						b->joystick.pos_y = y - cy;
					}
					else
					{
						b->joystick.pos_x = 0;
						b->joystick.pos_y = 0;
					}
					if(f)
					{
						switch(d)
						{
							case circle_top_direction:
								f(b->joystick.actions[0], btrue, midx, midy);
								break;
							case circle_bottom_direction:
								f(b->joystick.actions[1], btrue, midx, midy);
								break;
							case circle_right_direction:
								f(b->joystick.actions[3], btrue, midx, midy);
								break;
							case circle_left_direction:
								f(b->joystick.actions[2], btrue, midx, midy);
								break;
							case circle_lefttop_direction:
								f(b->joystick.actions[0], btrue, midx, midy);
								f(b->joystick.actions[2], btrue, midx, midy);
								break;
							case circle_leftbottom_direction:
								f(b->joystick.actions[1], btrue, midx, midy);
								f(b->joystick.actions[2], btrue, midx, midy);
								break;
							case circle_righttop_direction:
								f(b->joystick.actions[0], btrue, midx, midy);
								f(b->joystick.actions[3], btrue, midx, midy);
								break;
							case circle_rightbottom_direction:
								f(b->joystick.actions[1], btrue, midx, midy);
								f(b->joystick.actions[3], btrue, midx, midy);
								break;
							default:
								break;
						}
					}
					nres = 1;
				}
				else if(s == last_in_now_out_range_status)
				{
					b->joystick.base.pressed = bfalse;
					if(f)
					{
						float cx = b->joystick.base.x + b->joystick.base.width / 2;
						float cy = b->joystick.base.y + b->joystick.base.height / 2;
						circle_direction d = karinGetJoystickDirection(last_x, last_y, cx, cy, b->joystick.e_ignore_radius, b->joystick.e_radius, NULL, NULL);
						switch(d)
						{
							case circle_top_direction:
								f(b->joystick.actions[0], bfalse, midx, midy);
								break;
							case circle_bottom_direction:
								f(b->joystick.actions[1], bfalse, midx, midy);
								break;
							case circle_right_direction:
								f(b->joystick.actions[3], bfalse, midx, midy);
								break;
							case circle_left_direction:
								f(b->joystick.actions[2], bfalse, midx, midy);
								break;
							case circle_lefttop_direction:
								f(b->joystick.actions[0], bfalse, midx, midy);
								f(b->joystick.actions[2], bfalse, midx, midy);
								break;
							case circle_leftbottom_direction:
								f(b->joystick.actions[1], bfalse, midx, midy);
								f(b->joystick.actions[2], bfalse, midx, midy);
								break;
							case circle_righttop_direction:
								f(b->joystick.actions[0], bfalse, midx, midy);
								f(b->joystick.actions[3], bfalse, midx, midy);
								break;
							case circle_rightbottom_direction:
								f(b->joystick.actions[1], bfalse, midx, midy);
								f(b->joystick.actions[3], bfalse, midx, midy);
								break;
							default:
								break;
						}
					}
					b->joystick.angle = -1.0f;
					b->joystick.percent = 0.0f;
					b->joystick.pos_x = 0.0f;
					b->joystick.pos_y = 0.0f;
				}
				else if(s == all_in_range_status)
				{
					if(nres && b->joystick.base.z < last_nz)
					{
						b->joystick.base.pressed = bfalse;
						if(f)
						{
							float cx = b->joystick.base.x + b->joystick.base.width / 2;
							float cy = b->joystick.base.y + b->joystick.base.height / 2;
							circle_direction d = karinGetJoystickDirection(last_x, last_y, cx, cy, b->joystick.e_ignore_radius, b->joystick.e_radius, NULL, NULL);
							switch(d)
							{
								case circle_top_direction:
									f(b->joystick.actions[0], bfalse, midx, midy);
									break;
								case circle_bottom_direction:
									f(b->joystick.actions[1], bfalse, midx, midy);
									break;
								case circle_right_direction:
									f(b->joystick.actions[3], bfalse, midx, midy);
									break;
								case circle_left_direction:
									f(b->joystick.actions[2], bfalse, midx, midy);
									break;
								case circle_lefttop_direction:
									f(b->joystick.actions[0], bfalse, midx, midy);
									f(b->joystick.actions[2], bfalse, midx, midy);
									break;
								case circle_leftbottom_direction:
									f(b->joystick.actions[1], bfalse, midx, midy);
									f(b->joystick.actions[2], bfalse, midx, midy);
									break;
								case circle_righttop_direction:
									f(b->joystick.actions[0], bfalse, midx, midy);
									f(b->joystick.actions[3], bfalse, midx, midy);
									break;
								case circle_rightbottom_direction:
									f(b->joystick.actions[1], bfalse, midx, midy);
									f(b->joystick.actions[3], bfalse, midx, midy);
									break;
								default:
									break;
							}
						}
						b->joystick.angle = -1.0f;
						b->joystick.percent = 0.0f;
						b->joystick.pos_x = 0.0f;
						b->joystick.pos_y = 0.0f;
					}
					else
					{
						last_nz = b->joystick.base.z;
						float cx = b->joystick.base.x + b->joystick.base.width / 2;
						float cy = b->joystick.base.y + b->joystick.base.height / 2;
						circle_direction ld = karinGetJoystickDirection(last_x, last_y, cx, cy, b->joystick.e_ignore_radius, b->joystick.e_radius, NULL, NULL);
						circle_direction d = karinGetJoystickDirection(x, y, cx, cy, b->joystick.e_ignore_radius, b->joystick.e_radius, &b->joystick.angle, &b->joystick.percent);
						if(d != ld)
						{
							if(f)
							{
								int mask[4] = {0, 0, 0, 0};
								switch(ld)
								{
									case circle_top_direction:
										mask[0] -= 1;
										break;
									case circle_bottom_direction:
										mask[1] -= 1;
										break;
									case circle_right_direction:
										mask[3] -= 1;
										break;
									case circle_left_direction:
										mask[2] -= 1;
										break;
									case circle_lefttop_direction:
										mask[0] -= 1;
										mask[2] -= 1;
										break;
									case circle_leftbottom_direction:
										mask[1] -= 1;
										mask[2] -= 1;
										break;
									case circle_righttop_direction:
										mask[0] -= 1;
										mask[3] -= 1;
										break;
									case circle_rightbottom_direction:
										mask[1] -= 1;
										mask[3] -= 1;
										break;
									default:
										break;
								}
								switch(d)
								{
									case circle_top_direction:
										mask[0] += 1;
										break;
									case circle_bottom_direction:
										mask[1] += 1;
										break;
									case circle_right_direction:
										mask[3] += 1;
										break;
									case circle_left_direction:
										mask[2] += 1;
										break;
									case circle_lefttop_direction:
										mask[0] += 1;
										mask[2] += 1;
										break;
									case circle_leftbottom_direction:
										mask[1] += 1;
										mask[2] += 1;
										break;
									case circle_righttop_direction:
										mask[0] += 1;
										mask[3] += 1;
										break;
									case circle_rightbottom_direction:
										mask[1] += 1;
										mask[3] += 1;
										break;
									default:
										break;
								}
								if(mask[0] != 0)
								{
									f(b->joystick.actions[0], mask[0] > 0 ? btrue : bfalse, midx, midy);
								}
								if(mask[1] != 0)
								{
									f(b->joystick.actions[1], mask[1] > 0 ? btrue : bfalse, midx, midy);
								}
								if(mask[2] != 0)
								{
									f(b->joystick.actions[2], mask[2] > 0 ? btrue : bfalse, midx, midy);
								}
								if(mask[3] != 0)
								{
									f(b->joystick.actions[3], mask[3] > 0 ? btrue : bfalse, midx, midy);
								}
							}
						}
						if(d != circle_outside)
						{
							b->joystick.pos_x = x - cx;
							b->joystick.pos_y = y - cy;
						}
						else
						{
							b->joystick.pos_x = 0;
							b->joystick.pos_y = 0;
						}
						nres = 1;
					}
				}
			}

			// swipe
			else if(b->type == vkb_swipe_type)
			{
				if(s == last_out_now_in_range_status)
				{
					if(nres && b->swipe.base.z < last_nz)
						break;
					last_nz = b->swipe.base.z; // ??? down
					/*
					if(!b->swipe.base.pressed) // ???
					{
						if(f)
							f(b->swipe.action, btrue, 0, 0);
					}
					*/
					b->swipe.base.pressed = btrue;
					b->swipe.distance = 0.0f;
					b->swipe.angle = -1.0f;
					nres = 1;
				}
				else if(s == last_in_now_out_range_status)
				{
					/*
					if(b->swipe.base.pressed)
					{
						if(f)
							f(b->swipe.action, bfalse, 0, 0);
					}
					*/
					b->swipe.base.pressed = bfalse;
					if(b->swipe.mask[0] > 0)
					{
						if(f)
							f(b->swipe.actions[0], bfalse, midx, midy);
						b->swipe.mask[0] = 0;
					}
					if(b->swipe.mask[1] > 0)
					{
						if(f)
							f(b->swipe.actions[1], bfalse, midx, midy);
						b->swipe.mask[1] = 0;
					}
					if(b->swipe.mask[2] > 0)
					{
						if(f)
							f(b->swipe.actions[2], bfalse, midx, midy);
						b->swipe.mask[2] = 0;
					}
					if(b->swipe.mask[3] > 0)
					{
						if(f)
							f(b->swipe.actions[3], bfalse, midx, midy);
						b->swipe.mask[3] = 0;
					}
					b->swipe.distance = 0.0f;
					b->swipe.angle = -1.0f;
				}
				else if(s == all_in_range_status)
				{
					if(nres && b->swipe.base.z < last_nz)
					{
						b->swipe.base.pressed = bfalse;
						if(b->swipe.mask[0] > 0)
						{
							if(f)
								f(b->swipe.actions[0], bfalse, midx, midy);
							b->swipe.mask[0] = 0;
						}
						if(b->swipe.mask[1] > 0)
						{
							if(f)
								f(b->swipe.actions[1], bfalse, midx, midy);
							b->swipe.mask[1] = 0;
						}
						if(b->swipe.mask[2] > 0)
						{
							if(f)
								f(b->swipe.actions[2], bfalse, midx, midy);
							b->swipe.mask[2] = 0;
						}
						if(b->swipe.mask[3] > 0)
						{
							if(f)
								f(b->swipe.actions[3], bfalse, midx, midy);
							b->swipe.mask[3] = 0;
						}
						b->swipe.distance = 0.0f;
						b->swipe.angle = -1.0f;
					}
					else
					{
						last_nz = b->swipe.base.z;
						b->swipe.base.pressed = btrue;
						circle_direction d = karinGetSwipeDirection(dx, dy, &b->swipe.angle, &b->swipe.distance);
						if(d != circle_center)
						{
							if(b->swipe.distance < b->swipe.ignore_distance)
								d = circle_center;
						}
						if(f)
						{
							uint mask[4] = {0, 0, 0, 0};
							switch(d)
							{
								case circle_top_direction:
									mask[0] += 1;
									break;
								case circle_bottom_direction:
									mask[1] += 1;
									break;
								case circle_right_direction:
									mask[3] += 1;
									break;
								case circle_left_direction:
									mask[2] += 1;
									break;
								case circle_lefttop_direction:
									mask[0] += 1;
									mask[2] += 1;
									break;
								case circle_leftbottom_direction:
									mask[1] += 1;
									mask[2] += 1;
									break;
								case circle_righttop_direction:
									mask[0] += 1;
									mask[3] += 1;
									break;
								case circle_rightbottom_direction:
									mask[1] += 1;
									mask[3] += 1;
									break;
								default:
									break;
							}
							if(b->swipe.mask[0] != 0 || mask[0] != 0)
							{
								f(b->swipe.actions[0], mask[0] >= b->swipe.mask[0] ? btrue : bfalse, midx, midy);
								b->swipe.mask[0] = mask[0];
								nres = 1;
							}
							if(b->swipe.mask[1] != 0 || mask[1] != 0)
							{
								f(b->swipe.actions[1], mask[1] >= b->swipe.mask[1] ? btrue : bfalse, midx, midy);
								b->swipe.mask[1] = mask[1];
								nres = 1;
							}
							if(b->swipe.mask[2] != 0 || mask[2] != 0)
							{
								f(b->swipe.actions[2], mask[2] >= b->swipe.mask[2] ? btrue : bfalse, midx, midy);
								b->swipe.mask[2] = mask[2];
								nres = 1;
							}
							if(b->swipe.mask[3] != 0 || mask[3] != 0)
							{
								f(b->swipe.actions[3], mask[3] >= b->swipe.mask[3] ? btrue : bfalse, midx, midy);
								b->swipe.mask[3] = mask[3];
								nres = 1;
							}
						}
					}
				}
			}

			// cursor
			else if(b->type == vkb_cursor_type)
			{
				if(s == last_in_now_out_range_status)
				{
					b->cursor.base.pressed = bfalse;
					if(f)
						f(b->cursor.action, bfalse, midx, midy);
					if(b->cursor.mask[0] > 0)
					{
						if(f)
							f(b->cursor.actions[0], bfalse, midx, midy);
						b->cursor.mask[0] = 0;
					}
					if(b->cursor.mask[1] > 0)
					{
						if(f)
							f(b->cursor.actions[1], bfalse, midx, midy);
						b->cursor.mask[1] = 0;
					}
					if(b->cursor.mask[2] > 0)
					{
						if(f)
							f(b->cursor.actions[2], bfalse, midx, midy);
						b->cursor.mask[2] = 0;
					}
					if(b->cursor.mask[3] > 0)
					{
						if(f)
							f(b->cursor.actions[3], bfalse, midx, midy);
						b->cursor.mask[3] = 0;
					}
					b->cursor.distance = 0.0f;
					b->cursor.angle = -1.0f;
					b->cursor.pos_x = 0;
					b->cursor.pos_y = 0;
				}
				else if(s == all_in_range_status)
				{
					if(nres && b->cursor.base.z < last_nz)
					{
						if(b->cursor.mask[0] > 0)
						{
							if(f)
								f(b->cursor.actions[0], bfalse, midx, midy);
							b->cursor.mask[0] = 0;
						}
						if(b->cursor.mask[1] > 0)
						{
							if(f)
								f(b->cursor.actions[1], bfalse, midx, midy);
							b->cursor.mask[1] = 0;
						}
						if(b->cursor.mask[2] > 0)
						{
							if(f)
								f(b->cursor.actions[2], bfalse, midx, midy);
							b->cursor.mask[2] = 0;
						}
						if(b->cursor.mask[3] > 0)
						{
							if(f)
								f(b->cursor.actions[3], bfalse, midx, midy);
							b->cursor.mask[3] = 0;
						}
						b->cursor.distance = 0.0f;
						b->cursor.angle = -1.0f;
						b->cursor.pos_x = 0;
						b->cursor.pos_y = 0;
						b->cursor.base.pressed = bfalse;
						if(f)
							f(b->cursor.action, bfalse, midx, midy);
					}
					else
					{
						if(b->cursor.base.pressed)
						{
							last_nz = b->cursor.base.z;
							circle_direction d = karinGetSwipeDirection(dx, dy, &b->cursor.angle, &b->cursor.distance);
							if(d != circle_center)
							{
								if(b->cursor.distance < b->cursor.ignore_distance)
									d = circle_center;
							}
							if(f)
							{
								uint mask[4] = {0, 0, 0, 0};
								switch(d)
								{
									case circle_top_direction:
										mask[0] += 1;
										break;
									case circle_bottom_direction:
										mask[1] += 1;
										break;
									case circle_right_direction:
										mask[3] += 1;
										break;
									case circle_left_direction:
										mask[2] += 1;
										break;
									case circle_lefttop_direction:
										mask[0] += 1;
										mask[2] += 1;
										break;
									case circle_leftbottom_direction:
										mask[1] += 1;
										mask[2] += 1;
										break;
									case circle_righttop_direction:
										mask[0] += 1;
										mask[3] += 1;
										break;
									case circle_rightbottom_direction:
										mask[1] += 1;
										mask[3] += 1;
										break;
									default:
										break;
								}
								if(b->cursor.mask[0] != 0 || mask[0] != 0)
								{
									if(f)
										f(b->cursor.actions[0], mask[0] >= b->cursor.mask[0] ? btrue : bfalse, midx, midy);
									b->cursor.mask[0] = mask[0];
									nres = 1;
								}
								if(b->cursor.mask[1] != 0 || mask[1] != 0)
								{
									if(f)
										f(b->cursor.actions[1], mask[1] >= b->cursor.mask[1] ? btrue : bfalse, midx, midy);
									b->cursor.mask[1] = mask[1];
									nres = 1;
								}
								if(b->cursor.mask[2] != 0 || mask[2] != 0)
								{
									if(f)
										f(b->cursor.actions[2], mask[2] >= b->cursor.mask[2] ? btrue : bfalse, midx, midy);
									b->cursor.mask[2] = mask[2];
									nres = 1;
								}
								if(b->cursor.mask[3] != 0 || mask[3] != 0)
								{
									if(f)
										f(b->cursor.actions[3], mask[3] >= b->cursor.mask[3] ? btrue : bfalse, midx, midy);
									b->cursor.mask[3] = mask[3];
									nres = 1;
								}
							}
							float cx = b->cursor.base.x + b->cursor.base.width / 2;
							float cy = b->cursor.base.y + b->cursor.base.height / 2;
							b->cursor.pos_x = x - cx;
							b->cursor.pos_y = y - cy;
						}
					}
				}
			}
		}
		if(nres)
			return 1;
	}
	return 0;
}

static unsigned karinVKBMouseEvent(int b, int p, int x, int y, VKB_Key_Action_Function f)
{
	if(!the_vkb.inited)
		return 0;
	int last_z = INT_MIN;
	int res = 0;
	int clicked[TOTAL_VKB_COUNT];
	int count = karinGetControlItemOnPosition(x, y, clicked);
	if(count > 0)
	{
		int i;
		for(i = 0; i < count; i++)
		{
			virtual_control_item *b = the_vkb.vb + clicked[i];
			if(b->type == vkb_joystick_type)
			{
				boolean pr = bfalse;
				if(res && b->base.z < last_z)
				{
					pr = bfalse;
				}
				else
				{
					pr = p ? btrue : bfalse;
					last_z = b->base.z;
					res = 1;
				}
				b->joystick.base.pressed = pr;
				float cx = b->joystick.base.x + b->joystick.base.width / 2;
				float cy = b->joystick.base.y + b->joystick.base.width / 2;
				circle_direction d = karinGetJoystickDirection(x, y, cx, cy, b->joystick.e_ignore_radius, b->joystick.e_radius, &b->joystick.angle, &b->joystick.percent);
				if(f)
				{
					switch(d)
					{
						case circle_top_direction:
							f(b->joystick.actions[0], b->joystick.base.pressed, 0, 0);
							break;
						case circle_bottom_direction:
							f(b->joystick.actions[1], b->joystick.base.pressed, 0, 0);
							break;
						case circle_right_direction:
							f(b->joystick.actions[3], b->joystick.base.pressed, 0, 0);
							break;
						case circle_left_direction:
							f(b->joystick.actions[2], b->joystick.base.pressed, 0, 0);
							break;
						case circle_lefttop_direction:
							f(b->joystick.actions[0], b->joystick.base.pressed, 0, 0);
							f(b->joystick.actions[2], b->joystick.base.pressed, 0, 0);
							break;
						case circle_leftbottom_direction:
							f(b->joystick.actions[1], b->joystick.base.pressed, 0, 0);
							f(b->joystick.actions[2], b->joystick.base.pressed, 0, 0);
							break;
						case circle_righttop_direction:
							f(b->joystick.actions[0], b->joystick.base.pressed, 0, 0);
							f(b->joystick.actions[3], b->joystick.base.pressed, 0, 0);
							break;
						case circle_rightbottom_direction:
							f(b->joystick.actions[1], b->joystick.base.pressed, 0, 0);
							f(b->joystick.actions[3], b->joystick.base.pressed, 0, 0);
							break;
						default:
							break;
					}
				}
				if(!pr)
				{
					b->joystick.angle = -1.0f;
					b->joystick.percent = 0.0f;
					b->joystick.pos_x = 0.0f;
					b->joystick.pos_y = 0.0f;
				}
				else
				{
					if(d != circle_outside)
					{
						b->joystick.pos_x = x - cx;
						b->joystick.pos_y = y - cy;
					}
					else
					{
						b->joystick.pos_x = 0;
						b->joystick.pos_y = 0;
					}
				}
			}
			else if(b->type == vkb_button_type)
			{
				boolean pr = bfalse;
				if(res && b->base.z < last_z)
				{
					pr = bfalse;
				}
				else
				{
					pr = p ? btrue : bfalse;
					last_z = b->base.z;
					res = 1;
				}
				b->button.base.pressed = pr;
				if(f)
					f(b->button.action, b->button.base.pressed, 0, 0);
			}
			else if(b->type == vkb_swipe_type)
			{
				boolean pr = bfalse;
				if(res && b->base.z < last_z)
				{
					pr = bfalse;
				}
				else
				{
					pr = p ? btrue : bfalse;
				}
				if(!pr)
				{
					res = 1;
					/*
					if(b->swipe.base.pressed)
					{
						if(f)
							f(b->swipe.action, bfalse, 0, 0);
					}
					*/
					b->swipe.base.pressed = pr;
					if(b->swipe.mask[0] > 0)
					{
						if(f)
							f(b->swipe.actions[0], bfalse, 0, 0);
						b->swipe.mask[0] = 0;
					}
					if(b->swipe.mask[1] > 0)
					{
						if(f)
							f(b->swipe.actions[1], bfalse, 0, 0);
						b->swipe.mask[1] = 0;
					}
					if(b->swipe.mask[2] > 0)
					{
						if(f)
							f(b->swipe.actions[2], bfalse, 0, 0);
						b->swipe.mask[2] = 0;
					}
					if(b->swipe.mask[3] > 0)
					{
						if(f)
							f(b->swipe.actions[3], bfalse, 0, 0);
						b->swipe.mask[3] = 0;
					}
					b->swipe.distance = 0.0f;
					b->swipe.angle = -1.0f;
				}
				else
				{
					/*
					if(!b->swipe.base.pressed)
					{
						if(f)
							f(b->swipe.action, btrue, 0, 0);
					}
					*/
					b->swipe.base.pressed = pr;
					//res = 1;
				}
			}
			else if(b->type == vkb_cursor_type)
			{
				boolean pr = bfalse;
				if(res && b->base.z < last_z)
				{
					pr = bfalse;
				}
				else
				{
					pr = p ? btrue : bfalse;
				}
				float cx = b->cursor.base.x + b->cursor.base.width / 2;
				float cy = b->cursor.base.y + b->cursor.base.height / 2;
				circle_direction d = karinGetJoystickDirection(x, y, cx, cy, 0, b->cursor.e_radius, NULL, NULL);
				if(!pr)
				{
					if(b->cursor.mask[0] > 0)
					{
						if(f)
							f(b->cursor.actions[0], bfalse, 0, 0);
						b->cursor.mask[0] = 0;
					}
					if(b->cursor.mask[1] > 0)
					{
						if(f)
							f(b->cursor.actions[1], bfalse, 0, 0);
						b->cursor.mask[1] = 0;
					}
					if(b->cursor.mask[2] > 0)
					{
						if(f)
							f(b->cursor.actions[2], bfalse, 0, 0);
						b->cursor.mask[2] = 0;
					}
					if(b->cursor.mask[3] > 0)
					{
						if(f)
							f(b->cursor.actions[3], bfalse, 0, 0);
						b->cursor.mask[3] = 0;
					}
					b->cursor.distance = 0.0f;
					b->cursor.angle = -1.0f;
					b->cursor.pos_x = 0.0f;
					b->cursor.pos_y = 0.0f;
					b->cursor.base.pressed = bfalse;
					if(f)
						f(b->cursor.action, bfalse, 0, 0);
				}
				else
				{
					if(d != circle_outside)
					{
						b->cursor.base.pressed = btrue;
						last_z = b->base.z;
						res = 1;
						if(f)
							f(b->cursor.action, btrue, 0, 0);
						b->cursor.distance = 0.0f;
						b->cursor.angle = -1.0f;
						b->cursor.pos_x = x - cx;
						b->cursor.pos_y = y - cy;
					}
				}
			}
		}
	}
	return res;
}

