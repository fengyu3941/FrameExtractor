#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif
extern "C"
{
#include <stdint.h>
//#include <config.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
};
#include <Encoder.h>

/* __MAX and __MIN: self explanatory */
#ifndef __MAX
#   define __MAX(a, b)   ( ((a) > (b)) ? (a) : (b) )
#endif

//全局变量，jpg压缩质量
extern int g_Quality = 100;


class Encoder
{
public:
	Encoder(int id, unsigned int width, unsigned int height);
	virtual ~Encoder();
public:

	virtual bool Is_open();
	bool EncodeVideo(uint8_t* src[4], int src_stride[4], const char* outfile_name);
private:
	bool b_open;

	//H.264全局变量
	AVCodec *m_ffvcodec;
	AVCodecContext *m_ffvpCodecCtx;

	//转码
	SwsContext *sws_ctx;
	AVFrame* temp;

	//图像尺寸
	unsigned int input_width;
	unsigned int input_height;

};

void* createEncoder( int id, unsigned int width, unsigned int height )
{
	Encoder* p = new Encoder(id, width, height);
	if (p->Is_open())
	{
		return p;
	}
	delete p;
	return NULL;
}

void EncodeVideo( void* p, uint8_t* src[4], int src_stride[4], const char* outfile_name )
{
	if (!p)
	{
		return;
	}
	Encoder* p_Enc = (Encoder*)p;
	p_Enc->EncodeVideo(src, src_stride, outfile_name);
}

void destroyEncoder( void*p )
{
	if (!p)
	{
		return;
	}
	Encoder* p_Enc = (Encoder*)p;
	delete p_Enc;
}


Encoder::Encoder(int id, unsigned int width, unsigned int height):b_open(true), 
		m_ffvcodec(NULL),m_ffvpCodecCtx(NULL),sws_ctx(NULL),temp(NULL),input_width(width), input_height(height)
{
        int dst_bufsize = 0;
        AVCodecID codec_id = AVCodecID(id);
	m_ffvcodec = avcodec_find_encoder(AVCodecID(id));
	if (!m_ffvcodec)  
	{
		goto error; 
	} 
	
	m_ffvpCodecCtx = avcodec_alloc_context3(m_ffvcodec);
	/* put sample parameters */
	m_ffvpCodecCtx->bit_rate = 400000;//设置或者默认400000

	m_ffvpCodecCtx->width = input_width;
	m_ffvpCodecCtx->height = input_height;

	/* frames per second */
	m_ffvpCodecCtx->time_base.num = 1;
	m_ffvpCodecCtx->time_base.den = 25;
	m_ffvpCodecCtx->gop_size = 10; /* emit one intra frame every ten frames */


	if(codec_id == AV_CODEC_ID_H264)
		av_opt_set(m_ffvpCodecCtx->priv_data, "preset", "slow", 0);
	else if (codec_id == AV_CODEC_ID_MJPEG)
	{
		m_ffvpCodecCtx->pix_fmt = AV_PIX_FMT_YUVJ420P;
		m_ffvpCodecCtx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
		m_ffvpCodecCtx->qcompress = g_Quality/100.0;
	}
	else if (codec_id == AV_CODEC_ID_BMP)
	{
		m_ffvpCodecCtx->pix_fmt = /*AV_PIX_FMT_YUV420P*/AV_PIX_FMT_BGR24;
	}
	else if (codec_id == AV_CODEC_ID_PNG)
	{
		m_ffvpCodecCtx->pix_fmt = /*AV_PIX_FMT_YUV420P*/AV_PIX_FMT_YUV420P;
	}


	if (avcodec_open2(m_ffvpCodecCtx, m_ffvcodec, NULL) < 0) 
	{
		goto error; 
	} 

	if(!m_ffvpCodecCtx)
	{
		goto error; 
	}

	temp = avcodec_alloc_frame();
	temp->format = m_ffvpCodecCtx->pix_fmt;
	temp->width  = width;
	temp->height = height;
        dst_bufsize = av_image_alloc(temp->data, temp->linesize, width, height, m_ffvpCodecCtx->pix_fmt, 32);
	if (dst_bufsize<0)
	{
		avcodec_free_frame(&temp);
		temp = NULL;
		goto error;
	}

	/* create scaling context */
	sws_ctx = sws_getContext(width, height, AV_PIX_FMT_YUV420P,
		width, height, m_ffvpCodecCtx->pix_fmt,
		SWS_BICUBIC, NULL, NULL, NULL);
	if (!sws_ctx) 
	{
		goto error;
	}
	
	return;
error:
	b_open = false;
	return;
}

Encoder::~Encoder()
{
	if(temp)
	{
		av_freep(&temp->data[0]);
		avcodec_free_frame(&temp);
		temp = NULL;
	}

	if(sws_ctx)
	{
		sws_freeContext(sws_ctx);
		sws_ctx = NULL;
	}

	m_ffvcodec = NULL;


	if(m_ffvpCodecCtx) 
	{
		avcodec_close(m_ffvpCodecCtx); 	
		av_free(m_ffvpCodecCtx);
		m_ffvpCodecCtx = NULL;
	} 
}

bool Encoder::Is_open()
{
	return b_open;
}



bool Encoder::EncodeVideo(uint8_t* src[4], int src_stride[4] ,  const char* outfile_name)
{

	AVPacket av_pkt;
	int is_data;
    av_init_packet( &av_pkt );
	av_pkt.data = NULL;    // packet data will be allocated by the encoder
    av_pkt.size = 0;
	if(sws_scale(sws_ctx, src,src_stride,0,input_height, temp->data, temp->linesize)<0)
	{
		return false;
	}
    if( avcodec_encode_video2( m_ffvpCodecCtx, &av_pkt, temp, &is_data ) < 0
     || is_data == 0 )
    {
        return false;
    }
	else
	{
		FILE* outfile = fopen(outfile_name, "wb");
		if(outfile)
		{
			fwrite(av_pkt.data, 1, av_pkt.size, outfile);
			fclose(outfile);
		}
		
		av_free_packet(&av_pkt);
	}
	return true;
}
