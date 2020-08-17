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
https://github.com/wlxklyh/FFMpegStudy/blob/master/win/Tutorial2
![播放视频](Img/playvedio.gif)
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

## 三、Tutorial3——播放音频
现在我们要来播放音频。SDL 也为我们准备了输出音频的方法。函数SDL_OpenAudio() 本身就是用来打开音
频设备的。它使用一个叫做SDL_AudioSpec 结构体作为参数，这个结构体中包含了我们将要输出的音频的所有信
息。
在我们展示如何建立之前，让我们先解释一下电脑是如何处理音频的。数字音频是由一长串的样本
（samples）流组成的。每个样本表示音频波形中的一个值。音频按照一个特定的采样率sample rate 来进行录制，
采样率表示以多快的速度来播放这段样本流，它的表示方式为每秒多少次采样。例如22050 和44100 的采样率就
是电台和CD 常用的采样率。此外，大多音频有不只一个通道来表示立体声或者环绕。例如，如果采样是立体声，
那么每次的采样数就为2 个。当我们从一个电影文件中得到数据的时候，我们不知道我们将得到多少个样本，但
是ffmpeg 将不会给我们部分的样本——这意味着它将不会把立体声分割开来。
SDL 播放音频的方式是这样的：你先设置音频的选项：采样率（在SDL 的结构体中叫做“freq”，代指
frequency），通道数和其它的参数，然后我们设置一个回调函数和一些用户数据（userdata）。当开始播放音频的
时候，SDL 将不断地调用这个回调函数并且要求它来向音频缓冲填入特定的数量的字节。当我们把这些信息放到


## 附
后面学习FFMpeg（win）都从[这里](https://github.com/wlxklyh/FFMpegStudy/blob/master/win/BackUp/HelloWorld)拷贝出来  不用管环境和头文件的问题。


