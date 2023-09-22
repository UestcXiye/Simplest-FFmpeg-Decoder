// Simplest FFmpeg Decoder.cpp : �������̨Ӧ�ó������ڵ㡣
//

/**
* ��򵥵Ļ���FFmpeg�Ľ�����
* Simplest FFmpeg Decoder
*
* ԭ����
* ������ Lei Xiaohua
* leixiaohua1020@126.com
* �й���ý��ѧ/���ֵ��Ӽ���
* Communication University of China / Digital TV Technology
* http://blog.csdn.net/leixiaohua1020
*
* �޸ģ�
* ���ĳ� Liu Wenchen
* 812288728@qq.com
* ���ӿƼ���ѧ/������Ϣ
* University of Electronic Science and Technology of China / Electronic and Information Science
* https://blog.csdn.net/ProgramNovice
*
* ������ʵ������Ƶ�ļ��Ľ���(֧��HEVC��H.264��MPEG2��)��
* ����򵥵�FFmpeg��Ƶ���뷽��Ľ̡̳�
* ͨ��ѧϰ�����ӿ����˽�FFmpeg�Ľ������̡�
*
* This software is a simplest video decoder based on FFmpeg.
* Suitable for beginner of FFmpeg.
*
*/

#include "stdafx.h"
#pragma warning(disable:4996)

#include <stdio.h>

#define __STDC_CONSTANT_MACROS

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
};


int main(int argc, char* argv[])
{
	AVFormatContext	*pFormatCtx;
	int				i, videoindex;
	AVCodecContext	*pCodecCtx;
	AVCodec			*pCodec;
	AVFrame	*pFrame, *pFrameYUV;
	uint8_t *out_buffer;
	AVPacket *packet;
	int y_size;
	int ret, got_picture;
	struct SwsContext *img_convert_ctx;
	//�����ļ�·��
	char filepath[] = "Titanic.ts";

	int frame_cnt;

	av_register_all();
	avformat_network_init();
	// ����avFormatContext�ռ䣬�ǵ�Ҫ�ͷ�
	pFormatCtx = avformat_alloc_context();
	// ��ý���ļ�
	if (avformat_open_input(&pFormatCtx, filepath, NULL, NULL) != 0)
	{
		printf("Couldn't open input stream.\n");
		return -1;
	}
	// ��ȡý���ļ���Ϣ����pFormatCtx��ֵ
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}
	videoindex = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++)
	{
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoindex = i;
			break;
		}
	}
	if (videoindex == -1)
	{
		printf("Didn't find a video stream.\n");
		return -1;
	}

	pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	// ������Ƶ����Ϣ��codec_id�ҵ���Ӧ�Ľ�����
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL)
	{
		printf("Codec not found.\n");
		return -1;
	}
	// ʹ�ø�����pCodec��ʼ��pCodecCtx
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		printf("Could not open codec.\n");
		return -1;
	}

	/*
	* �ڴ˴���������Ƶ��Ϣ�Ĵ���
	* ȡ����pFormatCtx��ʹ��fprintf()
	*/
	FILE *fp_txt = fopen("output.txt", "wb+");

	fprintf(fp_txt, "��װ��ʽ��%s\n", pFormatCtx->iformat->long_name);
	fprintf(fp_txt, "�����ʣ�%d\n", pFormatCtx->bit_rate);
	fprintf(fp_txt, "��Ƶʱ����%d\n", pFormatCtx->duration);
	fprintf(fp_txt, "��Ƶ���뷽ʽ��%s\n", pFormatCtx->streams[videoindex]->codec->codec->long_name);
	fprintf(fp_txt, "��Ƶ�ֱ��ʣ�%d * %d\n", pFormatCtx->streams[videoindex]->codec->width, pFormatCtx->streams[videoindex]->codec->height);

	// ��avcodec_receive_frame()������Ϊ��������ȡ��frame
	// ��ȡ����frame��Щ�����Ǵ����Ҫ���˵���������Ӧ֡���ܳ�������
	pFrame = av_frame_alloc();
	//��Ϊyuv�����frame�����ߣ���������ź͹��˳����֡��YUV��Ӧ������Ҳ�ǴӸö����ж�ȡ
	pFrameYUV = av_frame_alloc();
	// ������Ⱦ�����ݣ��Ҹ�ʽΪYUV420P
	out_buffer = (uint8_t *)av_malloc(avpicture_get_size(PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
	avpicture_fill((AVPicture *)pFrameYUV, out_buffer, PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);
	packet = (AVPacket *)av_malloc(sizeof(AVPacket));
	// Output Info
	printf("--------------- File Information ----------------\n");
	av_dump_format(pFormatCtx, 0, filepath, 0);
	printf("-------------------------------------------------\n");
	// ���ڽ��������֡��ʽ��һ����YUV420P��,����Ⱦ֮ǰ��Ҫ���и�ʽת��
	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
		pCodecCtx->width, pCodecCtx->height, PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

	FILE *fp_h264 = fopen("output.h264", "wb+");
	FILE *fp_yuv = fopen("output.yuv", "wb+");

	// ֡������
	frame_cnt = 0;
	// ��ʼһ֡һ֡��ȡ
	while (av_read_frame(pFormatCtx, packet) >= 0)
	{
		if (packet->stream_index == videoindex)
		{
			/*
			* �ڴ˴�������H264�����Ĵ���
			* ȡ����packet��ʹ��fwrite()
			*/
			fwrite(packet->data, 1, packet->size, fp_h264);

			// ���ÿһ������ǰ��Ƶ֡������֡��С
			fprintf(fp_txt, "֡%d��С��%d\n", frame_cnt, packet->size);

			ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
			if (ret < 0)
			{
				printf("Decode Error.\n");
				return -1;
			}
			if (got_picture)
			{
				// ��ʽת�������������ݾ���sws_scale()��������ȥ����Ч����
				sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
					pFrameYUV->data, pFrameYUV->linesize);
				printf("Decoded frame index: %d\n", frame_cnt);

				/*
				* �ڴ˴�������YUV�Ĵ���
				* ȡ����pFrameYUV��ʹ��fwrite()
				*/
				fwrite(pFrameYUV->data[0], 1, pCodecCtx->width*pCodecCtx->height, fp_yuv);// Y
				fwrite(pFrameYUV->data[1], 1, pCodecCtx->width*pCodecCtx->height / 4, fp_yuv);// U
				fwrite(pFrameYUV->data[2], 1, pCodecCtx->width*pCodecCtx->height / 4, fp_yuv);// V

				// ���ÿһ���������Ƶ֡������֡����
				char pict_type_str[10];
				switch (pFrame->pict_type)
				{
				case AV_PICTURE_TYPE_NONE:
					sprintf(pict_type_str, "NONE");
					break;
				case AV_PICTURE_TYPE_I:
					sprintf(pict_type_str, "I");
					break;
				case AV_PICTURE_TYPE_P:
					sprintf(pict_type_str, "P");
					break;
				case AV_PICTURE_TYPE_B:
					sprintf(pict_type_str, "B");
					break;
				case AV_PICTURE_TYPE_SI:
					sprintf(pict_type_str, "SI");
					break;
				case AV_PICTURE_TYPE_S:
					sprintf(pict_type_str, "S");
					break;
				case AV_PICTURE_TYPE_SP:
					sprintf(pict_type_str, "SP");
					break;
				case AV_PICTURE_TYPE_BI:
					sprintf(pict_type_str, "BI");
					break;
				default:
					break;
				}
				fprintf(fp_txt, "֡%d���ͣ�%s\n", frame_cnt, pict_type_str);

				frame_cnt++;

			}
		}
		av_free_packet(packet);
	}

	// �ر��ļ�
	fclose(fp_txt);
	fclose(fp_h264);
	fclose(fp_yuv);

	// �ͷ������Դ
	sws_freeContext(img_convert_ctx);

	av_frame_free(&pFrameYUV);
	av_frame_free(&pFrame);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

	return 0;
}
