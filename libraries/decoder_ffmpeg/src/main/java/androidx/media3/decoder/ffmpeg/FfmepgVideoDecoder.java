package androidx.media3.decoder.ffmpeg;


import android.view.Surface;
import androidx.annotation.Nullable;

import androidx.media3.common.C;
import androidx.media3.common.Format;
import androidx.media3.common.util.Assertions;
import androidx.media3.common.util.Log;
import androidx.media3.common.util.Util;
import androidx.media3.decoder.CryptoConfig;
import androidx.media3.decoder.CryptoInfo;
import androidx.media3.decoder.DecoderInputBuffer;
import androidx.media3.decoder.SimpleDecoder;
import androidx.media3.decoder.VideoDecoderOutputBuffer;
import java.io.File;
import java.io.FileFilter;
import java.nio.ByteBuffer;
import java.util.List;
import java.util.regex.Pattern;

public class FfmepgVideoDecoder extends
    SimpleDecoder<DecoderInputBuffer, VideoDecoderOutputBuffer, FfmpegDecoderException> {

  private long nativeContext = 0; // May be reassigned on resetting the codec
  private volatile @C.VideoOutputMode int outputMode;
  @Nullable
  private final byte[] extraData;
  private static final int AUDIO_DECODER_ERROR_INVALID_DATA = -1;
  private static final int AUDIO_DECODER_ERROR_OTHER = -2;
  private Surface surface = null;

  private int lastW = 0;
  private int lastH = 0;

  public FfmepgVideoDecoder(
      Format format,
      int numInputBuffers,
      int numOutputBuffers,
      int initialInputBufferSize,
      @Nullable CryptoConfig cryptoConfig) throws FfmpegDecoderException {
    super(new DecoderInputBuffer[numInputBuffers], new VideoDecoderOutputBuffer[numOutputBuffers]);

    assert format.sampleMimeType != null;
    String codecName = Assertions.checkNotNull(FfmpegLibrary.getCodecName(format.sampleMimeType));
    extraData = getExtraData(format.sampleMimeType, format.initializationData);

    lastH = format.height;
    lastW = format.width;
    nativeContext = ffmpegInit(codecName,
        format.width,
        format.height,
        extraData,
        getCpuNumCores() + 1);
    if (nativeContext == 0) {
      throw new FfmpegDecoderException("Failed to initialize decoder");
    }
    setInitialInputBufferSize(initialInputBufferSize);
  }

  private static int getCpuNumCores() {
    try {
      //Get directory containing CPU info
      File dir = new File("/sys/devices/system/cpu/");
      //Filter to only list the devices we care about
      File[] files = dir.listFiles(new FileFilter() {
        @Override
        public boolean accept(File pathname) {
          return Pattern.matches("cpu[0-9]+", pathname.getName());
        }
      });
      //Return the number of cores (virtual CPU devices)
      return files.length;
    } catch (Exception e) {
      //Default to return 1 core
      return 1;
    }
  }

  private byte[] getExtraData(String mimeType, List<byte[]> initializationData) {
    byte[] extraData = null;
    if (initializationData.size() > 1) {
      int extraDataLength = 0;
      for (byte[] data : initializationData) {
        // 加2个分割
        if (extraDataLength != 0) {
          extraDataLength += 2;
        }
        extraDataLength += data.length + 2;
      }

      if (extraDataLength > 0) {
        extraData = new byte[extraDataLength];
        int currentPos = 0;
        for (byte[] data : initializationData) {
          if (currentPos != 0) {
            extraData[currentPos] = 0;
            extraData[currentPos + 1] = 0;
            currentPos += 2;
          }
          extraData[currentPos] = (byte) (data.length >> 8);
          extraData[currentPos + 1] = (byte) (data.length & 0xFF);
          System.arraycopy(data, 0, extraData, currentPos + 2, data.length);
          currentPos += data.length + 2;
        }
      }
    } else if (initializationData.size() == 1) {
      byte[] data = initializationData.get(0);
      if (data != null && data.length > 0) {
        extraData = new byte[data.length];
        System.arraycopy(data, 0, extraData, 0, data.length);
      }
    }
    return extraData;
  }

  @Override
  public String getName() {
    return "FfmepgVideoDecoder";
  }

  @Override
  protected DecoderInputBuffer createInputBuffer() {
    return new DecoderInputBuffer(DecoderInputBuffer.BUFFER_REPLACEMENT_MODE_DIRECT);
  }

  @Override
  protected VideoDecoderOutputBuffer createOutputBuffer() {
    return new VideoDecoderOutputBuffer(this::releaseOutputBuffer);
  }

  @Override
  protected void releaseOutputBuffer(VideoDecoderOutputBuffer outputBuffer) {
    super.releaseOutputBuffer(outputBuffer);
    if (outputMode == C.VIDEO_OUTPUT_MODE_SURFACE_YUV && !outputBuffer.isDecodeOnly()) {
    }
  }

  @Override
  protected FfmpegDecoderException createUnexpectedDecodeException(Throwable error) {
    return new FfmpegDecoderException("Unexpected decode error", error);
  }

  protected void setDecoderOutputMode(@C.VideoOutputMode int outputMode) {
    this.outputMode = outputMode;
  }

  protected void renderOutputBufferToSurface(VideoDecoderOutputBuffer outputBuffer,
      Surface surface) {
    if (this.surface != surface) {
      setSurface(nativeContext, surface);
      this.surface = surface;
    }
    assert outputBuffer.data != null;
    renderSurface(nativeContext, surface, outputBuffer.data,
        outputBuffer.data.limit(), outputBuffer.width,
        outputBuffer.height);
    outputBuffer.release();
  }

  @Nullable
  @Override
  protected FfmpegDecoderException decode(DecoderInputBuffer inputBuffer,
      VideoDecoderOutputBuffer outputBuffer, boolean reset) {
    assert inputBuffer.format != null;
    if (reset || inputBuffer.format.width != lastW || inputBuffer.format.height != lastH
    ) {
      lastW = inputBuffer.format.width;
      lastH = inputBuffer.format.height;
      ffmpegReset(nativeContext, extraData, inputBuffer.format.width, inputBuffer.format.height);
    }
    ByteBuffer inputData = Util.castNonNull(inputBuffer.data);
    int inputSize = inputData.limit();
    CryptoInfo cryptoInfo = inputBuffer.cryptoInfo;
    Log.d("FfmepgVideoDecoder",
        "ffmpegSendAVPacket  " + inputBuffer.format.width + "  " + inputBuffer.format.height);
    ffmpegSendAVPacket(nativeContext, inputData, inputSize, inputBuffer.timeUs,
        inputBuffer.isKeyFrame(),
        inputBuffer.isDecodeOnly(),
        inputBuffer.isEndOfStream());

    if (!inputBuffer.isDecodeOnly()) {
      if (outputBuffer.data == null ||
          outputBuffer.data.capacity() != inputBuffer.format.width * inputBuffer.format.height * 2
      ) {
        assert inputBuffer.format != null;
        outputBuffer.data = ByteBuffer.allocateDirect(
            inputBuffer.format.width * inputBuffer.format.height * 2);
      }
      outputBuffer.mode = outputMode;
      FfmpegOutputFrame ffmpegOutputFrame = new FfmpegOutputFrame();
      ffmpegOutputFrame.videoDecoderOutputBuffer = outputBuffer;
      int result = ffmpegGetFrame(nativeContext, ffmpegOutputFrame);
      if (result <= 0) {
        // There's no need to output empty buffers.
        outputBuffer.setFlags(C.BUFFER_FLAG_DECODE_ONLY);
        return null;
      } else {
        outputBuffer.clearFlag(C.BUFFER_FLAG_DECODE_ONLY);
      }
      Log.d("FfmepgVideoDecoder",
          "decode  timeUs" + outputBuffer.timeUs + "  len" + result);
      outputBuffer.data.position(0);
      outputBuffer.data.limit(
          result
      );
    }
    return null;
  }

  @Override
  public void release() {
    super.release();
    ffmpegClose(nativeContext);
  }

  private native long ffmpegInit(String codecName, int width, int height,
      byte[] extraData, int threadCount);

  private native void ffmpegClose(long context);

  private native void ffmpegSendAVPacket(
      long context,
      ByteBuffer encoded,
      int length,
      long timeUs,
      boolean isKeyFrame,
      boolean isDecodeOnly,
      boolean isEndOfStream
  );

  private native void renderSurface(
      long context,
      Surface surface,
      ByteBuffer encoded,
      int length,
      int w, int h
  );

  private native void setSurface(
      long context,
      Surface surface
  );

  private native int ffmpegGetFrame(long context, FfmpegOutputFrame output);

  private native void ffmpegReset(long context, @Nullable byte[] extraData, int w, int h);

}
