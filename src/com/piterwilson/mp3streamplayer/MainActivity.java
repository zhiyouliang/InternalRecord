package com.piterwilson.mp3streamplayer;

import java.io.FileInputStream;
import java.io.IOException;
import java.util.HashMap;

import android.app.Activity;
import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;
import android.media.MediaPlayer;
import android.media.SoundPool;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.widget.Button;
import android.widget.ProgressBar;

import com.piterwilson.audio.MP3RadioStreamDelegate;
import com.piterwilson.audio.MP3RadioStreamPlayer;

public class MainActivity extends Activity implements MP3RadioStreamDelegate {
	
	private Button mPlayButton;
	private Button mStopButton;
	private ProgressBar mProgressBar;
	MP3RadioStreamPlayer player;
	private static final String TAG = "MainActivity";
	private boolean mPlaying;
	
	private MediaPlayer mediaPlayer; 
	private SoundPool soundPool;
	private SoundPool soundPool2;
	private HashMap musicId;
	private int soundID = 0;
	private int soundID2 = 0;
	private int streamID = 0;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		
	 
		musicId = new HashMap();
		
		soundPool = new SoundPool(12, 0,5);
		//soundPool2 = new SoundPool(12, 0,5);
		
		//soundID = soundPool.load("/sdcard/pickup_coin.mp3",1);
		//soundID2 = soundPool.load("/sdcard/background.mp3",1);
		
		
		mPlayButton = (Button) this.findViewById(R.id.button1);
		mPlayButton.setOnClickListener(new View.OnClickListener() {
			
			@Override
			public void onClick(View v) {
				//play();
				createMediaplayer("wb.mp3");
				
				
				mPlaying = true; 
				
				//streamID = soundPool.play(soundID,10,10, 1, 0, 1);
				//soundPool.setLoop(streamID, 100);
			}
		});
		mStopButton = (Button) this.findViewById(R.id.button2);
		mStopButton.setOnClickListener(new View.OnClickListener() {
			
			@Override
			public void onClick(View v) {
				//stop();
				mediaPlayer.stop();
				
				mediaPlayer.release();
				
				//soundPool.stop(soundID);
				
				
				/*
				if(mPlaying)
					mediaPlayer.pause();
				else
					mediaPlayer.start();
				
				mPlaying = !mPlaying;
				*/
					
				showGUIStopped();
			}
		});
		mProgressBar = (ProgressBar) this.findViewById(R.id.progressBar1);
		
		showGUIStopped();

	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}
	
	private MediaPlayer createMediaplayer(final String pPath) {
		mediaPlayer = new MediaPlayer();

		try {
			if (pPath.startsWith("/")) {
				final FileInputStream fis = new FileInputStream(pPath);
				mediaPlayer.setDataSource(fis.getFD());
				fis.close();
			} else {
				final AssetFileDescriptor assetFileDescritor = this.getAssets().openFd(pPath);
				mediaPlayer.setDataSource(assetFileDescritor.getFileDescriptor(), assetFileDescritor.getStartOffset(), assetFileDescritor.getLength());
			}
			
			mediaPlayer.setLooping(true);
			mediaPlayer.prepare();

			mediaPlayer.setVolume(10, 10);
			
			mediaPlayer.start();
			showGUIPlaying();
			
		} catch (final Exception e) {
			mediaPlayer = null;
			
		}

		return mediaPlayer;
	}
	
	 
	
	private void play()
	{
		if(player != null)
		{
			player.stop();
			player.release();
			player = null;
			
		}
		
		player = new MP3RadioStreamPlayer();
		player.setUrlString("/sdcard/lsph.mp3");		
		player.setDelegate(this);
		
		showGUIBuffering();
		
		try {
			player.play();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
	
	private void stop()
	{
		player.stop();
	}
	
	private void showGUIBuffering()
	{
		mProgressBar.setVisibility(View.VISIBLE);
		mPlayButton.setEnabled(false);
		mStopButton.setEnabled(false);
	}
	
	private void showGUIPlaying()
	{
		mProgressBar.setVisibility(View.GONE);
		mPlayButton.setEnabled(false);
		mStopButton.setEnabled(true);
	}
	
	private void showGUIStopped()
	{
		mProgressBar.setVisibility(View.GONE);
		mPlayButton.setEnabled(true);
		mStopButton.setEnabled(false);
	}
	
	
	/****************************************
	*
	*	Delegate methods. These are all fired from a background thread so we have to call any GUI code on the main thread.
	*
	****************************************/
	
	@Override
	public void onRadioPlayerPlaybackStarted(MP3RadioStreamPlayer player) {
		Log.i(TAG, "onRadioPlayerPlaybackStarted");;
		this.runOnUiThread(new Runnable(){

			@Override
			public void run() {
				showGUIPlaying();
			}
		});
	}

	@Override
	public void onRadioPlayerStopped(MP3RadioStreamPlayer player) {
		this.runOnUiThread(new Runnable(){

			@Override
			public void run() {
				showGUIStopped();
			}
		});
		
	}

	@Override
	public void onRadioPlayerError(MP3RadioStreamPlayer player) {
		this.runOnUiThread(new Runnable(){

			@Override
			public void run() {
				showGUIStopped();
			}
		});
		
	}

	@Override
	public void onRadioPlayerBuffering(MP3RadioStreamPlayer player) {
		this.runOnUiThread(new Runnable(){

			@Override
			public void run() {
				showGUIBuffering();
			}
		});
		
	}
	 
	    
	    
	    static {
	        System.loadLibrary("internal_audio_record");
	    }
	
}
