#include "stdafx.h"
#include <fstream>


//将 AVFrame 保存成图片文件  ppm格式
void SaveFrame(AVFrame* pFrame, int width, int height, int iFrame)
{
	//（1）文件名
	char szFileName[32];
	sprintf(szFileName, "frame%04d.ppm", iFrame);


	//（2）打开文件
	FILE *pFile;
	pFile = fopen(szFileName, "wb");
	if (pFile == NULL)
	{
		return;
	}

	//（3）写入ppm文件的文件头 
	fprintf(pFile, "P6\n%d %d\n255\n", width, height);

	//（4）写入ppm文件的内容
	for (int i = 0; i < height; i++)
	{
		fwrite(pFrame->data[0] + i * pFrame->linesize[0], 1, width * 3, pFile);
	}
}


int _tmain(int argc, _TCHAR* argv[])
{
	//文件名字 
	char filepath[] = "nwn.mp4";

	//(1)这里注册了所有的文件格式和编解码器的库 所以他们将被自动的使用在被打开的合适格式的文件  只需要注册一次
	av_register_all();
	avformat_network_init();
	AVFormatContext *pFormatCtx;
	pFormatCtx = avformat_alloc_context();

	//(2)打开一个文件 打开之后pFormatCtx就有有了文件句柄  这个会打开文件且读取Header
	if (avformat_open_input(&pFormatCtx, filepath, NULL, NULL) != 0) 
	{
		return -1;
	}

	//(3)检查在文件中的流的信息
	if(avformat_find_stream_info(pFormatCtx,0)<0)
	{
		return -1;
	}

	//(4)dump下信息
	av_dump_format(pFormatCtx,0, filepath,0);


	AVCodecContext *pCodecCtx;
	int videoStream = -1;
	//pFormatCtx->Streams 仅仅是一组pFormatCtx->nb_streams 的指针 包含了哪些流
	for(int i=0; i<pFormatCtx->nb_streams;i++)
	{
		if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoStream = i;
			break;
		}
	}

	if(videoStream == -1)
	{
		return -1; 
	}

	pCodecCtx = pFormatCtx->streams[videoStream]->codec;

	AVCodec *pCodec;


	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);

	if(pCodec == NULL)
	{
		fprintf(stderr, "Unsupported codec!\n");
		return -1;
	}

	if(avcodec_open(pCodecCtx,pCodec) < 0)
	{
		return -1;
	}

	AVFrame *pFrame;
	pFrame = avcodec_alloc_frame();

	AVFrame *pFrameRGB = avcodec_alloc_frame();
	if(pFrameRGB == NULL)
	{
		return -1;
	}
	uint8_t *buffer;
	int numBytes;
	numBytes = avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);

	buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));

	//数据是从buffer来的  这里相当于是pFrameRGB初始化数据 可能是个黑色的图片
	avpicture_fill((AVPicture*)pFrameRGB, buffer, PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);


	int frameFinished;
	AVPacket packet;
	int i = 0;
	//（5）循环从Steams中 读取出frame packet通常包含一个压缩的Frame 音频则可能是多个Frame
	while(av_read_frame(pFormatCtx,&packet)>=0)
	{
		//stream_index Packet所在stream的index 通过这个来判断是不是视频帧
		if(packet.stream_index == videoStream)
		{
			//把视频帧解压到Frame中
			avcodec_decode_video2(pCodecCtx,pFrame,&frameFinished,&packet);
			if(frameFinished)
			{
				//旧版本
				//img_convert((AVPicture *)pFrameRGB, PIX_FMT_RGB24,(AVPicture*)pFrame, pCodecCtx->pix_fmt,pCodecCtx->width, pCodecCtx->height);
				//转换参数 
				SwsContext *img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);
				//转换
				sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
				++i;
				if( i%120 == 1)
				{
					SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height, i);
				}
			}
			av_free_packet(&packet);
		}
	}
	av_free(buffer);
	av_free(pFrame);
	av_free(pFrameRGB);

	avcodec_close(pCodecCtx);

	avformat_close_input(&pFormatCtx);


	return 0;
}

