#include <jni.h>
#include <cstdint>
#include "VideoDecoder.h"


//
// Created by 满家乐 on 2022/11/18.
//

extern "C"
JNIEXPORT jlong JNICALL
Java_androidx_media3_decoder_ffmpeg_FfmepgVideoDecoder_ffmpegInit(JNIEnv *env,
                                                                            jobject thiz,
                                                                            jstring codec_name,
                                                                            jint width,
                                                                            jint height,
                                                                            jbyteArray extra_data,
                                                                            jint thread_count) {

  jsize size = env->GetArrayLength(extra_data);
  auto *extra_temp = static_cast<uint8_t *>(malloc(size));
  env->GetByteArrayRegion(extra_data, 0, size, (jbyte *) extra_temp);

  char *codecNameChars = const_cast<char *>(env->GetStringUTFChars(codec_name, NULL));
  auto *videoDecoder = new VideoDecoder(codecNameChars,
                                        (int) width,
                                        (int) height,
                                        extra_temp,
                                        (int) size,
                                        (int) thread_count);

  env->ReleaseStringUTFChars(codec_name, codecNameChars);
  free(extra_temp);
  return reinterpret_cast<jlong>(videoDecoder);
}

extern "C"
JNIEXPORT void JNICALL
Java_androidx_media3_decoder_ffmpeg_FfmepgVideoDecoder_ffmpegClose(JNIEnv *env,
                                                                             jobject thiz,
                                                                             jlong context) {
  auto *videoDecoder = reinterpret_cast<VideoDecoder *>(context);
  delete videoDecoder;
}

extern "C"
JNIEXPORT jint JNICALL
Java_androidx_media3_decoder_ffmpeg_FfmepgVideoDecoder_ffmpegGetFrame(JNIEnv *env,
                                                                                jobject thiz,
                                                                                jlong context,
                                                                                jobject output) {
  auto *videoDecoder = reinterpret_cast<VideoDecoder *>(context);
  int ret = videoDecoder->getFrame(env, output);
  return ret;
}


extern "C"
JNIEXPORT void JNICALL
Java_androidx_media3_decoder_ffmpeg_FfmepgVideoDecoder_ffmpegReset(JNIEnv *env,
                                                                             jobject thiz,
                                                                             jlong context,
                                                                             jbyteArray extra_data,
                                                                             jint w,
                                                                             jint h
                                                                             ) {
  auto *videoDecoder = reinterpret_cast<VideoDecoder *>(context);
  jsize size = env->GetArrayLength(extra_data);
  auto *extra_temp = static_cast<uint8_t *>(malloc(size));
  env->GetByteArrayRegion(extra_data, 0, size, (jbyte *) extra_temp);

  videoDecoder->reset(extra_temp,w,h);
  free(extra_temp);
}
extern "C"
JNIEXPORT void JNICALL
Java_androidx_media3_decoder_ffmpeg_FfmepgVideoDecoder_ffmpegSendAVPacket(JNIEnv *env,
                                                                                    jobject thiz,
                                                                                    jlong context,
                                                                                    jobject encoded,
                                                                                    jint length,
                                                                                    jlong time_us,
                                                                                    jboolean is_key_frame,
                                                                                    jboolean is_decode_only,
                                                                                    jboolean is_end_of_stream) {

  auto *packetBuffer = (uint8_t *) env->GetDirectBufferAddress(encoded);
  auto *videoDecoder = reinterpret_cast<VideoDecoder *>(context);
  videoDecoder->sendAVPacket(packetBuffer,
                             length,
                             (long) time_us,
                             is_key_frame,
                             is_decode_only,
                             is_end_of_stream);

}

extern "C"
JNIEXPORT void JNICALL
Java_androidx_media3_decoder_ffmpeg_FfmepgVideoDecoder_renderSurface(JNIEnv *env,
                                                                               jobject thiz,
                                                                               jlong context,
                                                                               jobject surface,
                                                                               jobject encoded,
                                                                               jint length,
                                                                               jint w,
                                                                               jint h) {
  auto *frame = (uint8_t *) env->GetDirectBufferAddress(encoded);
  auto *videoDecoder = reinterpret_cast<VideoDecoder *>(context);
  videoDecoder->renderToSurface(env, surface, frame, length, w, h);
}
extern "C"
JNIEXPORT void JNICALL
Java_androidx_media3_decoder_ffmpeg_FfmepgVideoDecoder_setSurface(JNIEnv *env,
                                                                            jobject thiz,
                                                                            jlong context,
                                                                            jobject surface) {
  auto *videoDecoder = reinterpret_cast<VideoDecoder *>(context);
  videoDecoder->setSurface(env, surface);
}