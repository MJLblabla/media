//
// Created by 满家乐 on 2022/11/18.
//
#ifndef EXOPLAYER_VIDEODECODER_H
#define EXOPLAYER_VIDEODECODER_H

#include <cstdint>
#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "LogUtil.h"
#include "libavutil/imgutils.h"

}

class VideoDecoder {
 private:
  AVCodecContext *codecContext = nullptr;
  AVCodec *codec = nullptr;
  ANativeWindow *nativeWindow = nullptr;
  ANativeWindow_Buffer m_NativeWindowBuffer;

  SwsContext *renderSwsContext = nullptr;
  const AVPixelFormat RENDER_DST_PIXEL_FORMAT = AV_PIX_FMT_RGBA;
  uint8_t *rgbaFrameBuffer = nullptr;

  SwsContext *outSwsContext = nullptr;
  uint8_t *i420FrameBuffer = nullptr;
  const AVPixelFormat OUT_DST_PIXEL_FORMAT = AV_PIX_FMT_YUV420P;

  int renderWidth = 0;
  int renderHeight = 0;
  void onOutPutSizeChange(int width, int height);


 public:
  AVFrame *holdFrame = nullptr;
  ~VideoDecoder();
  VideoDecoder(char *codec_name,
               int width,
               int height,
               const uint8_t *extra_data,
               int extraSize,
               int thread_count);

  void
  renderToSurface(JNIEnv *env, jobject surface, uint8_t *frame, int length, int width, int height);

  void setSurface(JNIEnv *env, jobject surface);

  void sendAVPacket(uint8_t *packet,
                    int length,
                    long timeUs,
                    bool isKeyFrame,
                    bool isDecodeOnly,
                    bool isEndOfStream);

  int getFrame(JNIEnv *env, jobject javaFFBuff);

  void reset(uint8_t *ext, int width, int height);

};

#endif //EXOPLAYER_VIDEODECODER_H
