//
// Created by 满家乐 on 2022/11/18.
//

#include "VideoDecoder.h"

VideoDecoder::VideoDecoder(char *codec_name,
                           int width,
                           int height,
                           const uint8_t *extra_data,
                           int extraSize,
                           int thread_count) {
  codec = avcodec_find_decoder_by_name(codec_name);
  codecContext = avcodec_alloc_context3(codec);
  if (!codecContext) {
    LOGCATE("Failed to allocate avcodec context");
  }
  codecContext->bits_per_coded_sample = 10;
  codecContext->profile = FF_PROFILE_HEVC_MAIN_10;
  codecContext->opaque = nullptr;
  if (extra_data != nullptr) {
    codecContext->extradata_size = extraSize;
    codecContext->extradata = (uint8_t *) av_malloc(extraSize + AV_INPUT_BUFFER_PADDING_SIZE);
    memcpy(codecContext->extradata, extra_data, extraSize);
  }
  AVDictionary *opts = nullptr;
  av_dict_set_int(&opts, "threads", thread_count, 0);
  codecContext->width = width;
  codecContext->height = height;
  int result = avcodec_open2(codecContext, codec, &opts);
  if (result < 0) {
    LOGCATE("Failed to avcodec_open2");
  }
  holdFrame = av_frame_alloc();

}

void VideoDecoder::sendAVPacket(uint8_t *packet,
                                int length,
                                long timeUs,
                                bool isKeyFrame,
                                bool isDecodeOnly,
                                bool isEndOfStream) {

  AVPacket avPacket;
  av_init_packet(&avPacket);
  avPacket.data = packet;
  avPacket.size = length;
  avPacket.pts = timeUs;

  if (isDecodeOnly) {
    avPacket.flags |= AV_PKT_FLAG_DISCARD;
  }

  if (isKeyFrame) {
    avPacket.flags |= AV_PKT_FLAG_KEY;
  }
  int result = avcodec_send_packet(codecContext, &avPacket);
  if (result == 0 && isEndOfStream) {
    result = avcodec_send_packet(codecContext, nullptr);
  }
}

int VideoDecoder::getFrame(JNIEnv *env, jobject javaFFBuff) {
  int error = avcodec_receive_frame(codecContext, holdFrame);
  if (error == 0) {
    LOGCATE("avcodec_receive_frame %ld   , w %d, h %d",
            holdFrame->pts,
            holdFrame->width,
            holdFrame->height);
    if (holdFrame->width != renderWidth || holdFrame->height != renderHeight) {
      onOutPutSizeChange(holdFrame->width,
                         holdFrame->height);
    }

    int y_size = holdFrame->width * holdFrame->height;
    int u_size = holdFrame->width / 2 * holdFrame->height / 2;

    uint8_t *i42OData[3];
    i42OData[0] = i420FrameBuffer;
    i42OData[1] = i420FrameBuffer + y_size;
    i42OData[2] = i420FrameBuffer + y_size + u_size;

    int i42OLinesize[3];
    i42OLinesize[0] = holdFrame->width;
    i42OLinesize[1] = holdFrame->width / 2;
    i42OLinesize[2] = holdFrame->width / 2;

    sws_scale(outSwsContext, holdFrame->data, holdFrame->linesize, 0,
              holdFrame->height, i42OData, i42OLinesize);


    int outputSize = av_image_get_buffer_size(static_cast<AVPixelFormat>(OUT_DST_PIXEL_FORMAT),
                                              holdFrame->width,
                                              holdFrame->height,
                                              1);
    // Populate JNI References.
    jclass outputBufferClass = env->FindClass(
        "androidx/media3/decoder/ffmpeg/FfmpegOutputFrame");
    auto dataFunc =
        env->GetMethodID(outputBufferClass, "getData", "()Ljava/nio/ByteBuffer;");
    auto initForInfo =
        env->GetMethodID(outputBufferClass, "init", "(IIIJ)V");
    // get pointer to the data buffer.
    auto dataObject = env->CallObjectMethod(javaFFBuff, dataFunc);
    auto *const data =
        reinterpret_cast<jbyte *>(env->GetDirectBufferAddress(dataObject));

    av_image_copy_to_buffer((uint8_t *) data,
                            outputSize,
                            i42OData,
                            i42OLinesize,
                            static_cast<AVPixelFormat>(OUT_DST_PIXEL_FORMAT),
                            holdFrame->width,
                            holdFrame->height,
                            1);
    env->CallVoidMethod(
        javaFFBuff, initForInfo, holdFrame->width, holdFrame->height, outputSize, holdFrame->pts);

    return outputSize;
  } else if (error == AVERROR(EAGAIN)) {
    // packet还不够
    return 0;
  } else if (error == AVERROR_EOF || error == AVERROR_INVALIDDATA) {
    return 0;
  } else {
    return -2;
  }
}

void VideoDecoder::reset(uint8_t *ext, int width, int height) {
  if (codecContext == nullptr) {
    LOGCATE("Tried to reset without a context");
    return;
  }
  avcodec_flush_buffers(codecContext);
  LOGCATE("VideoDecoder::reset avcodec_flush_buffers");
  int ret = 0;
  while (ret == 0) {
    ret = avcodec_receive_frame(codecContext, holdFrame);
    LOGCATE("VideoDecoder::reset  %d ", ret);
  }
}

VideoDecoder::~VideoDecoder() {

  if (codecContext != nullptr) {
    avcodec_close(codecContext);
    avcodec_free_context(&codecContext);
    codecContext = nullptr;
    codec = nullptr;
  }
  if (holdFrame != nullptr) {
    av_frame_free(&holdFrame);
    holdFrame = nullptr;
  }
  if (rgbaFrameBuffer != nullptr) {
    free(rgbaFrameBuffer);
    rgbaFrameBuffer = nullptr;
  }

  if (renderSwsContext != nullptr) {
    sws_freeContext(renderSwsContext);
    renderSwsContext = nullptr;
  }
  if (nativeWindow)
    ANativeWindow_release(nativeWindow);
  nativeWindow = nullptr;

  if (outSwsContext != nullptr) {
    sws_freeContext(outSwsContext);
    outSwsContext = nullptr;
  }
  if (i420FrameBuffer != nullptr) {
    free(i420FrameBuffer);
    i420FrameBuffer = nullptr;
  }
}

void VideoDecoder::setSurface(JNIEnv *env, jobject surface) {
  if (nativeWindow) {
    ANativeWindow_release(nativeWindow);
  }
  nativeWindow = ANativeWindow_fromSurface(env, surface);
}

void VideoDecoder::onOutPutSizeChange(int width, int height) {

  renderWidth = width;
  renderHeight = height;

  if (rgbaFrameBuffer != nullptr) {
    free(rgbaFrameBuffer);
    rgbaFrameBuffer = nullptr;
  }

  if (renderSwsContext != nullptr) {
    sws_freeContext(renderSwsContext);
    renderSwsContext = nullptr;
  }

  int bufferSize = av_image_get_buffer_size(RENDER_DST_PIXEL_FORMAT, width, height, 1);
  rgbaFrameBuffer = (uint8_t *) av_malloc(bufferSize * sizeof(uint8_t));

  renderSwsContext = sws_getContext(width, height, OUT_DST_PIXEL_FORMAT,
                                    width, height, RENDER_DST_PIXEL_FORMAT,
                                    SWS_FAST_BILINEAR, NULL, NULL, NULL);

  if (outSwsContext != nullptr) {
    sws_freeContext(outSwsContext);
    outSwsContext = nullptr;
  }
  if (i420FrameBuffer != nullptr) {
    free(i420FrameBuffer);
    i420FrameBuffer = nullptr;
  }

  bufferSize = av_image_get_buffer_size(OUT_DST_PIXEL_FORMAT, width, height, 1);
  i420FrameBuffer = (uint8_t *) av_malloc(width * height * 3 / 2 * sizeof(uint8_t));

  outSwsContext = sws_getContext(width, height, codecContext->pix_fmt,
                                 width, height, OUT_DST_PIXEL_FORMAT,
                                 SWS_FAST_BILINEAR, NULL, NULL, NULL);
}

void VideoDecoder::renderToSurface(JNIEnv *env,
                                   jobject surface,
                                   uint8_t *frame,
                                   int length,
                                   int width,
                                   int height) {
  if (nativeWindow == nullptr) {
    return;
  }

  int y_size = width * height;
  int u_size = width / 2 * height / 2;
  int v_size = width / 2 * height / 2;
  int linesize[3];
  linesize[0] = width;
  linesize[1] = width / 2;
  linesize[2] = width / 2;
  uint8_t *data[3];
  data[0] = frame;
  data[1] = frame + y_size;
  data[2] = frame + y_size + u_size;

  uint8_t *rgbaData[3];
  rgbaData[0] = rgbaFrameBuffer;
  int rgbaLinesize[3];
  rgbaLinesize[0] = width * 4;

  sws_scale(renderSwsContext, data, linesize, 0,
            holdFrame->height, rgbaData, rgbaLinesize);

  ANativeWindow_setBuffersGeometry(nativeWindow, width,
                                   height, WINDOW_FORMAT_RGBA_8888);
  ANativeWindow_lock(nativeWindow, &m_NativeWindowBuffer, nullptr);
  auto *dstBuffer = static_cast<uint8_t *>(m_NativeWindowBuffer.bits);
  int srcLineSize = width * 4;//RGBA
  int dstLineSize = m_NativeWindowBuffer.stride * 4;
  LOGCATE("renderToSurface %d, %d, %d, %d,", srcLineSize, dstLineSize, width, height);
  for (int i = 0; i < renderHeight; ++i) {
    memcpy(dstBuffer + i * dstLineSize, rgbaFrameBuffer + i * srcLineSize, srcLineSize);
  }
  ANativeWindow_unlockAndPost(nativeWindow);
}

