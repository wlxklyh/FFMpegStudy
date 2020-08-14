## FFmpeg入门

## 引用
1、雷神的FFMpeg最小demo
https://blog.csdn.net/leixiaohua1020/article/details/10528443

2、FFmpeg Tutorial.pdf 中文翻译文档
https://github.com/wlxklyh/FFMpegStudy/blob/master/FFmpeg%20Tutorial.pdf

3、视频连接：

[![樱花庄](https://res.cloudinary.com/marcomontalbano/image/upload/v1597284996/video_to_markdown/images/youtube--Zfczinxjy-c-c05b58ac6eb4c4700831b2b3070cd403.jpg)](https://www.youtube.com/watch?v=Zfczinxjy-c&list=PLLvc0-MNtbKVbHv22Sn-YuiwYC5heRSkd "樱花庄")

## 一、Tutorial1——视频截帧保存
https://github.com/wlxklyh/FFMpegStudy/blob/master/win/Tutorial1
这个是9361帧的截图 
![](Img/2020-08-13-10-09-43.png)

流程说明：
1. 初始化
av_register_all()
AVFormatContext pFormatCtx = avformat_alloc_context()
2. 打开文件（会读取header）
avformat_open_input(pFormatCtx,filepath)
3. 检查和获取流信息的API
avformat_find_stream_info()
av_dump_format()
4. 从pFormatCtx中得到那个streamsindex是视频流
pFormatCtx->stream[i]->codec->codec_type == ACMEIDIA_TYPE_VIDEO 
5. 从视频流里面得到CodeContext 编解码上下文
6. 从而得到解码器
7. 读取帧
   av_read_frame 得到avpacket(存储的可能是视频流 也可能是音频流)
8. 判断avpacket是不是视频流
9. 从读取出的avpacket中读取AVFrame出来
10. 用sws_scale转AVFrame的格式 大多可能是YUV 我们要转成RGB保存
11. SaveFrame
    fwrite(frame->data[0]+pFrame->linesize[0],1,width*3,pFile)

## 二、Tutorial2——视频显示到屏幕（SDL）
跟Tutorial1大致一致 除了SDL部分 和 Frame编码为YUV
1. SDL部分
```cpp
SDL_Init//SDL初始化
SDL_Surface* screen = SDL_SetVideoMode//初始化一个屏幕
SDL_Overlay* bmp = SDL_CreateYUVOverlay//有点像Screen上面的一个View
//YUVOverlay  赋值
SDL_LockYUVOverlay 
bmp->pixels[0] = nowYUVAVFrame->data[0];
...
bmp->pitches[0] = nowYUVAVFrame->linesize[0];
...
//显示
SDL_DisplayYUVOverlay
```
2. Frame编码为YUV
注意Frame的初始化 格式是PIX_FMT_YUV420P
avpicture_get_size(PIX_FMT_YUV420P, avcodecContext->width, avcodecContext->height);
sws_scale 转换的时候格式是PIX_FMT_YUV420P

## 附
后面学习FFMpeg（win）都从[这里](https://github.com/wlxklyh/FFMpegStudy/blob/master/win/BackUp/HelloWorld)拷贝出来  不用管环境和头文件的问题。


