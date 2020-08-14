#include "stdafx.h"
#include <fstream>

int _tmain(int argc, _TCHAR* argv[])
{
	char filepath[] = "nwn.mp4";

	//(1)这里注册了所有的文件格式和编解码器的库 所以他们将被自动的使用在被打开的合适格式的文件  只需要注册一次
	av_register_all();
	avformat_network_init();
	AVFormatContext  *pFormatContext = avformat_alloc_context();

	//(2)打开文件
	if(avformat_open_input(&pFormatContext,filepath, NULL, NULL)!=0)
	{
		//打开失败
		return -1;
	}

	//(3)打印信息
	if (av_find_stream_info(pFormatContext) < 0)
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}
	av_dump_format(pFormatContext, 0, filepath, 0);


	//(4)查找视频流
	int vedioIndex = -1;
	for(int i=0;i<pFormatContext->nb_streams;i++)
	{
		if(pFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			vedioIndex = i;
			break;
		}
	}
	if(vedioIndex == -1)
	{
		return -1;
	}

	//(5)得到视频流的解码器
	AVCodec * avcodec;
	AVCodecContext * avcodecContext;
	avcodecContext = pFormatContext->streams[vedioIndex]->codec;
	avcodec = avcodec_find_decoder(avcodecContext->codec_id);
	if (avcodec == NULL)
	{
		fprintf(stderr, "Unsupported codec!\n");
		return -1;
	}
	if (avcodec_open(avcodecContext, avcodec) < 0)
	{
		return -1;
	}

	//(6)帧初始化
	AVFrame *nowAVFrame = avcodec_alloc_frame();
	AVFrame *nowYUVAVFrame = avcodec_alloc_frame();
	if (nowAVFrame == NULL || nowYUVAVFrame == NULL)
	{
		return -1;
	}
	uint8_t *buffer;
	int numBytes;
	numBytes = avpicture_get_size(PIX_FMT_YUV420P, avcodecContext->width, avcodecContext->height);
	buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
	//数据是从buffer来的  这里相当于是pFrameRGB初始化数据 可能是个黑色的图片
	avpicture_fill((AVPicture*)nowYUVAVFrame, buffer, PIX_FMT_YUV420P, avcodecContext->width, avcodecContext->height);

	//-------SDL
	if (SDL_Init((SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)))
	{
		return -1;
	}
	SDL_Rect rect;
	SDL_Surface* screen = SDL_SetVideoMode(avcodecContext->width,avcodecContext->height,0,0);
	SDL_Overlay* bmp = SDL_CreateYUVOverlay(avcodecContext->width, avcodecContext->height, SDL_YV12_OVERLAY, screen);
	//-------SDL

	int frameFinished;
	AVPacket *packet = (AVPacket *)malloc(sizeof(AVPacket));
	int y_size = avcodecContext->width * avcodecContext->height;
	av_new_packet(packet, y_size);
	int i = 0;
	//（6）循环从Steams中 读取出frame packet通常包含一个压缩的Frame 音频则可能是多个Frame
	while (av_read_frame(pFormatContext, packet) >= 0)
	{
		//stream_index Packet所在stream的index 通过这个来判断是不是视频帧
		if (packet->stream_index == vedioIndex)
		{
			//把视频帧解压到Frame中
			avcodec_decode_video2(avcodecContext, nowAVFrame, &frameFinished, packet);
			if (frameFinished)
			{
				//转换参数 
				SwsContext *img_convert_ctx = sws_getContext(avcodecContext->width, avcodecContext->height, avcodecContext->pix_fmt, avcodecContext->width, avcodecContext->height, PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
				//转换
				sws_scale(img_convert_ctx, (const uint8_t* const*)nowAVFrame->data, nowAVFrame->linesize, 0, avcodecContext->height, nowYUVAVFrame->data, nowYUVAVFrame->linesize);

				//-------SDL
				SDL_LockYUVOverlay(bmp);
				bmp->pixels[0] = nowYUVAVFrame->data[0];
				bmp->pixels[2] = nowYUVAVFrame->data[1];
				bmp->pixels[1] = nowYUVAVFrame->data[2];
				bmp->pitches[0] = nowYUVAVFrame->linesize[0];
				bmp->pitches[2] = nowYUVAVFrame->linesize[1];
				bmp->pitches[1] = nowYUVAVFrame->linesize[2];
				SDL_UnlockYUVOverlay(bmp);

				rect.x = 0;
				rect.y = 0;
				rect.w = avcodecContext->width;
				rect.h = avcodecContext->height;
				SDL_DisplayYUVOverlay(bmp, &rect);
				//延时40ms
				SDL_Delay(40);
				//-------SDL
			}
			av_free_packet(packet);
		}
	}

	av_free(buffer);
	av_free(nowAVFrame);
	av_free(nowYUVAVFrame);
	avcodec_close(avcodecContext);
	avformat_close_input(&pFormatContext);


	return 0;
}


