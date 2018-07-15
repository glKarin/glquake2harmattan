#!/bin/sh

EXEC="/usr/bin/sdlquake2 +set basedir /home/user/.quake2 +set vid_ref glx +set gl_driver libGLES_CM.so +set vid_fullscreen 1 +set gl_ext_multitexture 0 +set gl_ext_pointparameters 1 +set gl_shadows 1 +set gl_stencilshadow 1"
#+set cl_drawfps 1 

echo "Run Quake 2 (Renderer: OpenGL)......"
echo "${EXEC} $*"
${EXEC} $*

