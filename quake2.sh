#!/bin/sh

VER=debug
#VER=release
RUN=""
Q2=""
BUILD=""

case ${1} in
	"clean")
		make clean
		rm -rf ./${VER}arm/*
		echo "clean done."
		exit 0;
		;;
	"build")
		BUILD="build"
		;;
	"run")
		Q2="quake2"
		;;
	"debug")
		Q2="sdlquake2"
		RUN="debug"
		;;
	"build_run")
		BUILD="build"
		Q2="quake2"
		;;
	"build_debug")
		BUILD="build"
		Q2="quake2"
		RUN="debug"
		;;
	*)
		Q2="sdlquake2"
		;;
esac

if [ "x" != x${2} ]; then
	VER=${2}
fi

if [ "xbuild" == x${BUILD} ]; then
	make build_${VER}
fi

if [ "x0" != x$? ]; then
	echo "Build error!"
	exit -1;
fi

if [ "x" == x${Q2} ]; then
	echo "Build finished."
	exit 0;
fi

echo "Run......"

cp -r ${PWD}/resc ${PWD}/${VER}arm

cd ${PWD}/${VER}arm

rm -f ~user/.quake2/ref_*.so
cp ./ref_*so ~user/.quake2/

rm -f ~user/.quake2/baseq2/gamesarm.so
cp ./gamearm.so ~user/.quake2/baseq2/gamesarm.so

if [ x${RUN} == "xdebug" ]; then
	gdb --args ./${Q2} +set basedir /home/user/.quake2 +set vid_ref glx +set gl_driver libGLES_CM.so +set vid_fullscreen 1
else
	./${Q2} +set basedir /home/user/.quake2 +set vid_ref glx +set gl_driver libGLES_CM.so +set vid_fullscreen 1
	#./${Q2} +set basedir /home/user/.quake2 +set vid_ref softsdl +set vid_fullscreen 1 +set cl_drawfps 1
fi
#./sdlquake2 +set basedir /home/user/.quake2 +set vid_ref glx +set gl_driver libGLES_CM.so +set vid_fullscreen 1
#./sdlquake2 +set basedir /home/user/.quake2 +set vid_ref softx +set vid_fullscreen 1
#./sdlquake2 +set basedir /home/user/.quake2 +set vid_ref softsdl +set vid_fullscreen 1
