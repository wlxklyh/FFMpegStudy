#include "stdafx.h"
#include <fstream>


void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
	FILE *pFile;		//文件指针
	char szFilename[32];//文件名（字符串）
	int y;				//

	sprintf(szFilename, "frame%04d.ppm", iFrame);	//生成文件名
	pFile = fopen(szFilename, "wb");			//打开文件，只写入
	if (pFile == NULL) {
		return;
	}

	//getch();

	fprintf(pFile, "P6\n%d %d\n255\n", width, height);//在文档中加入，必须加入，不然PPM文件无法读取

	for (y = 0; y < height; y++) {
		fwrite(pFrame->data[0] + y * pFrame->linesize[0], 1, width * 3, pFile);
	}

	fclose(pFile);

}


int _tmain(int argc, _TCHAR* argv[])
{

	char filepath[] = "nwn.mp4";

	//(1)这里注册了所有的文件格式和编解码器的库 所以他们将被自动的使用在被打开的合适格式的文件  只需要注册一次
	av_register_all();
	avformat_network_init();
	AVFormatContext *pFormatCtx;
	pFormatCtx = avformat_alloc_context();

	//(2)打开一个文件
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


	int i;
	AVCodecContext *pCodecCtx;

	int videoStream = -1;
	//pFormatCtx->Streams 仅仅是一组pFormatCtx->nb_streams 的指针
	for(i=0; i<pFormatCtx->nb_streams;i++)
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

	//数据是上下文来的吗
	avpicture_fill((AVPicture*)pFrameRGB, buffer, PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);


	int frameFinished;
	AVPacket packet;
	i = 0;
	while(av_read_frame(pFormatCtx,&packet)>=0)
	{
		if(packet.stream_index == videoStream)
		{
			avcodec_decode_video2(pCodecCtx,pFrame,&frameFinished,&packet);
			if(frameFinished)
			{
				//旧版本
				//img_convert((AVPicture *)pFrameRGB, PIX_FMT_RGB24,(AVPicture*)pFrame, pCodecCtx->pix_fmt,pCodecCtx->width, pCodecCtx->height);

				SwsContext *img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);
				sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);

				if(++i <=5)
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

