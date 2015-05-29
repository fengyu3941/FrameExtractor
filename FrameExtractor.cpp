/*
 * Copyright (c) 2012 Stefano Sabatini
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * @file
 * libavformat demuxing API use example.
 *
 * Show how to use the libavformat and libavcodec API to demux and
 * decode audio and video data.
 * @example doc/examples/demuxing.c
 */
#define __STDC_CONSTANT_MACROS
extern "C"
{
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
}
#include <Encoder.h>

static AVFormatContext *fmt_ctx = NULL;
static AVCodecContext *video_dec_ctx = NULL, *audio_dec_ctx;
static AVStream *video_stream = NULL, *audio_stream = NULL;
static const char *src_filename = NULL;
static int nb_milliSec = 0;
static const char *output_folder = NULL;
static int out_file_ext_num = 0;
static const char* output_file_extensions[] = {"bmp","jpg"};
static const enum AVCodecID ids[] = {AV_CODEC_ID_BMP, AV_CODEC_ID_MJPEG};
static int output_index = 0;
static int width = 0;
static int  height = 0;
static int64_t last_pts = 0;


static uint8_t *video_dst_data[4] = {NULL};
static int      video_dst_linesize[4];
static int video_dst_bufsize;

static int video_stream_idx = -1, audio_stream_idx = -1;
static AVFrame *frame = NULL;
static AVPacket pkt;
static int video_frame_count = 0;
static int audio_frame_count = 0;
static int pts = 0;



static int decode_packet(int *got_frame, int cached)
{
    int ret = 0;
    int decoded = pkt.size;
	//tmp members
	int b_write  = 0;
	int m_width;
	int m_height;
	int id;
	const char* output_file_extension;
	char out_file_name[1024];
	void* p = NULL;
	
	

    if (pkt.stream_index == video_stream_idx) {

		if(video_stream->avg_frame_rate.num>0 && video_stream->avg_frame_rate.den>0)
		{
			pts += 1000000*video_stream->avg_frame_rate.den/video_stream->avg_frame_rate.num;
		}
		else if (pkt.pts>0)
		{
			pts = pkt.pts;
		}
		else if(pkt.dts>0)
		{
			pts  = pkt.dts;
		}
		else
		{
			pts += 1000000/25;
		}
		if ( 0 == last_pts)
		{
			last_pts = pts;
		}
		if (pts - last_pts >= nb_milliSec*1000)
		{
			b_write = 1;
		}
		

        /* decode video frame */
        ret = avcodec_decode_video2(video_dec_ctx, frame, got_frame, &pkt);
        if (ret < 0) {
            fprintf(stderr, "Error decoding video frame\n");
            return ret;
        }

        if (*got_frame) {
            //printf("video_frame%s n:%d coded_n:%d pts:%s\n",
            //       cached ? "(cached)" : "",
            //       video_frame_count++, frame->coded_picture_number,
            //       av_ts2timestr(frame->pts, &video_dec_ctx->time_base));

            /* copy decoded frame to destination buffer:
             * this is required since rawvideo expects non aligned data */
            av_image_copy(video_dst_data, video_dst_linesize,
                          (const uint8_t **)(frame->data), frame->linesize,
                          video_dec_ctx->pix_fmt, video_dec_ctx->width, video_dec_ctx->height);
			if (b_write>0)
			{
				last_pts = pts;

				m_width = video_dec_ctx->width;
				m_height = video_dec_ctx->height;
				id = AV_CODEC_ID_BMP;
				output_file_extension = "bmp";
				out_file_name[1024];
				if (width!=0 && height !=0)
				{
					m_height = height;
					m_width = width;
				}
				if (out_file_ext_num>=0 && out_file_ext_num<=1)
				{
					id = ids[out_file_ext_num];
					output_file_extension = output_file_extensions[out_file_ext_num];
				}
				
				
				p = createEncoder(id, m_width, m_height);
				if (p)
				{
					
					memset(out_file_name, 0 ,1024);
					if(output_folder)
					{
						sprintf(out_file_name, "%s\\%06d.%s", output_folder, output_index++, output_file_extension); 
					}
					else
					{
						sprintf(out_file_name, "%06d.%s", output_index++, output_file_extension); 
					}

					EncodeVideo(p, video_dst_data, video_dst_linesize, out_file_name);
					destroyEncoder(p);
					p = NULL;
				}
			}
			
			
			
            /* write to rawvideo file */
        }
    } else if (pkt.stream_index == audio_stream_idx) {
        /* decode audio frame */
        ret = avcodec_decode_audio4(audio_dec_ctx, frame, got_frame, &pkt);
        if (ret < 0) {
            fprintf(stderr, "Error decoding audio frame\n");
            return ret;
        }
        /* Some audio decoders decode only part of the packet, and have to be
         * called again with the remainder of the packet data.
         * Sample: fate-suite/lossless-audio/luckynight-partial.shn
         * Also, some decoders might over-read the packet. */
        decoded = FFMIN(ret, pkt.size);

        if (*got_frame) {
            size_t unpadded_linesize = frame->nb_samples * av_get_bytes_per_sample((AVSampleFormat)(frame->format));
            //printf("audio_frame%s n:%d nb_samples:%d pts:%s\n",
            //       cached ? "(cached)" : "",
            //       audio_frame_count++, frame->nb_samples,
            //       av_ts2timestr(frame->pts, &audio_dec_ctx->time_base));

            /* Write the raw audio data samples of the first plane. This works
             * fine for packed formats (e.g. AV_SAMPLE_FMT_S16). However,
             * most audio decoders output planar audio, which uses a separate
             * plane of audio samples for each channel (e.g. AV_SAMPLE_FMT_S16P).
             * In other words, this code will write only the first audio channel
             * in these cases.
             * You should use libswresample or libavfilter to convert the frame
             * to packed data. */
        }
    }

    return decoded;
}

static int open_codec_context(int *stream_idx,
                              AVFormatContext *fmt_ctx, enum AVMediaType type)
{
    int ret;
    AVStream *st;
    AVCodecContext *dec_ctx = NULL;
    AVCodec *dec = NULL;

    ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not find %s stream in input file '%s'\n",
                av_get_media_type_string(type), src_filename);
        return ret;
    } else {
        *stream_idx = ret;
        st = fmt_ctx->streams[*stream_idx];

        /* find decoder for the stream */
        dec_ctx = st->codec;
        dec = avcodec_find_decoder(dec_ctx->codec_id);
        if (!dec) {
            fprintf(stderr, "Failed to find %s codec\n",
                    av_get_media_type_string(type));
            return ret;
        }

        if ((ret = avcodec_open2(dec_ctx, dec, NULL)) < 0) {
            fprintf(stderr, "Failed to open %s codec\n",
                    av_get_media_type_string(type));
            return ret;
        }
    }

    return 0;
}

static int get_format_from_sample_fmt(const char **fmt,
                                      enum AVSampleFormat sample_fmt)
{
    int i;
    struct sample_fmt_entry {
        enum AVSampleFormat sample_fmt; const char *fmt_be, *fmt_le;
    } sample_fmt_entries[] = {
        { AV_SAMPLE_FMT_U8,  "u8",    "u8"    },
        { AV_SAMPLE_FMT_S16, "s16be", "s16le" },
        { AV_SAMPLE_FMT_S32, "s32be", "s32le" },
        { AV_SAMPLE_FMT_FLT, "f32be", "f32le" },
        { AV_SAMPLE_FMT_DBL, "f64be", "f64le" },
    };
    *fmt = NULL;

    for (i = 0; i < FF_ARRAY_ELEMS(sample_fmt_entries); i++) {
        struct sample_fmt_entry *entry = &sample_fmt_entries[i];
        if (sample_fmt == entry->sample_fmt) {
            *fmt = AV_NE(entry->fmt_be, entry->fmt_le);
            return 0;
        }
    }

    fprintf(stderr,
            "sample format %s is not supported as output format\n",
            av_get_sample_fmt_name(sample_fmt));
    return -1;
}

int main (int argc, char **argv)
{
    int ret = 0, got_frame;

   
        fprintf(stderr, "usage: %s input_file [nb_milliSec] [output_folder] [output_file_extension] [width] [height]\n"
                "Program extract frames from an input file every nb_milliSec milliseconds.\n"
                "This program reads frames from a file, decodes them, and write a frame every nb_milliSec milliseconds.\n"
				"input_file: path to be read.\n"
				"nb_milliSec:  Optional,time to write a frame; default 0, all images will be write\n"
				"output_folder: Optional, output folder, default current path\n"
				"output_file_extension: Optional,image file type; 0 for bmp, 1 for jpg;default bmp\n"
				"width: Optional,image width, default the video width\n"
				"height: Optional,image height, default the video height\n"
                "\n", argv[0]);
	if (argc <2) {
        exit(1);
    }
    src_filename = argv[1];
	if (argc>=3)
	{
		nb_milliSec = atoi(argv[2]);
	}
	if (argc>=4)
	{
		output_folder = argv[3];
	}
	if (argc>=5)
	{
		out_file_ext_num = atoi(argv[4]);
	}
	if (argc>=6)
	{
		width = atoi(argv[5]);
	}
	if (argc>=7)
	{
		height = atoi(argv[6]);
	}

	

    /* register all formats and codecs */
    av_register_all();

    /* open input file, and allocate format context */
    if (avformat_open_input(&fmt_ctx, src_filename, NULL, NULL) < 0) {
        fprintf(stderr, "Could not open source file %s\n", src_filename);
        exit(1);
    }

    /* retrieve stream information */
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        exit(1);
    }

    if (open_codec_context(&video_stream_idx, fmt_ctx, AVMEDIA_TYPE_VIDEO) >= 0) {
        video_stream = fmt_ctx->streams[video_stream_idx];
        video_dec_ctx = video_stream->codec;


        /* allocate image where the decoded image will be put */
        ret = av_image_alloc(video_dst_data, video_dst_linesize,
                             video_dec_ctx->width, video_dec_ctx->height,
                             video_dec_ctx->pix_fmt, 1);
        if (ret < 0) {
            fprintf(stderr, "Could not allocate raw video buffer\n");
            goto end;
        }
        video_dst_bufsize = ret;
    }

    if (open_codec_context(&audio_stream_idx, fmt_ctx, AVMEDIA_TYPE_AUDIO) >= 0) {
        audio_stream = fmt_ctx->streams[audio_stream_idx];
        audio_dec_ctx = audio_stream->codec;

    }

    /* dump input information to stderr */
    av_dump_format(fmt_ctx, 0, src_filename, 0);

    if (!audio_stream && !video_stream) {
        fprintf(stderr, "Could not find audio or video stream in the input, aborting\n");
        ret = 1;
        goto end;
    }

    frame = avcodec_alloc_frame();
    if (!frame) {
        fprintf(stderr, "Could not allocate frame\n");
        ret = AVERROR(ENOMEM);
        goto end;
    }

    /* initialize packet, set data to NULL, let the demuxer fill it */
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;


    /* read frames from the file */
    while (av_read_frame(fmt_ctx, &pkt) >= 0) {
        AVPacket orig_pkt = pkt;
        do {
            ret = decode_packet(&got_frame, 0);
            if (ret < 0)
                break;
            pkt.data += ret;
            pkt.size -= ret;
        } while (pkt.size > 0);
        av_free_packet(&orig_pkt);
    }

    /* flush cached frames */
    pkt.data = NULL;
    pkt.size = 0;
    do {
        decode_packet(&got_frame, 1);
    } while (got_frame);

    printf("Demuxing succeeded.\n");



    if (audio_stream) {
        enum AVSampleFormat sfmt = audio_dec_ctx->sample_fmt;
        int n_channels = audio_dec_ctx->channels;
        const char *fmt;

        if (av_sample_fmt_is_planar(sfmt)) {
            const char *packed = av_get_sample_fmt_name(sfmt);
            printf("Warning: the sample format the decoder produced is planar "
                   "(%s). This example will output the first channel only.\n",
                   packed ? packed : "?");
            sfmt = av_get_packed_sample_fmt(sfmt);
            n_channels = 1;
        }

        if ((ret = get_format_from_sample_fmt(&fmt, sfmt)) < 0)
            goto end;
    }

end:
    if (video_dec_ctx)
        avcodec_close(video_dec_ctx);
    if (audio_dec_ctx)
        avcodec_close(audio_dec_ctx);
    avformat_close_input(&fmt_ctx);
    av_free(frame);
    av_free(video_dst_data[0]);

    return ret < 0;
}
