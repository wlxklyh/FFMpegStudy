#include "stdafx.h"
#include <fstream>

#define SDL_AUDIO_BUFFER_SIZE 1024
typedef struct PacketQueue {
	AVPacketList *first_pkt, *last_pkt;
	int nb_packets;
	int size;
	SDL_mutex *mutex;
	SDL_cond *cond;
} PacketQueue;

PacketQueue audioq;


int quit = 0;
//队列初始化
void packet_queue_init(PacketQueue*q)
{
	memset(q, 0, sizeof(PacketQueue));
	q->mutex = SDL_CreateMutex();
	q->cond = SDL_CreateCond();
}
//出队列
static int packet_queue_get(PacketQueue *q, AVPacket *packatToPop, int block)
{
	AVPacketList *pktToPop;
	int ret;
	//获取要加锁
	SDL_LockMutex(q->mutex);

	while (true) {
		if (quit) {
			ret = -1;
			break;
		}

		pktToPop = q->first_pkt;
		if (pktToPop) {
			q->first_pkt = pktToPop->next;
			if (!q->first_pkt)
				q->last_pkt = NULL;
			q->nb_packets--;
			q->size -= pktToPop->pkt.size;
			*packatToPop = pktToPop->pkt;
			av_free(pktToPop);
			ret = 1;
			break;
		}
		else if (!block) {
			ret = 0;
			break;
		}
		else {
			SDL_CondWait(q->cond, q->mutex);
		}
	}
	SDL_UnlockMutex(q->mutex);
	return ret;
}
//入队列
int packet_queue_put(PacketQueue* q, AVPacket*pkt)
{
	AVPacketList *pkt1;
	if (av_dup_packet(pkt) < 0)//将shared buffer 的AVPacket duplicate(复制)到独立的buffer中
	{
		return -1;
	}
	pkt1 = (AVPacketList *)av_malloc(sizeof(AVPacketList));
	if (pkt1 == NULL)
	{
		return -1;
	}
	pkt1->pkt = *pkt;
	pkt1->next = NULL;

	SDL_LockMutex(q->mutex);

	if (!q->last_pkt)
	{
		q->first_pkt = pkt1;
	}
	else
	{
		q->last_pkt->next = pkt1;
	}
	q->last_pkt = pkt1;
	q->nb_packets++;
	q->size += pkt1->pkt.size;
	SDL_CondSignal(q->cond);

	SDL_UnlockMutex(q->mutex);
}

//从packet队列解码出数据塞到audio_buf中
int audio_decode_frame(AVCodecContext *aCodecCtx, uint8_t *audio_buf, int buf_size) {
	static AVFrame *decoded_aframe;
	static AVPacket pkt, pktTemp;


	int len1, data_size;

	while(true) 
	{
		while (pktTemp.size > 0)
		{
			int got_frame = 0;

			if (!decoded_aframe) {
				if (!(decoded_aframe = avcodec_alloc_frame())) {
					fprintf(stderr, "out of memory\n");
					exit(1);
				}
			}
			else
				avcodec_get_frame_defaults(decoded_aframe);

			//data_size = buf_size; /// ????
			len1 = avcodec_decode_audio4(aCodecCtx, decoded_aframe, &got_frame, &pktTemp);


			/// Check if 
			if (len1 < 0) {
				pktTemp.size = 0;
				break; // skip packet
			}


			if (got_frame) {
				printf("\nGot frame!");
				//printf("\nFrame data size: %d", sizeof(decoded_aframe->data[0]));
				data_size = av_samples_get_buffer_size(NULL, aCodecCtx->channels,
					decoded_aframe->nb_samples,
					aCodecCtx->sample_fmt, 1);
				if (data_size > buf_size) {
					data_size = buf_size;
				}
				memcpy(audio_buf, decoded_aframe->data[0], data_size);

			}
			else {
				data_size = 0;
			}

			printf("\nData size %d", data_size);
			pktTemp.data += len1;
			pktTemp.size -= len1;

			if (data_size <= 0) {
				continue;
			}

			return data_size;
			/* Deprecated
						data_size = buf_size;
						len1 = avcodec_decode_audio2(aCodecCtx, (int16_t *)audio_buf, &data_size,
													 audio_pkt_data, audio_pkt_size);
						if(len1 < 0) {
							// if error, skip frame
							audio_pkt_size = 0;
							break;
						}
						audio_pkt_data += len1;
						audio_pkt_size -= len1;
						if(data_size <= 0) {
							// No data yet, get more frames
							continue;
						}
						// We have data, return it and come back for more later
						return data_size;
			*/
		}


		if (pkt.data)
			av_free_packet(&pkt);

		if (quit)
		{
			return -1;
		}

		//从队列里面获取 这里是在音频线程 如果没有音频数据则会wait等待生产者的信号量
		if (packet_queue_get(&audioq, &pkt, 1) < 0)
		{
			return -1;
		}


		av_init_packet(&pktTemp);

		pktTemp.data = pkt.data;
		pktTemp.size = pkt.size;
	}
}

//音频线程回调
void audio_callback(void *userdata, Uint8 *stream, int len) {

	//回调传入的解码器上下文
	AVCodecContext *aCodecCtx = (AVCodecContext *)userdata;
	int len1, audio_size;

	static uint8_t audio_buf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];
	static unsigned int audio_buf_size = 0;
	static unsigned int audio_buf_index = 0;

	while (len > 0) {
		/// audio
		if (audio_buf_index >= audio_buf_size) {
			//从队列里面读取出解码后的音频数据
			audio_size = audio_decode_frame(aCodecCtx, audio_buf, sizeof(audio_buf));
			if (audio_size < 0) {
				/* If error, output silence */
				audio_buf_size = 1024; // arbitrary?
				memset(audio_buf, 0, audio_buf_size);
			}
			else {
				audio_buf_size = audio_size;
			}
			audio_buf_index = 0;
		}

		len1 = audio_buf_size - audio_buf_index;
		if (len1 > len)
			len1 = len;
		memcpy(stream, (uint8_t *)audio_buf + audio_buf_index, len1);
		len -= len1;
		stream += len1;
		audio_buf_index += len1;
	}
}

int _tmain(int args,_TCHAR* argv[])
{
	//视频文件名字
	char videoFilePath[] = "nwn.mp4";

	//（1）这里注册了所有的文件格式和编解码器的库 所以他们将被自动的使用在被打开的合适格式的文件  需要注册一次
	av_register_all();
	avformat_network_init();
	AVFormatContext *pFormatCtx = avformat_alloc_context();//这句应该是最新的FFMpeg的要求

	//（2）打开文件
	if(avformat_open_input(&pFormatCtx,videoFilePath,NULL,NULL)!=0)
	{
		//打开失败
		return -1;
	}
	//打印视频文件信息
	if(av_find_stream_info(pFormatCtx) < 0)
	{
		return -1;
	}
	av_dump_format(pFormatCtx, 0, videoFilePath, 0);

	//（3）查找视频流、音频流
	int vedioIndex = -1;
	int audioIndex = -1;
	for(int i=0;i<pFormatCtx->nb_streams;i++)
	{
		if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			vedioIndex = i;
			continue;
		}
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			audioIndex = i;
			continue;
		}
	}
	if(vedioIndex == -1 || audioIndex == -1)
	{
		//找不到视频流
		return -1;
	}

	//（4）得到视频流的解码器
	AVCodec *avVideoCodec, *avAudioCodec;
	AVCodecContext *avVideoCodecCtx,*avAudioCodecCtx;
	
	avVideoCodecCtx = pFormatCtx->streams[vedioIndex]->codec;
	avAudioCodecCtx = pFormatCtx->streams[audioIndex]->codec;

	avVideoCodec = avcodec_find_decoder(avVideoCodecCtx->codec_id);
	avAudioCodec = avcodec_find_decoder(avAudioCodecCtx->codec_id);
	if(avVideoCodec == NULL|| avAudioCodec==NULL)
	{
		return -1;
	}
	//打开视频解码器 打开音频解码器
	if(avcodec_open2(avVideoCodecCtx,avVideoCodec,NULL) < 0)
	{
		return -1;
	}
	if (avcodec_open2(avAudioCodecCtx, avAudioCodec, NULL) < 0)
	{
		return -1;
	}

	packet_queue_init(&audioq);
	
	//初始化音频流的SDL  
	SDL_AudioSpec   wanted_spec, spec;
	wanted_spec.freq = avAudioCodecCtx->sample_rate;				//采样率：
	wanted_spec.format = AUDIO_S16SYS;								//格式：S16SYS signed 每个样本16位 SYS代表大小端跟系统一样
	wanted_spec.channels = avAudioCodecCtx->channels;				//通道数
	wanted_spec.silence = 0;										//静音的值 0代表静音 
	wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;					//音频缓冲区的尺寸
	wanted_spec.callback = audio_callback;							//回调函数
	wanted_spec.userdata = avAudioCodecCtx;							//
	if (SDL_OpenAudio(&wanted_spec, &spec) < 0)
	{
		return -1;
	}


	//这个函数一定要在SDL_OpenAudio之后调用 可以让你安全的初始化回调函数
	SDL_PauseAudio(0);



	//（5）帧初始化
	AVFrame *nowAVFrame = avcodec_alloc_frame();
	AVFrame *nowYUVAVFrame = avcodec_alloc_frame();
	if(nowAVFrame == NULL || nowYUVAVFrame == NULL)
	{
		return -1;
	}
	int numBytes = avpicture_get_size(PIX_FMT_YUV420P, avVideoCodecCtx->width, avVideoCodecCtx->height);
	uint8_t *buffer = (uint8_t*)av_malloc(numBytes*sizeof(uint8_t));
	avpicture_fill((AVPicture*)nowYUVAVFrame, buffer, PIX_FMT_YUV420P, avVideoCodecCtx->width, avVideoCodecCtx->height);


	//（6）SDL
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
	{
		return -1;
	}
	SDL_Rect rect;
	SDL_Surface *screen = SDL_SetVideoMode(avVideoCodecCtx->width, avVideoCodecCtx->height, 0, 0);
	SDL_Overlay *bitmap = SDL_CreateYUVOverlay(avVideoCodecCtx->width, avVideoCodecCtx->height, SDL_YV12_OVERLAY, screen);

	//初始化一个Packet
	int frameFinished;
	AVPacket *packet = (AVPacket*)av_malloc(sizeof(AVPacket));
	int YSize = avVideoCodecCtx->width*avVideoCodecCtx->height;
	av_new_packet(packet, YSize);

	//（7）循环的读帧
	while(av_read_frame(pFormatCtx,packet) >= 0)
	{
		if(packet->stream_index ==vedioIndex)
		{
			avcodec_decode_video2(avVideoCodecCtx, nowAVFrame, &frameFinished, packet);
			if(frameFinished)
			{
				SwsContext *imageConvertCtx = sws_getContext(avVideoCodecCtx->width, avVideoCodecCtx->height, avVideoCodecCtx->pix_fmt, avVideoCodecCtx->width, avVideoCodecCtx->height,
					PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
				sws_scale(imageConvertCtx, (const uint8_t* const*)nowAVFrame->data, nowAVFrame->linesize, 0, avVideoCodecCtx->height, nowYUVAVFrame->data, nowYUVAVFrame->linesize);

				SDL_LockYUVOverlay(bitmap);
				bitmap->pixels[0] = nowYUVAVFrame->data[0];
				bitmap->pixels[2] = nowYUVAVFrame->data[1];
				bitmap->pixels[1] = nowYUVAVFrame->data[2];
				bitmap->pitches[0] = nowYUVAVFrame->linesize[0];
				bitmap->pitches[2] = nowYUVAVFrame->linesize[1];
				bitmap->pitches[1] = nowYUVAVFrame->linesize[2];
				SDL_UnlockYUVOverlay(bitmap);

				rect.x = 0;
				rect.y = 0;
				rect.w = avVideoCodecCtx->width;
				rect.h = avVideoCodecCtx->height;

				SDL_DisplayYUVOverlay(bitmap, &rect);
				//这里可以设置一下 让视频音频看起来同步 
				SDL_Delay(0);
			}
			
		}
		else if(packet->stream_index == audioIndex)
		{
			packet_queue_put(&audioq,packet);
		}
		else
		{
			av_free_packet(packet);
		}
	}

	//释放内存
	av_free(buffer);
	av_free(nowAVFrame);
	av_free(nowAVFrame);
	avcodec_close(avVideoCodecCtx);
	avformat_close_input(&pFormatCtx);
}
