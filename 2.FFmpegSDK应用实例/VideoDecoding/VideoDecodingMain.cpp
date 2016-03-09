#include <stdio.h>

#include "InputOutput.h"
#include "Decoder.h"

void hello()
{
	printf("*********************************\n");
	printf("VideoDecoding: A FFmpeg SDK demo.\nDeveloped by Yin Wenjie\n");
	printf("*********************************\n");
	printf("=================================\nCompulsory Paramaters:\n");
	printf("Input YUV file name");
	printf("\tOutput stream file name\n");
	printf("*********************************\n");
}

void write_out_yuv_frame(const Codec_Ctx &ctx, IO_Param &in_out)
{
	uint8_t **pBuf	= ctx.frame->data;
	int*	pStride = ctx.frame->linesize;
	
	for (int color_idx = 0; color_idx < 3; color_idx++)
	{
		int		nWidth	= color_idx == 0 ? ctx.frame->width : ctx.frame->width / 2;
		int		nHeight = color_idx == 0 ? ctx.frame->height : ctx.frame->height / 2;
		for(int idx=0;idx < nHeight; idx++)
		{
			fwrite(pBuf[color_idx],1, nWidth, in_out.pFout);
			pBuf[color_idx] += pStride[color_idx];
		}
		fflush(in_out.pFout);
	}
}

int main(int argc, char **argv)
{
	uint8_t *pDataPtr = NULL;
	int uDataSize = 0;
	int got_picture, len;

	Codec_Ctx ctx;
	IO_Param inputoutput;

	hello();
	
	Parse(argc, argv, inputoutput);

	OpenFiles(inputoutput);
	
	int frame_count;
	uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
	
	/* set end of buffer to 0 (this ensures that no overreading happens for damaged mpeg streams) */
	memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);
	
	printf("Decode video file %s to %s\n", argv[1], argv[2]);

	OpenDeocder(ctx);

    

	frame_count = 0;
	for (;;) 
	{
//		uDataSize = fread(inbuf, 1, INBUF_SIZE, inputoutput.pFin);
		uDataSize = fread_s(inbuf,INBUF_SIZE, 1, INBUF_SIZE, inputoutput.pFin);
		if (0 == uDataSize)
		{
			break;
		}

		pDataPtr = inbuf;

		while(uDataSize > 0)
		{
			len = av_parser_parse2(ctx.pCodecParserCtx, ctx.pCodecContext, 
										&(ctx.pkt.data), &(ctx.pkt.size), 
										pDataPtr, uDataSize, 
										AV_NOPTS_VALUE, AV_NOPTS_VALUE, AV_NOPTS_VALUE);
			pDataPtr += len;
			uDataSize -= len;

			if (0 == ctx.pkt.size)
			{
				continue;
			}

			//Some Info from AVCodecParserContext
			printf("[Packet]Size:%6d\t",ctx.pkt.size);
			switch(ctx.pCodecParserCtx->pict_type)
			{
			case AV_PICTURE_TYPE_I: 
				printf("Type:I\t");
				break;
			case AV_PICTURE_TYPE_P: 
				printf("Type:P\t");
				break;
			case AV_PICTURE_TYPE_B: 
				printf("Type:B\t");
				break;
			default: 
				printf("Type:Other\t");
				break;
			}
			printf("Number:%4d\n",ctx.pCodecParserCtx->output_picture_number);

			int ret = avcodec_decode_video2(ctx.pCodecContext, ctx.frame, &got_picture, &(ctx.pkt));
			if (ret < 0) 
			{
				printf("Decode Error.\n");
				return ret;
			}
			if (got_picture) 
			{
				write_out_yuv_frame(ctx, inputoutput);
				printf("Succeed to decode 1 frame!\n");
			}
		}
	}

    ctx.pkt.data = NULL;
    ctx.pkt.size = 0;
	while(1)
	{
		int ret = avcodec_decode_video2(ctx.pCodecContext, ctx.frame, &got_picture, &(ctx.pkt));
		if (ret < 0) 
		{
			printf("Decode Error.\n");
			return ret;
		}
		if (!got_picture)
			break;
		if (got_picture) 
		{
			write_out_yuv_frame(ctx, inputoutput);
			printf("Flush Decoder: Succeed to decode 1 frame!\n");
		}
	}

	CloseFiles(inputoutput);
	CloseDecoder(ctx);

	return 1;
}