#include <assert.h>
#include <string.h>
#include <jni.h>

#include <stdbool.h>

#include <android/log.h>

#include <libavcodec/avcodec.h>
#include <libavcodec/opt.h>
#include <libavformat/avformat.h>
#include <libavformat/rtsp.h>
#include <libswscale/swscale.h>

/**
 * If non-zero, debugging will be produced that shows wall timings during
 * critical phases of the frame writing process.
 */
#define PROFILE_WRITE_FRAME 1

/* XXX: This used to be in ffmpeg... */
#define MAX_STREAMS 20

#if 1
#define LOG_TAG "ffmpeg-jni"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#else
#define LOGE
#define LOGI
#define LOGW
#define LOGD
#define LOGV
#endif

static jint jniThrowException(JNIEnv *env, const char *className,
        const char *msg) {
    jclass clazz = (*env)->FindClass(env, className);
    if (clazz == NULL) {
        LOGE("jniThrowException failed: cannot find className=%s", className);
        return -1;
    }

    return (*env)->ThrowNew(env, clazz, msg);
}

static jint jniThrowOOM(JNIEnv *env) {
    return jniThrowException(env, "java/lang/OutOfMemoryError", NULL);
}

static jint jniThrowNPE(JNIEnv *env, const char *msg) {
    return jniThrowException(env, "java/lang/NullPointerException", msg);
}

/*****************************************************************************/
/* init...                                                                   */
/*****************************************************************************/

static void dalvik_log_callback(void *ptr, int level, const char *fmt, va_list vl) {
    /* Build up the line until there is an LF. */
    static char line[1024] = { '\0' };
    char *line_ptr;

    size_t line_len = strlen(line);
    vsnprintf(line + line_len, sizeof(line) - line_len, fmt, vl);

    while ((line_ptr = strchr(line, '\n'))) {
        *line_ptr = '\0';
        const char *outfmt = "[ffmpeg] %s";
        switch (level) {
            case AV_LOG_QUIET:
            case AV_LOG_PANIC:
            case AV_LOG_FATAL:
            case AV_LOG_ERROR:     break;
            case AV_LOG_WARNING:  break;
            case AV_LOG_INFO:      break;
            case AV_LOG_VERBOSE:   break;
            case AV_LOG_DEBUG:
            default:              LOGD(outfmt, line); break;
            
           // case AV_LOG_ERROR:    LOGE(outfmt, line); break;
           // case AV_LOG_WARNING:  LOGW(outfmt, line); break;
           // case AV_LOG_INFO:     LOGI(outfmt, line); break;
           // case AV_LOG_VERBOSE:  LOGV(outfmt, line); break;
           // case AV_LOG_DEBUG:
           // default:              LOGD(outfmt, line); break;
        }
        strcpy(line, line_ptr + 1);
    }
}

jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    /* Register all formats. */
    av_register_all();

    /* No-op on Linux I think, but ffmpeg bitches if I don't call this. */
    avformat_network_init();

    /* Capture ffmpeg logging. */
    av_log_set_callback(dalvik_log_callback);

    return JNI_VERSION_1_4;
}

/*****************************************************************************/
/* timing helpers                                                            */
/*****************************************************************************/

/* Subtract the `struct timeval' values X and Y,
   storing the result in RESULT.
   Return 1 if the difference is negative, otherwise 0.  */
int timeval_subtract(struct timeval *result, struct timeval *x,
         struct timeval *y) {
    /* Perform the carry for the later subtraction by updating y. */
    if (x->tv_usec < y->tv_usec) {
        int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
        y->tv_usec -= 1000000 * nsec;
        y->tv_sec += nsec;
    }
    if (x->tv_usec - y->tv_usec > 1000000) {
        int nsec = (x->tv_usec - y->tv_usec) / 1000000;
        y->tv_usec += 1000000 * nsec;
        y->tv_sec -= nsec;
    }

    /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_usec = x->tv_usec - y->tv_usec;

    /* Return 1 if result is negative. */
    return x->tv_sec < y->tv_sec;
}

static void store_elapsed(long *elapsed_msec, struct timeval *then) {
    struct timeval now, elapsed;

    gettimeofday(&now, NULL);
    timeval_subtract(&elapsed, &now, then);

    *elapsed_msec = (elapsed.tv_sec * 1000) + (elapsed.tv_usec / 1000);
}

static void print_elapsed(const char *str, struct timeval *then) {
    long elapsed_msec;
    store_elapsed(&elapsed_msec, then);
    LOGD("%s took %ld msec", str, elapsed_msec);
}

/*****************************************************************************/
/* org.devtcg.rojocam.ffmpeg.FFStreamConfig                                  */
/*****************************************************************************/

typedef struct {
    int num_streams;
    AVStream *streams[MAX_STREAMS];
    const char *title;
} FFStreamConfig;

static int opt_default(const char *opt, const char *arg,
        AVCodecContext *avctx, int type) {
    int ret = 0;
    const AVOption *o = av_find_opt(avctx, opt, NULL, type, type);
    if (o != NULL) {
        ret = av_set_string3(avctx, opt, arg, 1, NULL);
    }
    return ret;
}

jint Java_org_devtcg_rojocam_ffmpeg_FFStreamConfig_nativeCreate(JNIEnv *env,
        jclass clazz) {
    FFStreamConfig *defaultConfig = NULL;
    AVCodecContext *videoEnc = NULL;
    AVStream *st = NULL;

    defaultConfig = av_mallocz(sizeof(FFStreamConfig));
    if (defaultConfig == NULL) {
        jniThrowOOM(env);
        goto fail;
    }
    defaultConfig->title = "rojocam feed";
//CODEC_ID_MPEG4
    AVCodec *codec = avcodec_find_encoder(CODEC_ID_MPEG4);

    videoEnc = avcodec_alloc_context2(AVMEDIA_TYPE_VIDEO);
    if (videoEnc == NULL) {
        jniThrowOOM(env);
        goto fail;
    }

    /* XXX: We should attempt to encode with parameters that match our camera
     * preview input to reduce overhead, but being in a proof-of-concept phase
     * I'd rather stick to the simpler, less variable approach. */
    videoEnc->time_base.num =1;
    videoEnc->time_base.den = 25;
    videoEnc->bit_rate = 180000;
    videoEnc->width = 320;
    videoEnc->height =240;
    videoEnc->pix_fmt = PIX_FMT_YUV420P;

    /* This apparently modifies the SDP created by avf_sdp_create.  In my
     * experiments it looks like it does indeed add the "config=" parameter to
     * the fmtp directive in SDP. No idea what that does, though... */
    if (opt_default("flags", "+global_header", videoEnc,
            AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_ENCODING_PARAM)) {
        LOGE("opt_default: flags, +global_header failed!");
        jniThrowException(env, "java/IO/IOException", "Failed to set encoder options");
        goto fail;
    }

    if (avcodec_open(videoEnc, codec) < 0) {
        LOGE("avcodec_open failed!");
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        goto fail;
    }

    st = av_mallocz(sizeof(AVStream));
    if (st == NULL) {
        jniThrowOOM(env);
        goto fail;
    }

    st->index = defaultConfig->num_streams;
    st->codec = videoEnc;
    defaultConfig->streams[defaultConfig->num_streams++] = st;

    return (jint)defaultConfig;

fail:
    if (defaultConfig != NULL) {
        av_free(defaultConfig);
    }
    if (videoEnc != NULL) {
        av_free(videoEnc);
    }
    if (st != NULL) {
        av_free(st);
    }
    assert((*env)->ExceptionOccurred(env));
    return 0;
}

jstring Java_org_devtcg_rojocam_ffmpeg_FFStreamConfig_nativeGetSDPDescription(JNIEnv *env,
        jclass clazz, jint nativeInt) {
    FFStreamConfig *config = (FFStreamConfig *)nativeInt;
    AVFormatContext *avc;
    AVStream *avs = NULL;
    char buf[2048];
    int i;

    avc = avformat_alloc_context();
    if (avc == NULL) {
        jniThrowOOM(env);
        return NULL;
    }

    av_dict_set(&avc->metadata, "title",
            config->title ? config->title : "No Title", 0);

    avc->nb_streams = config->num_streams;
    snprintf(avc->filename, sizeof(avc->filename), "rtp://0.0.0.0");

    avc->streams = av_malloc(avc->nb_streams * sizeof(*avc->streams));
    avs = av_malloc(avc->nb_streams * sizeof(*avs));

    for (i = 0; i < avc->nb_streams; i++) {
        avc->streams[i] = &avs[i];
        avc->streams[i]->codec = config->streams[i]->codec;
    }

    memset(buf, 0, sizeof(buf));
    avf_sdp_create(&avc, 1, buf, sizeof(buf));
    av_free(avc->streams);
    av_dict_free(&avc->metadata);
    av_free(avc);
    av_free(avs);

    return (*env)->NewStringUTF(env, buf);
}

void Java_org_devtcg_rojocam_ffmpeg_FFStreamConfig_nativeDestroy(JNIEnv *env,
        jclass clazz, jint nativeInt) {
    FFStreamConfig *config = (FFStreamConfig *)nativeInt;
    int i;

    for (i = 0; i < config->num_streams; i++) {
        AVStream *stream = config->streams[i];
        av_free(stream->codec);
        av_free(stream);
    }

    av_free(config);
}

/*****************************************************************************/
/* org.devtcg.rojocam.ffmpeg.RtpOutputContext                                */
/*****************************************************************************/

typedef struct {
    FFStreamConfig *config;
    URLContext *urlContext;
    AVFormatContext *avContext;

    /* The time the last frame was sent, or the time this object was created to
     * begin. */
    int64_t lastFrameTime;

    /**
     * Context we use to convert our NV21 frames into something our codec can
     * handle.
     */
    struct SwsContext *imgConvert;

    /**
     * Temporary buffer into which we put our output frame data.
     */
    AVFrame *tempFrame;

    /**
     * Holds the encoded video data while encoding and delivering to the RTP
     * peer.
     */
    uint8_t tempEncodedBuf[200000];

#if PROFILE_WRITE_FRAME
    long resampling_time;
    long encoding_time;
    long write_time;
#endif
} RtpOutputContext;

static void free_av_format_context(AVFormatContext *avContext) {
    /* XXX: I'm hesitant to call avformat_free_context as we have not taken the
     * "normal" API flow here by manually constructing this object so much...
     * */
    av_free(avContext->streams);
    av_free(avContext);
}

static void rtp_output_context_free(RtpOutputContext *rtpContext) {
    if (rtpContext->urlContext != NULL) {
        url_close(rtpContext->urlContext);
    }
    if (rtpContext->avContext != NULL) {
        free_av_format_context(rtpContext->avContext);
    }
    if (rtpContext->tempFrame != NULL) {
        avpicture_free((AVPicture *)rtpContext->tempFrame);
        av_free(rtpContext->tempFrame);
    }
    if (rtpContext->imgConvert != NULL) {
        sws_freeContext(rtpContext->imgConvert);
    }
    av_free(rtpContext);
}

jint Java_org_devtcg_rojocam_ffmpeg_RtpOutputContext_nativeCreate(JNIEnv *env,
        jclass clazz, jint streamConfigNativeInt, jlong nowNanoTime,
        jstring hostAddress, jint rtpPort) {
    FFStreamConfig *config = (FFStreamConfig *)streamConfigNativeInt;
    RtpOutputContext *rtpContext = NULL;
    AVFormatContext *avContext = NULL;
    AVStream *st = NULL;
    uint8_t *dummy_buf;
    int max_packet_size;

    rtpContext = av_mallocz(sizeof(RtpOutputContext));
    if (rtpContext == NULL) {
        jniThrowOOM(env);
        goto cleanup;
    }

    rtpContext->lastFrameTime = nowNanoTime;
    rtpContext->config = config;

    avContext = avformat_alloc_context();
    if (avContext == NULL) {
        jniThrowOOM(env);
        goto cleanup;
    }
    avContext->oformat = av_guess_format("rtp", NULL, NULL);
    if (avContext->oformat == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException",
                "rtp avformat is not available");
        goto cleanup;
    }

    rtpContext->avContext = avContext;

    st = av_mallocz(sizeof(AVStream));
    if (st == NULL) {
        jniThrowOOM(env);
        goto cleanup;
    }
    avContext->nb_streams = 1;
    avContext->streams = av_malloc(avContext->nb_streams * sizeof(*avContext->streams));
    avContext->streams[0] = st;

    /* XXX: What would we be doing if we supported audio as well? */
    memcpy(st, config->streams[0], sizeof(AVStream));
    st->priv_data = NULL;

    const jbyte *hostAddress_str = (*env)->GetStringUTFChars(env,
            hostAddress, NULL);
    snprintf(avContext->filename, sizeof(avContext->filename),
            "rtp://%s:%d?localrtpport=5000&localrtcpport=5001",
            hostAddress_str, rtpPort);
    (*env)->ReleaseStringUTFChars(env, hostAddress, hostAddress_str);

    if (url_open(&rtpContext->urlContext,
            avContext->filename, URL_WRONLY) < 0) {
        LOGE("Cannot open url context for filename=%s", avContext->filename);
        jniThrowException(env, "java/io/IOException", "Unable to open URL");
        goto cleanup;
    }

    max_packet_size = url_get_max_packet_size(rtpContext->urlContext);

    /* XXX: No idea what purpose this serves... */
    url_open_dyn_packet_buf(&avContext->pb, max_packet_size);

    av_set_parameters(avContext, NULL);
    if (av_write_header(avContext) < 0) {
        jniThrowException(env, "java/io/IOException", "Unexpected error writing dummy RTP header");
        goto cleanup;
    }

    url_close_dyn_buf(avContext->pb, &dummy_buf);
    av_free(dummy_buf);

    return (jint)rtpContext;

cleanup:
    rtp_output_context_free(rtpContext);
    assert((*env)->ExceptionOccurred(env));

    return 0;
}

jint Java_org_devtcg_rojocam_ffmpeg_RtpOutputContext_nativeGetLocalRtpPort(JNIEnv *env,
        jclass clazz, jint nativeInt) {
    RtpOutputContext *rtpContext = (RtpOutputContext *)nativeInt;
    return ff_rtp_get_local_rtp_port(rtpContext->urlContext);
}

jint Java_org_devtcg_rojocam_ffmpeg_RtpOutputContext_nativeGetLocalRtcpPort(JNIEnv *env,
        jclass clazz, jint nativeInt) {
    RtpOutputContext *rtpContext = (RtpOutputContext *)nativeInt;
    return ff_rtp_get_local_rtcp_port(rtpContext->urlContext);
}

static AVFrame *alloc_picture(enum PixelFormat pix_fmt, int width, int height) {
    AVFrame *picture;
    uint8_t *picture_buf;
    int size;

    picture = avcodec_alloc_frame();
    if (picture == NULL) {
        return NULL;
    }
    size = avpicture_get_size(pix_fmt, width, height);
    picture_buf = av_mallocz(size);
    if (picture_buf == NULL) {
        av_free(picture);
        return NULL;
    }
    avpicture_fill((AVPicture *)picture, picture_buf,
            pix_fmt, width, height);
    return picture;
}

static int androidPixFmtToFFmpeg(jint androidPixFmt) {
    /* See android.graphics.ImageFormat */
    switch (androidPixFmt) {
        case 0x11: return PIX_FMT_NV21;
        default: abort();
    }
}

/**
 * Encode our raw camera picture to an output packet, ultimately to be written
 * using RTP.  The resulting packet has been timescaled to match the output
 * stream.
 *
 * @return True if the packet was written; false if the picture was buffered.
 */
static bool encode_video_frame(RtpOutputContext *rtpContext,
        AVStream *stream, AVFrame *tempFrame,
        struct SwsContext *imgConvert, uint8_t *outbuf, int outbuf_size,
        jbyte *data, jlong frameTime, jint frameDuration,
        jint frameFormat, jint frameWidth, jint frameHeight,
        jint frameBitsPerPixel, AVPacket *pkt) {
    AVFrame frame;
    AVPicture *picture = (AVPicture *)&frame;
    AVCodecContext *c;
    int n;

    c = stream->codec;

#if PROFILE_WRITE_FRAME
    struct timeval then;
    gettimeofday(&then, NULL);
#endif

    /* XXX: Hmm, we're resampling here but perhaps this is a mistake.  Why not
     * just define the codec context to match our input close enough for fast
     * translation? */
    avcodec_get_frame_defaults(&frame);
    avpicture_fill(picture, data, androidPixFmtToFFmpeg(frameFormat),
            frameWidth, frameHeight);

    sws_scale(imgConvert, picture->data, picture->linesize, 0,
            frameHeight, tempFrame->data, tempFrame->linesize);

#if PROFILE_WRITE_FRAME
    store_elapsed(&rtpContext->resampling_time, &then);
#endif

#if PROFILE_WRITE_FRAME
    gettimeofday(&then, NULL);
#endif
    tempFrame->pts = av_rescale_q(frameTime, AV_TIME_BASE_Q, c->time_base);
    n = avcodec_encode_video(c, outbuf, outbuf_size, tempFrame);
#if PROFILE_WRITE_FRAME
    store_elapsed(&rtpContext->encoding_time, &then);
#endif
    if (n > 0) {
        av_init_packet(pkt);

        if (c->coded_frame->pts != AV_NOPTS_VALUE) {
            pkt->pts = av_rescale_q(c->coded_frame->pts,
                    c->time_base, stream->time_base);
        }

        if (c->coded_frame->key_frame) {
            pkt->flags |= AV_PKT_FLAG_KEY;
        }

        pkt->dts = pkt->pts;
        pkt->duration = av_rescale_q(frameDuration, AV_TIME_BASE_Q, stream->time_base);
        pkt->stream_index = stream->index;
        pkt->data = outbuf;
        pkt->size = n;

        return true;
    } else {
        return false;
    }
}

static ssize_t exhaustive_send(URLContext *urlContext, uint8_t *packetized_data,
    int packetized_data_len) {
    uint8_t *ptr = packetized_data;
    uint8_t *end = packetized_data + packetized_data_len;
    ssize_t numBytes = 0;

    /* XXX: Do we need packet clock synchronization?  I don't think so... */
    while (ptr < end) {
        int remaining = end - ptr;
        int len;

        /* The size for each packet is actually encoded in the first 4 bytes of
         * each of a series of packets contained in `rtp_data'. */
        assert(remaining >= 4);

        len = ptr[0] << 24 | ptr[1] << 16 | ptr[2] << 8 | ptr[3];
        assert(len <= remaining - 4);

        ptr += 4;
        int n = url_write(urlContext, ptr, len);
        ptr += len;

        /* XXX: Our poor error handling comes courtesy of ffserver.c */
        if (n > 0) {
            numBytes += n;
        }
    }

    return numBytes;
}

/**
 * Perform some lazy initialization of the context on the first frame write,
 * assuming that all future frame writes will have the same dimensions.
 * Blargh, our API should be restructured.
 */
static bool first_frame_init(JNIEnv *env, RtpOutputContext *rtpContext,
        jint frameFormat, jint frameWidth, jint frameHeight) {
    AVCodecContext *codec = rtpContext->avContext->streams[0]->codec;
    rtpContext->tempFrame = alloc_picture(codec->pix_fmt,
            codec->width, codec->height);
    if (rtpContext->tempFrame == NULL) {
        jniThrowOOM(env);
        return false;
    }

    rtpContext->imgConvert = sws_getContext(frameWidth, frameHeight,
            androidPixFmtToFFmpeg(frameFormat),
            codec->width, codec->height, codec->pix_fmt,
            SWS_BICUBIC, NULL, NULL, NULL);
    if (rtpContext->imgConvert == NULL) {
        jniThrowOOM(env);
        return false;
    }

    return true;
}

/* XXX: This class should really pass the picture parameters by a separate API
 * so that we can, by contract, enforce that the frame size can't suddenly
 * change on us. */
void Java_org_devtcg_rojocam_ffmpeg_RtpOutputContext_nativeWriteFrame(JNIEnv *env,
        jclass clazz, jint nativeInt, jbyteArray data, jlong frameTime,
        jint frameFormat, jint frameWidth, jint frameHeight,
        jint frameBitsPerPixel) {
    RtpOutputContext *rtpContext = (RtpOutputContext *)nativeInt;
    AVFormatContext *avContext;
    AVCodecContext *codec;
    AVStream *outputStream;
    AVPacket pkt;
    jbyte *data_c;
    int max_packet_size;
    uint8_t *rtp_data;
    int rtp_data_len;

    avContext = rtpContext->avContext;
    outputStream = avContext->streams[0];
    codec = outputStream->codec;

    if (rtpContext->tempFrame == NULL) {
        if (!first_frame_init(env, rtpContext, frameFormat,
                frameWidth, frameHeight)) {
            LOGE("Error initializing encoding buffers, cannot stream");
            return;
        }
    }

    data_c = (*env)->GetByteArrayElements(env, data, NULL);

    /* Convert the input arguments to an AVPacket, simulating it as though we
     * read this from the ffmpeg libraries but there was no need to do this as
     * it was passed into us already as a raw video frame. */
    int frameDuration = frameTime - rtpContext->lastFrameTime;
    bool frameEncoded = encode_video_frame(rtpContext,
            outputStream, rtpContext->tempFrame, rtpContext->imgConvert,
            rtpContext->tempEncodedBuf, sizeof(rtpContext->tempEncodedBuf),
            data_c, frameTime, frameDuration, frameFormat,
            frameWidth, frameHeight, frameBitsPerPixel, &pkt);
    rtpContext->lastFrameTime = frameTime;

    (*env)->ReleaseByteArrayElements(env, data, data_c, JNI_ABORT);

    if (frameEncoded) {
#if PROFILE_WRITE_FRAME
        struct timeval then;
        gettimeofday(&then, NULL);
#endif

        max_packet_size = url_get_max_packet_size(rtpContext->urlContext);
        url_open_dyn_packet_buf(&avContext->pb, max_packet_size);

        avContext->pb->seekable = 0;

        /* This organizes our encoded packet into RTP packet segments (but it
         * doesn't actually send anything over the network yet). */
        if (av_write_frame(avContext, &pkt) < 0) {
            jniThrowException(env, "java/io/IOException", "Error writing frame to output");
        }

        /* Actually deliver the packetized RTP data to the remote peer. */
        rtp_data_len = url_close_dyn_buf(avContext->pb, &rtp_data);
        exhaustive_send(rtpContext->urlContext, rtp_data, rtp_data_len);
        av_free(rtp_data);

        /* XXX: I dunno, ffserver.c does this... */
        outputStream->codec->frame_number++;

#if PROFILE_WRITE_FRAME
        store_elapsed(&rtpContext->write_time, &then);
#endif
    } else {
#if PROFILE_WRITE_TIME
        rtpContext->write_time = 0;
#endif
    }

#if PROFILE_WRITE_FRAME
    //LOGI("resample@%ld ms; encode@%ld ms; write@%ld ms",
     //       rtpContext->resampling_time, rtpContext->encoding_time,
      //      rtpContext->write_time);
#endif
}

jint Java_org_devtcg_rojocam_ffmpeg_RtpOutputContext_nativeClose(JNIEnv *env,
        jclass clazz, jint nativeInt) {
    RtpOutputContext *rtpContext = (RtpOutputContext *)nativeInt;
    rtp_output_context_free(rtpContext);
}

/*****************************************************************************/
/* org.devtcg.rojocam.ffmpeg.SwsScaler                                       */
/*****************************************************************************/

typedef struct {
    struct SwsContext *swsContext;
    int srcPixFmt;
    int srcWidth;
    int srcHeight;
    int dstPixFmt;
    int dstWidth;
    int dstHeight;
} SwsScaler;

static void swsscaler_free(SwsScaler *scaler) {
    if (scaler->swsContext != NULL) {
        sws_freeContext(scaler->swsContext);
    }
    av_free(scaler);
}

jint Java_org_devtcg_rojocam_ffmpeg_SwsScaler_nativeCreate(JNIEnv *env,
        jclass clazz,
        jint srcPixFmt, jint srcWidth, jint srcHeight,
        jint dstPixFmt, jint dstWidth, jint dstHeight, jint flags) {
    SwsScaler *scaler = NULL;

    scaler = av_mallocz(sizeof(SwsScaler));
    if (scaler == NULL) {
        jniThrowOOM(env);
        goto cleanup;
    }

    scaler->swsContext = sws_getCachedContext(NULL,
            srcWidth, srcHeight, srcPixFmt,
            dstWidth, dstHeight, dstPixFmt, flags, NULL, NULL, NULL);
    if (scaler->swsContext == NULL) {
        jniThrowOOM(env);
        goto cleanup;
    }

    scaler->srcPixFmt = srcPixFmt;
    scaler->srcWidth = srcWidth;
    scaler->srcHeight = srcHeight;
    scaler->dstPixFmt = dstPixFmt;
    scaler->dstWidth = dstWidth;
    scaler->dstHeight = dstHeight;

    return (jint)scaler;

cleanup:
    swsscaler_free(scaler);
    assert((*env)->ExceptionOccurred(env));

    return 0;
}

void Java_org_devtcg_rojocam_ffmpeg_SwsScaler_nativeScale(JNIEnv *env,
        jclass clazz, jint nativeInt, jbyteArray src, jbyteArray dst) {
    SwsScaler *scaler;
    AVPicture srcPic;
    AVPicture dstPic;
    jbyte *src_c;
    jbyte *dst_c;

    if (nativeInt == 0 || src == NULL || dst == NULL) {
        jniThrowNPE(env, NULL);
        return;
    }

    scaler = (SwsScaler *)nativeInt;

    src_c = (*env)->GetByteArrayElements(env, src, NULL);
    dst_c = (*env)->GetByteArrayElements(env, dst, NULL);

    avpicture_fill(&srcPic, src_c, scaler->srcPixFmt,
            scaler->srcWidth, scaler->srcHeight);
    avpicture_fill(&dstPic, dst_c, scaler->dstPixFmt,
            scaler->dstWidth, scaler->dstHeight);

    sws_scale(scaler->swsContext, srcPic.data, srcPic.linesize, 0,
            scaler->srcHeight, dstPic.data, dstPic.linesize);

    (*env)->ReleaseByteArrayElements(env, src, src_c, JNI_ABORT);
    (*env)->ReleaseByteArrayElements(env, dst, dst_c, 0);
}

void Java_org_devtcg_rojocam_ffmpeg_SwsScaler_nativeDestroy(JNIEnv *env,
        jclass clazz, jint nativeInt) {
    SwsScaler *scaler = (SwsScaler *)nativeInt;
    swsscaler_free(scaler);
}

jint Java_org_devtcg_rojocam_ffmpeg_SwsScaler_nativePixFmtNV21() { return PIX_FMT_NV21; }
jint Java_org_devtcg_rojocam_ffmpeg_SwsScaler_nativePixFmtRGBA() { return PIX_FMT_RGBA; }
jint Java_org_devtcg_rojocam_ffmpeg_SwsScaler_nativePixFmtRGB565BE() { return PIX_FMT_RGB565BE; }
