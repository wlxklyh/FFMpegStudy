// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#include "targetver.h"
#include <stdio.h>
#include <tchar.h>
#include <io.h>
#include <direct.h>
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
	//新版里的图像转换结构需要引入的头文件
#include "libswscale/swscale.h"
	//SDL
#include "sdl/SDL.h"
#include "sdl/SDL_thread.h"

};
//获取传入参数

// TODO: 在此处引用程序需要的其他头文件
