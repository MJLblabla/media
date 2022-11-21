//
// Created by 满家乐 on 2022/11/19.
//

#ifndef EXOPLAYER_LOGUTIL_H
#define EXOPLAYER_LOGUTIL_H

#include <android/log.h>

#define LOG_TAG "FfmepgVideoDecoder"
#define LOG_ABLE  1
#define  LOGCATE(...) {  if(LOG_ABLE){ __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__); }}
#define  LOGCATV(...) {  if(LOG_ABLE){ __android_log_print(ANDROID_LOG_VERBOSE,LOG_TAG,__VA_ARGS__);}}
#define  LOGCATD(...) { if(LOG_ABLE){ __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__);}}
#define  LOGCATI(...) { if(LOG_ABLE){ __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__);}}

#endif //EXOPLAYER_LOGUTIL_H
