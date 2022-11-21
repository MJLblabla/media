package androidx.media3.decoder.ffmpeg;

import androidx.media3.decoder.VideoDecoderOutputBuffer;
import java.nio.ByteBuffer;

public class FfmpegOutputFrame {

  public VideoDecoderOutputBuffer videoDecoderOutputBuffer = null;

  public ByteBuffer getData() {
    assert videoDecoderOutputBuffer.data != null;
    return videoDecoderOutputBuffer.data;
  }

  public void init(int width, int height, int len, long time) {
    videoDecoderOutputBuffer.width = width;
    videoDecoderOutputBuffer.height = height;
    videoDecoderOutputBuffer.timeUs = time;

    int yLength = width * height;
    int uLength = width / 2 * height / 2;
    int vLength = width / 2 * height / 2;

    if (videoDecoderOutputBuffer.yuvPlanes == null) {
      videoDecoderOutputBuffer.yuvPlanes = new ByteBuffer[3];
    }
    ByteBuffer data = videoDecoderOutputBuffer.data;
    ByteBuffer[] yuvPlanes = videoDecoderOutputBuffer.yuvPlanes;
    // Rewrapping has to be done on every frame since the stride might have changed.
    assert data != null;
    yuvPlanes[0] = data.slice();
    yuvPlanes[0].limit(yLength);
    data.position(yLength);
    yuvPlanes[1] = data.slice();
    yuvPlanes[1].limit(uLength);
    data.position(yLength + uLength);
    yuvPlanes[2] = data.slice();
    yuvPlanes[2].limit(vLength);

    if (videoDecoderOutputBuffer.yuvStrides == null) {
      videoDecoderOutputBuffer.yuvStrides = new int[3];
    }
    videoDecoderOutputBuffer.yuvStrides[0] = width;
    videoDecoderOutputBuffer.yuvStrides[1] = width / 2;
    videoDecoderOutputBuffer.yuvStrides[2] = width / 2;
  }
}
