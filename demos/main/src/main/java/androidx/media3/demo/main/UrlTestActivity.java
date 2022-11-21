package androidx.media3.demo.main;

import android.os.Bundle;
import android.util.Log;
import android.view.View;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.media3.common.MediaItem;
import androidx.media3.common.MimeTypes;
import androidx.media3.common.PlaybackException;
import androidx.media3.common.Player;
import androidx.media3.exoplayer.ExoPlayer;
import androidx.media3.ui.PlayerView;

public class UrlTestActivity extends AppCompatActivity {

  ExoPlayer player = null;

  @Override
  protected void onCreate(@Nullable Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_url_test);
    findViewById(R.id.test).setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        Log.d("player", player.isCurrentMediaItemLive() + "");
        player.seekToDefaultPosition();
      }
    });
    player = new ExoPlayer.Builder(this)

        .build();
    player.addListener(new Player.Listener() {
      @Override
      public void onPlayerError(PlaybackException error) {
        Player.Listener.super.onPlayerError(error);
        if (error.errorCode == PlaybackException.ERROR_CODE_BEHIND_LIVE_WINDOW) {
          player.seekToDefaultPosition();
          player.prepare();
        }
      }
    });
    PlayerView playerView = findViewById(R.id.player_view);
    playerView.setPlayer(player);
    String url = getIntent().getStringExtra("url");
    MediaItem mediaItem =
        new MediaItem.Builder()
            .setUri(url)
            .setLiveConfiguration(
                new MediaItem.LiveConfiguration.Builder()
                    .setTargetOffsetMs(2000)
                    .setMinOffsetMs(1000)
                    .setMaxOffsetMs(3000)
                    .setMinPlaybackSpeed(1f)
                    .setMaxPlaybackSpeed(2.0f)
                    .build())
            .build();

// Set the media item to be played.
    player.setMediaItem(mediaItem);
// Prepare the player.
    player.prepare();
// Start the playback.
    player.play();
  }

  @Override
  protected void onDestroy() {
    player.release();
    super.onDestroy();
  }

}