
#pragma hdrstop
#include "precompiled.h"

extern idCVar s_noSound;

#include "tr_local.h"

#if defined( USE_FFMPEG )
// Carl: ffmpg for bink video files
extern "C"
{
	//#ifdef WIN32
	#ifndef INT64_C
		#define INT64_C(c) (c ## LL)
		#define UINT64_C(c) (c ## ULL)
	#endif
	//#include <inttypes.h>
	//#endif

	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libswscale/swscale.h>
}
#endif

class idCinematicBIK : public idCinematic {
public:
	idCinematicBIK();
	virtual					~idCinematicBIK();

	virtual bool			InitFromFile( const char* qpath, bool looping );
	virtual cinData_t		ImageForTime( int milliseconds );
	virtual int				AnimationLength() const;
	virtual void			Close();
	virtual void			ResetTime( int time );

	virtual int				GetStartTime() const;
	virtual void			ExportToTGA( bool skipExisting = true );
	virtual float			GetFrameRate() const;
	bool                    IsPlaying() const;

private:
#if defined( USE_FFMPEG )
	int						video_stream_index;
	AVFormatContext *		fmt_ctx;
	AVFrame *				frame;
	AVFrame *				frame2;
	AVCodec *				dec;
	AVCodecContext *		dec_ctx;
	SwsContext *			img_convert_ctx;
	int						framePos;
	bool					hasFrame;

	cinData_t				ImageForTimeFFMPEG( int milliseconds );
	bool					InitFromFFMPEGFile( const char* qpath, bool looping );
	void					FFMPEGReset();
#endif

	idStr					fileName;
	int						CIN_WIDTH, CIN_HEIGHT;
	idFile * 				iFile;
	cinStatus_t				status;

	int						animationLength;
	int						startTime;
	float					frameRate;

	idImage *				img;
	byte * 					imageData;

	bool					looping;

	ID_INLINE void BIKShutdown()
	{
		if( status == FMV_IDLE )
		{
			return;
		}
		status = FMV_IDLE;

		if( iFile )
		{
			fileSystem->CloseFile( iFile );
			iFile = NULL;
		}

		fileName.Clear();
	}
};

/*
==============
idCinematicLocal::InitCinematic
==============
*/
void InitCinematicBIK()
{
#if defined( USE_FFMPEG )
	// Carl: ffmpeg for Bink and regular video files
	common->DPrintf( "...Loading FFMPEG ...\n" );
	avcodec_register_all();
	av_register_all();
#endif
}
/*
==============
idCinematicLocal::ShutdownCinematic
==============
*/
void ShutdownCinematicBIK()
{

}
/*
========================================================
InitROQFromFile
========================================================
*/
idCinematic * InitBIKFromFile( const char* path, bool looping )
{
	auto cin = new( TAG_CINEMATIC ) idCinematicBIK;
	if( !cin->InitFromFile( path, looping ) )
	{
		delete cin;
		return nullptr;
	}
	return cin;
}

/*
==============
idCinematicBIK::idCinematicBIK
==============
*/
idCinematicBIK::idCinematicBIK()
{
#if defined( USE_FFMPEG )
	// Carl: ffmpeg stuff, for bink and normal video files:

	//	fmt_ctx = avformat_alloc_context();
  #if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55,28,1)
	frame = av_frame_alloc();
	frame2 = av_frame_alloc();
  #else
	frame = avcodec_alloc_frame();
	frame2 = avcodec_alloc_frame();
  #endif // LIBAVCODEC_VERSION_INT
	dec_ctx = NULL;
	fmt_ctx = NULL;
	video_stream_index = -1;
	img_convert_ctx = NULL;
	hasFrame = false;
#endif

	// Carl: Original Doom 3 RoQ files:
	imageData = NULL;
	status = FMV_EOF;
	iFile = NULL;
	img = renderImageManager->CreateStandaloneImage( "_cinematicBIK" );
	if( img != NULL )
	{
		idImageOpts opts;
		opts.format = FMT_RGBA8;
		opts.colorFormat = CFM_DEFAULT;
		opts.width = 32;
		opts.height = 32;
		opts.numLevels = 1;

		idSamplerOpts smp;
		smp.filter = TF_LINEAR;
		smp.wrap = TR_REPEAT;

		img->AllocImage( opts, smp );
	}
}

/*
==============
idCinematicBIK::~idCinematicBIK
==============
*/
idCinematicBIK::~idCinematicBIK()
{
	Close();

#if defined( USE_FFMPEG )
	// Carl: ffmpeg for bink and other video files:

	// RB: TODO double check this. It seems we have different versions of ffmpeg on Kubuntu 13.10 and the win32 development files
  #if defined(_WIN32) || defined(_WIN64)
	av_frame_free( &frame );
	av_frame_free( &frame2 );
  #else
	av_freep( &frame );
	av_freep( &frame2 );
  #endif

	if( fmt_ctx )
	{
		avformat_free_context( fmt_ctx );
	}

	if( img_convert_ctx )
	{
		sws_freeContext( img_convert_ctx );
	}
#endif

	renderImageManager->DestroyStandaloneImage( img );
	img = NULL;
}

#if defined( USE_FFMPEG )
/*
==============
idCinematicBIK::InitFromFFMPEGFile
==============
*/
bool idCinematicBIK::InitFromFFMPEGFile( const char* qpath, bool amilooping )
{
	looping = amilooping;
	startTime = 0;
	CIN_HEIGHT = DEFAULT_CIN_HEIGHT;
	CIN_WIDTH = DEFAULT_CIN_WIDTH;

	idStr fullpath;
	idFile* testFile = fileSystem->OpenFileRead( qpath );
	if( testFile )
	{
		fullpath = testFile->GetFullPath();
		fileSystem->CloseFile( testFile );
	}
	// RB: case sensitivity HACK for Linux
	else if( idStr::Cmpn( qpath, "sound/vo", 8 ) == 0 )
	{
		idStr newPath( qpath );
		newPath.Replace( "sound/vo", "sound/VO" );

		testFile = fileSystem->OpenFileRead( newPath );
		if( testFile )
		{
			fullpath = testFile->GetFullPath();
			fileSystem->CloseFile( testFile );
		}
		else {
			common->Warning( "idCinematicBIK: Cannot open FFMPEG video file: '%s', %d\n", qpath, looping );
			return false;
		}
	}

	//idStr fullpath = fileSystem->RelativePathToOSPath( qpath, "fs_basepath" );
	int ret;
	if( ( ret = avformat_open_input( &fmt_ctx, fullpath, NULL, NULL ) ) < 0 )
	{
		common->Warning( "idCinematicBIK: Cannot open FFMPEG video file: '%s', %d\n", qpath, looping );
		return false;
	}
	if( ( ret = avformat_find_stream_info( fmt_ctx, NULL ) ) < 0 )
	{
		common->Warning( "idCinematicBIK: Cannot find stream info: '%s', %d\n", qpath, looping );
		return false;
	}
	/* select the video stream */
	ret = av_find_best_stream( fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0 );
	if( ret < 0 )
	{
		common->Warning( "idCinematicBIK: Cannot find a video stream in: '%s', %d\n", qpath, looping );
		return false;
	}
	video_stream_index = ret;
	dec_ctx = fmt_ctx->streams[ video_stream_index ]->codec;
	/* init the video decoder */
	if( ( ret = avcodec_open2( dec_ctx, dec, NULL ) ) < 0 )
	{
		common->Warning( "idCinematicBIK: Cannot open video decoder for: '%s', %d\n", qpath, looping );
		return false;
	}

	CIN_WIDTH = dec_ctx->width;
	CIN_HEIGHT = dec_ctx->height;
	/** Calculate Duration in seconds
	* This is the fundamental unit of time (in seconds) in terms
	* of which frame timestamps are represented. For fixed-fps content,
	* timebase should be 1/framerate and timestamp increments should be
	* identically 1.
	* - encoding: MUST be set by user.
	* - decoding: Set by libavcodec.
	*/
	AVRational avr = dec_ctx->time_base;
	/**
	* For some codecs, the time base is closer to the field rate than the frame rate.
	* Most notably, H.264 and MPEG-2 specify time_base as half of frame duration
	* if no telecine is used ...
	*
	* Set to time_base ticks per frame. Default 1, e.g., H.264/MPEG-2 set it to 2.
	*/
	int ticksPerFrame = dec_ctx->ticks_per_frame;
	float durationSec = static_cast< double >( fmt_ctx->streams[ video_stream_index ]->duration ) * static_cast< double >( ticksPerFrame ) / static_cast< double >( avr.den );
	animationLength = durationSec * 1000;
	frameRate = av_q2d( fmt_ctx->streams[ video_stream_index ]->avg_frame_rate );
	hasFrame = false;
	framePos = -1;
	common->Printf( "Loaded FFMPEG file: '%s', looping=%d%dx%d, %f FPS, %f sec\n", qpath, looping, CIN_WIDTH, CIN_HEIGHT, frameRate, durationSec );
	imageData = ( byte* ) Mem_Alloc( CIN_WIDTH * CIN_HEIGHT * 4 * 2, TAG_CINEMATIC );
	avpicture_fill( ( AVPicture* ) frame2, imageData, AV_PIX_FMT_BGR32, CIN_WIDTH, CIN_HEIGHT );
	if( img_convert_ctx )
	{
		sws_freeContext( img_convert_ctx );
	}
	img_convert_ctx = sws_getContext( dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt, CIN_WIDTH, CIN_HEIGHT, AV_PIX_FMT_BGR32, SWS_BICUBIC, NULL, NULL, NULL );
	status = FMV_PLAY;

	startTime = 0;
	ImageForTime( 0 );
	status = ( looping ) ? FMV_PLAY : FMV_IDLE;

	//startTime = sys->Milliseconds();

	return true;
}
/*
==============
idCinematicBIK::FFMPEGReset
==============
*/
void idCinematicBIK::FFMPEGReset()
{
	// RB: don't reset startTime here because that breaks video replays in the PDAs
	//startTime = 0;

	framePos = -1;

	if( av_seek_frame( fmt_ctx, video_stream_index, 0, 0 ) >= 0 )
	{
		status = FMV_LOOPED;
	}
	else {
		status = FMV_EOF;
	}
}
#endif // USE_FFMPEG

/*
==============
idCinematicBIK::InitFromFile
==============
*/
bool idCinematicBIK::InitFromFile( const char* qpath, bool amilooping )
{
	Close();

	animationLength = 100000;

#if defined( USE_FFMPEG )
	{
		animationLength = 0;
		hasFrame = false;
		BIKShutdown();
	}
	fileName = qpath;
	return InitFromFFMPEGFile( fileName, amilooping );
#else
	animationLength = 0;
	return false;
#endif
}

/*
==============
idCinematicBIK::Close
==============
*/
void idCinematicBIK::Close()
{
	if( imageData )
	{
		Mem_Free( ( void* ) imageData );
		imageData = NULL;

		status = FMV_EOF;
	}

	BIKShutdown();

#if defined( USE_FFMPEG )
	hasFrame = false;

	if( img_convert_ctx )
	{
		sws_freeContext( img_convert_ctx );
	}
	img_convert_ctx = nullptr;

	if( dec_ctx )
	{
		avcodec_close( dec_ctx );
	}
	//dec_ctx = nullptr;

	if( fmt_ctx )
	{
		avformat_close_input( &fmt_ctx );
	}
	//fmt_ctx = nullptr;

	status = FMV_EOF;
#endif
}

/*
==============
idCinematicBIK::AnimationLength
==============
*/
int idCinematicBIK::AnimationLength() const
{
	return animationLength;
}

/*
==============
idCinematicBIK::GetStartTime
==============
*/
int idCinematicBIK::GetStartTime() const
{
	return -1;	//SEA: todo!
}

/*
==============
idCinematicBIK::ExportToTGA
==============
*/
void idCinematicBIK::ExportToTGA( bool skipExisting )
{
	//SEA: todo!
}

/*
==============
idCinematicBIK::GetFrameRate
==============
*/
float idCinematicBIK::GetFrameRate() const
{
	return 30.0f;	//SEA: todo!  frameRate
}

/*
==============
idCinematicBIK::IsPlaying
==============
*/
bool idCinematicBIK::IsPlaying() const
{
	return( status == FMV_PLAY );
}

/*
==============
idCinematicBIK::ResetTime
==============
*/
void idCinematicBIK::ResetTime( int time )
{
	startTime = time; //originally this was: ( backEnd.viewDef ) ? 1000 * backEnd.viewDef->floatTime : -1;
	status = FMV_PLAY;
}

#if defined( USE_FFMPEG )
/*
==============
idCinematicBIK::ImageForTimeFFMPEG
==============
*/
cinData_t idCinematicBIK::ImageForTimeFFMPEG( int thisTime )
{
	cinData_t cinData = {};

	if( thisTime <= 0 )
	{
		thisTime = sys->Milliseconds();
	}

	if( r_skipDynamicTextures.GetBool() || status == FMV_EOF || status == FMV_IDLE )
	{
		return cinData;
	}

	if( !fmt_ctx )
	{
		// RB: .bik requested but not found
		return cinData;
	}

	if( ( !hasFrame ) || startTime == -1 )
	{
		if( startTime == -1 )
		{
			FFMPEGReset();
		}
		startTime = thisTime;
	}

	int desiredFrame = ( ( thisTime - startTime ) * frameRate ) / 1000;
	if( desiredFrame < 0 )
	{
		desiredFrame = 0;
	}

	if( desiredFrame < framePos )
	{
		FFMPEGReset();
	}

	if( hasFrame && desiredFrame == framePos )
	{
		cinData.imageWidth = CIN_WIDTH;
		cinData.imageHeight = CIN_HEIGHT;
		cinData.status = status;
		cinData.image = img;
		return cinData;
	}

	AVPacket packet;
	while( framePos < desiredFrame )
	{
		int frameFinished = 0;

		// Do a single frame by getting packets until we have a full frame
		while( !frameFinished )
		{
			// if we got to the end or failed
			if( av_read_frame( fmt_ctx, &packet ) < 0 )
			{
				// can't read any more, set to EOF
				status = FMV_EOF;
				if( looping )
				{
					desiredFrame = 0;
					FFMPEGReset();
					framePos = -1;
					startTime = thisTime;
					if( av_read_frame( fmt_ctx, &packet ) < 0 )
					{
						status = FMV_IDLE;
						return cinData;
					}
					status = FMV_PLAY;
				}
				else {
					status = FMV_IDLE;
					return cinData;
				}
			}
			// Is this a packet from the video stream?
			if( packet.stream_index == video_stream_index )
			{
				// Decode video frame
				avcodec_decode_video2( dec_ctx, frame, &frameFinished, &packet );
			}
			// Free the packet that was allocated by av_read_frame
			av_free_packet( &packet );
		}

		++framePos;
	}

	// We have reached the desired frame
	// Convert the image from its native format to RGB
	sws_scale( img_convert_ctx, frame->data, frame->linesize, 0, dec_ctx->height, frame2->data, frame2->linesize );

	cinData.imageWidth = CIN_WIDTH;
	cinData.imageHeight = CIN_HEIGHT;
	cinData.status = status;
	img->UploadScratch( imageData, CIN_WIDTH, CIN_HEIGHT );
	hasFrame = true;
	cinData.image = img;

	return cinData;
}
#endif
/*
==============
idCinematicBIK::ImageForTime
==============
*/
cinData_t idCinematicBIK::ImageForTime( int thisTime )
{
#if defined( USE_FFMPEG )
	// Carl: Handle BFG format BINK videos separately
	return ImageForTimeFFMPEG( thisTime );
#else
	// Carl: Handle original Doom 3 RoQ video files
	cinData_t cinData = {};
	return cinData;
#endif
}

