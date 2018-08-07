
#pragma hdrstop
#include "precompiled.h"

extern idCVar s_noSound;

#define JPEG_INTERNALS
//extern "C" {
	#include <jpeglib.h>
//}

#include "tr_local.h"

// Carl: ROQ files from original Doom 3
const int DEFAULT_CIN_WIDTH = 512;
const int DEFAULT_CIN_HEIGHT = 512;
const int MAXSIZE = 8;
const int MINSIZE = 4;

const int ROQ_FILE = 0x1084;
const int ROQ_QUAD = 0x1000;
const int ROQ_QUAD_INFO = 0x1001;
const int ROQ_CODEBOOK = 0x1002;
const int ROQ_QUAD_VQ = 0x1011;
const int ROQ_QUAD_JPEG = 0x1012;
const int ROQ_QUAD_HANG = 0x1013;
const int ROQ_PACKET = 0x1030;
const int ZA_SOUND_MONO = 0x1020;
const int ZA_SOUND_STEREO = 0x1021;

#define RoQ_CHUNK_PREAMBLE_SIZE 8
#define RoQ_AUDIO_SAMPLE_RATE 22050
#define RoQ_CHUNKS_TO_SCAN 30

// temporary buffers used by all cinematics
static int ROQ_YY_tab[ 256 ];
static int ROQ_VR_tab[ 256 ];
static int ROQ_UG_tab[ 256 ];
static int ROQ_VG_tab[ 256 ];
static int ROQ_UB_tab[ 256 ];
static byte* file = NULL;
static unsigned short* 	vq2 = NULL;
static unsigned short* 	vq4 = NULL;
static unsigned short* 	vq8 = NULL;

class idCinematicROQ : public idCinematic {
public:
	idCinematicROQ();
	virtual					~idCinematicROQ();

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
	idImage*				img;

	size_t					mcomp[ 256 ]; // RB: 64 bit fixes, changed long to int
	byte** 					qStatus[ 2 ];
	idStr					fileName;
	int						CIN_WIDTH, CIN_HEIGHT;
	idFile* 				iFile;
	cinStatus_t				status;
	int						tfps;
	int						RoQPlayed;
	int						ROQSize;
	unsigned int			RoQFrameSize;
	int						onQuad;
	int						numQuads;
	int						samplesPerLine;
	unsigned int			roq_id;
	int						screenDelta;
	byte* 					buf;
	int						samplesPerPixel;				// defaults to 2
	unsigned int			xsize, ysize, maxsize, minsize;
	int						normalBuffer0;
	int						roq_flags;
	int						roqF0;
	int						roqF1;
	int						t[ 2 ];
	int						roqFPS;
	int						drawX, drawY;

	int						animationLength;
	int						startTime;
	float					frameRate;

	byte * 					imageData;

	bool					looping;
	bool					dirty;
	bool					half;
	bool					smootheddouble;
	bool					inMemory;

	void					RoQInit();
	void					RoQShutdown();
	void					RoQInterrupt();

	void					blitVQQuad32fs( byte** status, unsigned char* data );

	void					move8_32( byte* src, byte* dst, int spl ) const;
	void					move4_32( byte* src, byte* dst, int spl ) const;
	void					blit8_32( byte* src, byte* dst, int spl ) const;
	void					blit4_32( byte* src, byte* dst, int spl ) const;
	void					blit2_32( byte* src, byte* dst, int spl ) const;

	unsigned short			yuv_to_rgb( int y, int u, int v ) const;
	unsigned int			yuv_to_rgb24( int y, int u, int v ) const;

	void					decodeCodeBook( byte* input, unsigned short roq_flags );
	void					recurseQuad( int startX, int startY, int quadSize, int xOff, int yOff );
	void					setupQuad( int xOff, int yOff );
	void					readQuadInfo( byte* qData );
	void					RoQPrepMcomp( int xoff, int yoff );
	void					RoQReset();
};

/*
========================================================
idCinematicLocal::InitCinematic
========================================================
*/
void InitCinematicROQ()
{
	// generate YUV tables
	float t_ub = ( 1.77200f / 2.0f ) * ( float ) ( 1 << 6 ) + 0.5f;
	float t_vr = ( 1.40200f / 2.0f ) * ( float ) ( 1 << 6 ) + 0.5f;
	float t_ug = ( 0.34414f / 2.0f ) * ( float ) ( 1 << 6 ) + 0.5f;
	float t_vg = ( 0.71414f / 2.0f ) * ( float ) ( 1 << 6 ) + 0.5f;
	for( int i = 0; i < 256; ++i )
	{
		float x = ( float ) ( 2 * i - 255 );

		ROQ_UB_tab[ i ] = ( int ) ( ( t_ub * x ) + ( 1 << 5 ) );
		ROQ_VR_tab[ i ] = ( int ) ( ( t_vr * x ) + ( 1 << 5 ) );
		ROQ_UG_tab[ i ] = ( int ) ( ( -t_ug * x ) );
		ROQ_VG_tab[ i ] = ( int ) ( ( -t_vg * x ) + ( 1 << 5 ) );
		ROQ_YY_tab[ i ] = ( int ) ( ( i << 6 ) | ( i >> 2 ) );
	}

	file = idMem::Alloc<byte, TAG_CINEMATIC>( 65536 );
	vq2 = idMem::Alloc<word, TAG_CINEMATIC>( 256 * 16 * 4 * sizeof( word ) );
	vq4 = idMem::Alloc<word, TAG_CINEMATIC>( 256 * 64 * 4 * sizeof( word ) );
	vq8 = idMem::Alloc<word, TAG_CINEMATIC>( 256 * 256 * 4 * sizeof( word ) );
}
/*
========================================================
idCinematicLocal::ShutdownCinematic
========================================================
*/
void ShutdownCinematicROQ()
{
	// Carl: Original Doom 3 RoQ files:
	idMem::Free( file ); file = NULL;
	idMem::Free( vq2 ); vq2 = NULL;
	idMem::Free( vq4 ); vq4 = NULL;
	idMem::Free( vq8 ); vq8 = NULL;
}
/*
========================================================
 InitROQFromFile
========================================================
*/
idCinematic * InitROQFromFile( const char * path, bool looping )
{
	auto cin = new( TAG_CINEMATIC ) idCinematicROQ;
	if( !cin->InitFromFile( path, looping ) ) {
		delete cin;
		return nullptr;
	}
	return cin;
}

/*
==============
idCinematicROQ::idCinematicLocal
==============
*/
idCinematicROQ::idCinematicROQ()
{
	qStatus[ 0 ] = idMem::Alloc<byte*, TAG_CINEMATIC>( 32768 * sizeof( byte* ) );
	qStatus[ 1 ] = idMem::Alloc<byte*, TAG_CINEMATIC>( 32768 * sizeof( byte* ) );

	imageData = nullptr;
	status = FMV_EOF;
	buf = nullptr;
	iFile = nullptr;

	img = renderImageManager->CreateStandaloneImage( "_cinematicROQ" );
	assert( img != nullptr );
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
idCinematicLocal::~idCinematicLocal
==============
*/
idCinematicROQ::~idCinematicROQ()
{
	Close();

	// Carl: Original Doom 3 RoQ files:
	idMem::Free( qStatus[ 0 ] ); qStatus[ 0 ] = nullptr;
	idMem::Free( qStatus[ 1 ] ); qStatus[ 1 ] = nullptr;

	renderImageManager->DestroyStandaloneImage( img );
	img = nullptr;
}

/*
==============
idCinematicROQ::InitFromFile
==============
*/
bool idCinematicROQ::InitFromFile( const char* qpath, bool amilooping )
{
	Close();

	inMemory = 0;
	animationLength = 100000;

	fileName = qpath;
	iFile = fileSystem->OpenFileRead( fileName );
	if( !iFile )
	{
		animationLength = 0;
		return false;
	}
	// Carl: The rest of this function is for original Doom 3 RoQ files:
	common->DPrintf( "RoQ file found: '%s'\n", fileName.c_str() );

	ROQSize = iFile->Length();

	looping = amilooping;

	CIN_HEIGHT = DEFAULT_CIN_HEIGHT;
	CIN_WIDTH = DEFAULT_CIN_WIDTH;
	samplesPerPixel = 4;
	startTime = 0;	//sys->Milliseconds();
	buf = NULL;

	iFile->Read( file, 16 );

	unsigned short RoQID = ( unsigned short ) ( file[ 0 ] ) + ( unsigned short ) ( file[ 1 ] ) * 256;

	frameRate = file[ 6 ];
	if( frameRate == 32.0f ) {
		frameRate = 1000.0f / 32.0f;
	}

	if( RoQID == ROQ_FILE )
	{
		RoQInit();
		status = FMV_PLAY;
		ImageForTime( 0 );
		status = ( looping ) ? FMV_PLAY : FMV_IDLE;
		return true;
	}

	RoQShutdown();
	return false;
}

/*
==============
idCinematicROQ::Close
==============
*/
void idCinematicROQ::Close()
{
	if( imageData )
	{
		idMem::Free( ( void* ) imageData );
		imageData = nullptr;

		buf = nullptr;

		status = FMV_EOF;
	}

	RoQShutdown();
}

/*
==============
idCinematicROQ::AnimationLength
==============
*/
int idCinematicROQ::AnimationLength() const
{
	return animationLength;
}

/*
==============
idCinematicROQ::GetStartTime
==============
*/
int idCinematicROQ::GetStartTime() const
{
	return -1;	//SEA: todo!
}

/*
==============
idCinematicROQ::ExportToTGA
==============
*/
void idCinematicROQ::ExportToTGA( bool skipExisting )
{
	//SEA: todo!
}

/*
==============
idCinematicROQ::GetFrameRate
==============
*/
float idCinematicROQ::GetFrameRate() const
{
	return 30.0f;	//SEA: todo!  frameRate
}

/*
==============
idCinematicROQ::IsPlaying
==============
*/
bool idCinematicROQ::IsPlaying() const
{
	return( status == FMV_PLAY );
}

/*
==============
idCinematicROQ::ResetTime
==============
*/
void idCinematicROQ::ResetTime( int time )
{
	startTime = time; //originally this was: ( backEnd.viewDef ) ? 1000 * backEnd.viewDef->floatTime : -1;
	status = FMV_PLAY;
}

/*
==============
idCinematicROQ::ImageForTime
==============
*/
cinData_t idCinematicROQ::ImageForTime( int thisTime )
{
	cinData_t cinData = {};

	if( thisTime == 0 )
	{
		thisTime = sys->Milliseconds();
	}

	if( thisTime < 0 )
	{
		thisTime = 0;
	}

	//	if ( r_skipROQ.GetBool() ) {
	if( r_skipDynamicTextures.GetBool() )
	{
		return cinData;
	}

	if( !iFile )
	{
		// RB: neither .bik or .roq found
		return cinData;
	}

	if( status == FMV_EOF || status == FMV_IDLE )
	{
		return cinData;
	}

	if( buf == NULL || startTime == -1 )
	{
		if( startTime == -1 )
		{
			RoQReset();
		}
		startTime = thisTime;
	}

	tfps = ( ( thisTime - startTime ) * frameRate ) / 1000;

	if( tfps < 0 )
	{
		tfps = 0;
	}

	if( tfps < numQuads )
	{
		RoQReset();
		buf = NULL;
		status = FMV_PLAY;
	}

	if( buf == NULL )
	{
		while( buf == NULL )
		{
			RoQInterrupt();
		}
	}
	else
	{
		while( ( tfps != numQuads && status == FMV_PLAY ) )
		{
			RoQInterrupt();
		}
	}

	if( status == FMV_LOOPED )
	{
		status = FMV_PLAY;
		while( buf == NULL && status == FMV_PLAY )
		{
			RoQInterrupt();
		}
		startTime = thisTime;
	}

	if( status == FMV_EOF )
	{
		if( looping )
		{
			RoQReset();
			buf = NULL;
			if( status == FMV_LOOPED )
			{
				status = FMV_PLAY;
			}
			while( buf == NULL && status == FMV_PLAY )
			{
				RoQInterrupt();
			}
			startTime = thisTime;
		}
		else
		{
			status = FMV_IDLE;
			RoQShutdown();
		}
	}

	cinData.imageWidth = CIN_WIDTH;
	cinData.imageHeight = CIN_HEIGHT;
	cinData.status = status;
	img->UploadScratch( imageData, CIN_WIDTH, CIN_HEIGHT );
	cinData.image = img;

	return cinData;
}

/*
==============
idCinematicROQ::move8_32
==============
*/
void idCinematicROQ::move8_32( byte* src, byte* dst, int spl ) const
{
	int* dsrc = ( int* ) src;
	int* ddst = ( int* ) dst;
	int dspl = spl >> 2;

#if USE_INTRINSICS

	__m128i	xmm[ 16 ];

	xmm[ 0 ] = _mm_loadu_si128( ( const __m128i * )( dsrc + dspl * 0 + 0 ) );
	xmm[ 1 ] = _mm_loadu_si128( ( const __m128i * )( dsrc + dspl * 0 + 4 ) );
	xmm[ 2 ] = _mm_loadu_si128( ( const __m128i * )( dsrc + dspl * 1 + 0 ) );
	xmm[ 3 ] = _mm_loadu_si128( ( const __m128i * )( dsrc + dspl * 1 + 4 ) );
	xmm[ 4 ] = _mm_loadu_si128( ( const __m128i * )( dsrc + dspl * 2 + 0 ) );
	xmm[ 5 ] = _mm_loadu_si128( ( const __m128i * )( dsrc + dspl * 2 + 4 ) );
	xmm[ 6 ] = _mm_loadu_si128( ( const __m128i * )( dsrc + dspl * 3 + 0 ) );
	xmm[ 7 ] = _mm_loadu_si128( ( const __m128i * )( dsrc + dspl * 3 + 4 ) );
	xmm[ 8 ] = _mm_loadu_si128( ( const __m128i * )( dsrc + dspl * 4 + 0 ) );
	xmm[ 9 ] = _mm_loadu_si128( ( const __m128i * )( dsrc + dspl * 4 + 4 ) );
	xmm[ 10 ] = _mm_loadu_si128( ( const __m128i * )( dsrc + dspl * 5 + 0 ) );
	xmm[ 11 ] = _mm_loadu_si128( ( const __m128i * )( dsrc + dspl * 5 + 4 ) );
	xmm[ 12 ] = _mm_loadu_si128( ( const __m128i * )( dsrc + dspl * 6 + 0 ) );
	xmm[ 13 ] = _mm_loadu_si128( ( const __m128i * )( dsrc + dspl * 6 + 4 ) );
	xmm[ 14 ] = _mm_loadu_si128( ( const __m128i * )( dsrc + dspl * 7 + 0 ) );
	xmm[ 15 ] = _mm_loadu_si128( ( const __m128i * )( dsrc + dspl * 7 + 4 ) );

	_mm_storeu_si128( ( __m128i * )( ddst + dspl * 0 + 0 ), xmm[ 0 ] );
	_mm_storeu_si128( ( __m128i * )( ddst + dspl * 0 + 4 ), xmm[ 1 ] );
	_mm_storeu_si128( ( __m128i * )( ddst + dspl * 1 + 0 ), xmm[ 2 ] );
	_mm_storeu_si128( ( __m128i * )( ddst + dspl * 1 + 4 ), xmm[ 3 ] );
	_mm_storeu_si128( ( __m128i * )( ddst + dspl * 2 + 0 ), xmm[ 4 ] );
	_mm_storeu_si128( ( __m128i * )( ddst + dspl * 2 + 4 ), xmm[ 5 ] );
	_mm_storeu_si128( ( __m128i * )( ddst + dspl * 3 + 0 ), xmm[ 6 ] );
	_mm_storeu_si128( ( __m128i * )( ddst + dspl * 3 + 4 ), xmm[ 7 ] );
	_mm_storeu_si128( ( __m128i * )( ddst + dspl * 4 + 0 ), xmm[ 8 ] );
	_mm_storeu_si128( ( __m128i * )( ddst + dspl * 4 + 4 ), xmm[ 9 ] );
	_mm_storeu_si128( ( __m128i * )( ddst + dspl * 5 + 0 ), xmm[ 10 ] );
	_mm_storeu_si128( ( __m128i * )( ddst + dspl * 5 + 4 ), xmm[ 11 ] );
	_mm_storeu_si128( ( __m128i * )( ddst + dspl * 6 + 0 ), xmm[ 12 ] );
	_mm_storeu_si128( ( __m128i * )( ddst + dspl * 6 + 4 ), xmm[ 13 ] );
	_mm_storeu_si128( ( __m128i * )( ddst + dspl * 7 + 0 ), xmm[ 14 ] );
	_mm_storeu_si128( ( __m128i * )( ddst + dspl * 7 + 4 ), xmm[ 15 ] );

#else

	ddst[ 0 * dspl + 0 ] = dsrc[ 0 * dspl + 0 ];
	ddst[ 0 * dspl + 1 ] = dsrc[ 0 * dspl + 1 ];
	ddst[ 0 * dspl + 2 ] = dsrc[ 0 * dspl + 2 ];
	ddst[ 0 * dspl + 3 ] = dsrc[ 0 * dspl + 3 ];
	ddst[ 0 * dspl + 4 ] = dsrc[ 0 * dspl + 4 ];
	ddst[ 0 * dspl + 5 ] = dsrc[ 0 * dspl + 5 ];
	ddst[ 0 * dspl + 6 ] = dsrc[ 0 * dspl + 6 ];
	ddst[ 0 * dspl + 7 ] = dsrc[ 0 * dspl + 7 ];

	ddst[ 1 * dspl + 0 ] = dsrc[ 1 * dspl + 0 ];
	ddst[ 1 * dspl + 1 ] = dsrc[ 1 * dspl + 1 ];
	ddst[ 1 * dspl + 2 ] = dsrc[ 1 * dspl + 2 ];
	ddst[ 1 * dspl + 3 ] = dsrc[ 1 * dspl + 3 ];
	ddst[ 1 * dspl + 4 ] = dsrc[ 1 * dspl + 4 ];
	ddst[ 1 * dspl + 5 ] = dsrc[ 1 * dspl + 5 ];
	ddst[ 1 * dspl + 6 ] = dsrc[ 1 * dspl + 6 ];
	ddst[ 1 * dspl + 7 ] = dsrc[ 1 * dspl + 7 ];

	ddst[ 2 * dspl + 0 ] = dsrc[ 2 * dspl + 0 ];
	ddst[ 2 * dspl + 1 ] = dsrc[ 2 * dspl + 1 ];
	ddst[ 2 * dspl + 2 ] = dsrc[ 2 * dspl + 2 ];
	ddst[ 2 * dspl + 3 ] = dsrc[ 2 * dspl + 3 ];
	ddst[ 2 * dspl + 4 ] = dsrc[ 2 * dspl + 4 ];
	ddst[ 2 * dspl + 5 ] = dsrc[ 2 * dspl + 5 ];
	ddst[ 2 * dspl + 6 ] = dsrc[ 2 * dspl + 6 ];
	ddst[ 2 * dspl + 7 ] = dsrc[ 2 * dspl + 7 ];

	ddst[ 3 * dspl + 0 ] = dsrc[ 3 * dspl + 0 ];
	ddst[ 3 * dspl + 1 ] = dsrc[ 3 * dspl + 1 ];
	ddst[ 3 * dspl + 2 ] = dsrc[ 3 * dspl + 2 ];
	ddst[ 3 * dspl + 3 ] = dsrc[ 3 * dspl + 3 ];
	ddst[ 3 * dspl + 4 ] = dsrc[ 3 * dspl + 4 ];
	ddst[ 3 * dspl + 5 ] = dsrc[ 3 * dspl + 5 ];
	ddst[ 3 * dspl + 6 ] = dsrc[ 3 * dspl + 6 ];
	ddst[ 3 * dspl + 7 ] = dsrc[ 3 * dspl + 7 ];

	ddst[ 4 * dspl + 0 ] = dsrc[ 4 * dspl + 0 ];
	ddst[ 4 * dspl + 1 ] = dsrc[ 4 * dspl + 1 ];
	ddst[ 4 * dspl + 2 ] = dsrc[ 4 * dspl + 2 ];
	ddst[ 4 * dspl + 3 ] = dsrc[ 4 * dspl + 3 ];
	ddst[ 4 * dspl + 4 ] = dsrc[ 4 * dspl + 4 ];
	ddst[ 4 * dspl + 5 ] = dsrc[ 4 * dspl + 5 ];
	ddst[ 4 * dspl + 6 ] = dsrc[ 4 * dspl + 6 ];
	ddst[ 4 * dspl + 7 ] = dsrc[ 4 * dspl + 7 ];

	ddst[ 5 * dspl + 0 ] = dsrc[ 5 * dspl + 0 ];
	ddst[ 5 * dspl + 1 ] = dsrc[ 5 * dspl + 1 ];
	ddst[ 5 * dspl + 2 ] = dsrc[ 5 * dspl + 2 ];
	ddst[ 5 * dspl + 3 ] = dsrc[ 5 * dspl + 3 ];
	ddst[ 5 * dspl + 4 ] = dsrc[ 5 * dspl + 4 ];
	ddst[ 5 * dspl + 5 ] = dsrc[ 5 * dspl + 5 ];
	ddst[ 5 * dspl + 6 ] = dsrc[ 5 * dspl + 6 ];
	ddst[ 5 * dspl + 7 ] = dsrc[ 5 * dspl + 7 ];

	ddst[ 6 * dspl + 0 ] = dsrc[ 6 * dspl + 0 ];
	ddst[ 6 * dspl + 1 ] = dsrc[ 6 * dspl + 1 ];
	ddst[ 6 * dspl + 2 ] = dsrc[ 6 * dspl + 2 ];
	ddst[ 6 * dspl + 3 ] = dsrc[ 6 * dspl + 3 ];
	ddst[ 6 * dspl + 4 ] = dsrc[ 6 * dspl + 4 ];
	ddst[ 6 * dspl + 5 ] = dsrc[ 6 * dspl + 5 ];
	ddst[ 6 * dspl + 6 ] = dsrc[ 6 * dspl + 6 ];
	ddst[ 6 * dspl + 7 ] = dsrc[ 6 * dspl + 7 ];

	ddst[ 7 * dspl + 0 ] = dsrc[ 7 * dspl + 0 ];
	ddst[ 7 * dspl + 1 ] = dsrc[ 7 * dspl + 1 ];
	ddst[ 7 * dspl + 2 ] = dsrc[ 7 * dspl + 2 ];
	ddst[ 7 * dspl + 3 ] = dsrc[ 7 * dspl + 3 ];
	ddst[ 7 * dspl + 4 ] = dsrc[ 7 * dspl + 4 ];
	ddst[ 7 * dspl + 5 ] = dsrc[ 7 * dspl + 5 ];
	ddst[ 7 * dspl + 6 ] = dsrc[ 7 * dspl + 6 ];
	ddst[ 7 * dspl + 7 ] = dsrc[ 7 * dspl + 7 ];

#endif
}

/*
==============
idCinematicROQ::move4_32
==============
*/
void idCinematicROQ::move4_32( byte* src, byte* dst, int spl ) const
{
	int* dsrc = ( int* ) src;
	int* ddst = ( int* ) dst;
	int dspl = spl >> 2;

#if USE_INTRINSICS

	__m128i xmm0 = _mm_loadu_si128( ( const __m128i * )( dsrc + dspl * 0 ) );
	__m128i xmm1 = _mm_loadu_si128( ( const __m128i * )( dsrc + dspl * 1 ) );
	__m128i xmm2 = _mm_loadu_si128( ( const __m128i * )( dsrc + dspl * 2 ) );
	__m128i xmm3 = _mm_loadu_si128( ( const __m128i * )( dsrc + dspl * 3 ) );

	_mm_storeu_si128( ( __m128i * )( ddst + dspl * 0 ), xmm0 );
	_mm_storeu_si128( ( __m128i * )( ddst + dspl * 1 ), xmm1 );
	_mm_storeu_si128( ( __m128i * )( ddst + dspl * 2 ), xmm2 );
	_mm_storeu_si128( ( __m128i * )( ddst + dspl * 3 ), xmm3 );

#else

	ddst[ 0 * dspl + 0 ] = dsrc[ 0 * dspl + 0 ];
	ddst[ 0 * dspl + 1 ] = dsrc[ 0 * dspl + 1 ];
	ddst[ 0 * dspl + 2 ] = dsrc[ 0 * dspl + 2 ];
	ddst[ 0 * dspl + 3 ] = dsrc[ 0 * dspl + 3 ];

	ddst[ 1 * dspl + 0 ] = dsrc[ 1 * dspl + 0 ];
	ddst[ 1 * dspl + 1 ] = dsrc[ 1 * dspl + 1 ];
	ddst[ 1 * dspl + 2 ] = dsrc[ 1 * dspl + 2 ];
	ddst[ 1 * dspl + 3 ] = dsrc[ 1 * dspl + 3 ];

	ddst[ 2 * dspl + 0 ] = dsrc[ 2 * dspl + 0 ];
	ddst[ 2 * dspl + 1 ] = dsrc[ 2 * dspl + 1 ];
	ddst[ 2 * dspl + 2 ] = dsrc[ 2 * dspl + 2 ];
	ddst[ 2 * dspl + 3 ] = dsrc[ 2 * dspl + 3 ];

	ddst[ 3 * dspl + 0 ] = dsrc[ 3 * dspl + 0 ];
	ddst[ 3 * dspl + 1 ] = dsrc[ 3 * dspl + 1 ];
	ddst[ 3 * dspl + 2 ] = dsrc[ 3 * dspl + 2 ];
	ddst[ 3 * dspl + 3 ] = dsrc[ 3 * dspl + 3 ];

#endif
}

/*
==============
idCinematicROQ::blit8_32
==============
*/
void idCinematicROQ::blit8_32( byte* src, byte* dst, int spl ) const
{
	int *dsrc = ( int* ) src;
	int *ddst = ( int* ) dst;
	int dspl = spl >> 2;

#if USE_INTRINSICS
#if 1
	__m128i	xmm[ 16 ];

	xmm[ 0 ] = _mm_load_si128( ( const __m128i * )( dsrc + 0 ) );
	xmm[ 1 ] = _mm_load_si128( ( const __m128i * )( dsrc + 4 ) );
	xmm[ 2 ] = _mm_load_si128( ( const __m128i * )( dsrc + 8 ) );
	xmm[ 3 ] = _mm_load_si128( ( const __m128i * )( dsrc + 12 ) );
	xmm[ 4 ] = _mm_load_si128( ( const __m128i * )( dsrc + 16 ) );
	xmm[ 5 ] = _mm_load_si128( ( const __m128i * )( dsrc + 20 ) );
	xmm[ 6 ] = _mm_load_si128( ( const __m128i * )( dsrc + 24 ) );
	xmm[ 7 ] = _mm_load_si128( ( const __m128i * )( dsrc + 28 ) );
	xmm[ 8 ] = _mm_load_si128( ( const __m128i * )( dsrc + 32 ) );
	xmm[ 9 ] = _mm_load_si128( ( const __m128i * )( dsrc + 36 ) );
	xmm[ 10 ] = _mm_load_si128( ( const __m128i * )( dsrc + 40 ) );
	xmm[ 11 ] = _mm_load_si128( ( const __m128i * )( dsrc + 44 ) );
	xmm[ 12 ] = _mm_load_si128( ( const __m128i * )( dsrc + 48 ) );
	xmm[ 13 ] = _mm_load_si128( ( const __m128i * )( dsrc + 52 ) );
	xmm[ 14 ] = _mm_load_si128( ( const __m128i * )( dsrc + 56 ) );
	xmm[ 15 ] = _mm_load_si128( ( const __m128i * )( dsrc + 60 ) );

	_mm_store_si128( ( __m128i * )( ddst + dspl * 0 + 0 ), xmm[ 0 ] );
	_mm_store_si128( ( __m128i * )( ddst + dspl * 0 + 4 ), xmm[ 1 ] );
	_mm_store_si128( ( __m128i * )( ddst + dspl * 1 + 0 ), xmm[ 2 ] );
	_mm_store_si128( ( __m128i * )( ddst + dspl * 1 + 4 ), xmm[ 3 ] );
	_mm_store_si128( ( __m128i * )( ddst + dspl * 2 + 0 ), xmm[ 4 ] );
	_mm_store_si128( ( __m128i * )( ddst + dspl * 2 + 4 ), xmm[ 5 ] );
	_mm_store_si128( ( __m128i * )( ddst + dspl * 3 + 0 ), xmm[ 6 ] );
	_mm_store_si128( ( __m128i * )( ddst + dspl * 3 + 4 ), xmm[ 7 ] );
	_mm_store_si128( ( __m128i * )( ddst + dspl * 4 + 0 ), xmm[ 8 ] );
	_mm_store_si128( ( __m128i * )( ddst + dspl * 4 + 4 ), xmm[ 9 ] );
	_mm_store_si128( ( __m128i * )( ddst + dspl * 5 + 0 ), xmm[ 10 ] );
	_mm_store_si128( ( __m128i * )( ddst + dspl * 5 + 4 ), xmm[ 11 ] );
	_mm_store_si128( ( __m128i * )( ddst + dspl * 6 + 0 ), xmm[ 12 ] );
	_mm_store_si128( ( __m128i * )( ddst + dspl * 6 + 4 ), xmm[ 13 ] );
	_mm_store_si128( ( __m128i * )( ddst + dspl * 7 + 0 ), xmm[ 14 ] );
	_mm_store_si128( ( __m128i * )( ddst + dspl * 7 + 4 ), xmm[ 15 ] );
#else
	__m256i	xmm[ 8 ];

	xmm[ 0 ] = _mm256_load_si256( ( const __m256i * )( dsrc + 0 ) );
	xmm[ 1 ] = _mm256_load_si256( ( const __m256i * )( dsrc + 8 ) );
	xmm[ 2 ] = _mm256_load_si256( ( const __m256i * )( dsrc + 16 ) );
	xmm[ 3 ] = _mm256_load_si256( ( const __m256i * )( dsrc + 24 ) );
	xmm[ 4 ] = _mm256_load_si256( ( const __m256i * )( dsrc + 32 ) );
	xmm[ 5 ] = _mm256_load_si256( ( const __m256i * )( dsrc + 40 ) );
	xmm[ 6 ] = _mm256_load_si256( ( const __m256i * )( dsrc + 48 ) );
	xmm[ 7 ] = _mm256_load_si256( ( const __m256i * )( dsrc + 56 ) );

	_mm256_store_si256( ( __m256i * )( ddst + dspl * 0 + 0 ), xmm[ 0 ] );
	_mm256_store_si256( ( __m256i * )( ddst + dspl * 1 + 0 ), xmm[ 1 ] );
	_mm256_store_si256( ( __m256i * )( ddst + dspl * 2 + 0 ), xmm[ 2 ] );
	_mm256_store_si256( ( __m256i * )( ddst + dspl * 3 + 0 ), xmm[ 3 ] );
	_mm256_store_si256( ( __m256i * )( ddst + dspl * 4 + 0 ), xmm[ 4 ] );
	_mm256_store_si256( ( __m256i * )( ddst + dspl * 5 + 0 ), xmm[ 5 ] );
	_mm256_store_si256( ( __m256i * )( ddst + dspl * 6 + 0 ), xmm[ 6 ] );
	_mm256_store_si256( ( __m256i * )( ddst + dspl * 7 + 0 ), xmm[ 7 ] );
#endif
#else

	ddst[ 0 * dspl + 0 ] = dsrc[ 0 ];
	ddst[ 0 * dspl + 1 ] = dsrc[ 1 ];
	ddst[ 0 * dspl + 2 ] = dsrc[ 2 ];
	ddst[ 0 * dspl + 3 ] = dsrc[ 3 ];
	ddst[ 0 * dspl + 4 ] = dsrc[ 4 ];
	ddst[ 0 * dspl + 5 ] = dsrc[ 5 ];
	ddst[ 0 * dspl + 6 ] = dsrc[ 6 ];
	ddst[ 0 * dspl + 7 ] = dsrc[ 7 ];

	ddst[ 1 * dspl + 0 ] = dsrc[ 8 ];
	ddst[ 1 * dspl + 1 ] = dsrc[ 9 ];
	ddst[ 1 * dspl + 2 ] = dsrc[ 10 ];
	ddst[ 1 * dspl + 3 ] = dsrc[ 11 ];
	ddst[ 1 * dspl + 4 ] = dsrc[ 12 ];
	ddst[ 1 * dspl + 5 ] = dsrc[ 13 ];
	ddst[ 1 * dspl + 6 ] = dsrc[ 14 ];
	ddst[ 1 * dspl + 7 ] = dsrc[ 15 ];

	ddst[ 2 * dspl + 0 ] = dsrc[ 16 ];
	ddst[ 2 * dspl + 1 ] = dsrc[ 17 ];
	ddst[ 2 * dspl + 2 ] = dsrc[ 18 ];
	ddst[ 2 * dspl + 3 ] = dsrc[ 19 ];
	ddst[ 2 * dspl + 4 ] = dsrc[ 20 ];
	ddst[ 2 * dspl + 5 ] = dsrc[ 21 ];
	ddst[ 2 * dspl + 6 ] = dsrc[ 22 ];
	ddst[ 2 * dspl + 7 ] = dsrc[ 23 ];

	ddst[ 3 * dspl + 0 ] = dsrc[ 24 ];
	ddst[ 3 * dspl + 1 ] = dsrc[ 25 ];
	ddst[ 3 * dspl + 2 ] = dsrc[ 26 ];
	ddst[ 3 * dspl + 3 ] = dsrc[ 27 ];
	ddst[ 3 * dspl + 4 ] = dsrc[ 28 ];
	ddst[ 3 * dspl + 5 ] = dsrc[ 29 ];
	ddst[ 3 * dspl + 6 ] = dsrc[ 30 ];
	ddst[ 3 * dspl + 7 ] = dsrc[ 31 ];

	ddst[ 4 * dspl + 0 ] = dsrc[ 32 ];
	ddst[ 4 * dspl + 1 ] = dsrc[ 33 ];
	ddst[ 4 * dspl + 2 ] = dsrc[ 34 ];
	ddst[ 4 * dspl + 3 ] = dsrc[ 35 ];
	ddst[ 4 * dspl + 4 ] = dsrc[ 36 ];
	ddst[ 4 * dspl + 5 ] = dsrc[ 37 ];
	ddst[ 4 * dspl + 6 ] = dsrc[ 38 ];
	ddst[ 4 * dspl + 7 ] = dsrc[ 39 ];

	ddst[ 5 * dspl + 0 ] = dsrc[ 40 ];
	ddst[ 5 * dspl + 1 ] = dsrc[ 41 ];
	ddst[ 5 * dspl + 2 ] = dsrc[ 42 ];
	ddst[ 5 * dspl + 3 ] = dsrc[ 43 ];
	ddst[ 5 * dspl + 4 ] = dsrc[ 44 ];
	ddst[ 5 * dspl + 5 ] = dsrc[ 45 ];
	ddst[ 5 * dspl + 6 ] = dsrc[ 46 ];
	ddst[ 5 * dspl + 7 ] = dsrc[ 47 ];

	ddst[ 6 * dspl + 0 ] = dsrc[ 48 ];
	ddst[ 6 * dspl + 1 ] = dsrc[ 49 ];
	ddst[ 6 * dspl + 2 ] = dsrc[ 50 ];
	ddst[ 6 * dspl + 3 ] = dsrc[ 51 ];
	ddst[ 6 * dspl + 4 ] = dsrc[ 52 ];
	ddst[ 6 * dspl + 5 ] = dsrc[ 53 ];
	ddst[ 6 * dspl + 6 ] = dsrc[ 54 ];
	ddst[ 6 * dspl + 7 ] = dsrc[ 55 ];

	ddst[ 7 * dspl + 0 ] = dsrc[ 56 ];
	ddst[ 7 * dspl + 1 ] = dsrc[ 57 ];
	ddst[ 7 * dspl + 2 ] = dsrc[ 58 ];
	ddst[ 7 * dspl + 3 ] = dsrc[ 59 ];
	ddst[ 7 * dspl + 4 ] = dsrc[ 60 ];
	ddst[ 7 * dspl + 5 ] = dsrc[ 61 ];
	ddst[ 7 * dspl + 6 ] = dsrc[ 62 ];
	ddst[ 7 * dspl + 7 ] = dsrc[ 63 ];

#endif
}

/*
==============
idCinematicROQ::blit4_32
==============
*/
void idCinematicROQ::blit4_32( byte* src, byte* dst, int spl ) const
{
	int* dsrc = ( int* ) src;
	int* ddst = ( int* ) dst;
	int dspl = spl >> 2;

#if USE_INTRINSICS
#if 1
	__m128i xmm0 = _mm_load_si128( ( const __m128i * )( dsrc + 0 ) );
	__m128i xmm1 = _mm_load_si128( ( const __m128i * )( dsrc + 4 ) );
	__m128i xmm2 = _mm_load_si128( ( const __m128i * )( dsrc + 8 ) );
	__m128i xmm3 = _mm_load_si128( ( const __m128i * )( dsrc + 12 ) );

	_mm_store_si128( ( __m128i * )( ddst + dspl * 0 ), xmm0 );
	_mm_store_si128( ( __m128i * )( ddst + dspl * 1 ), xmm1 );
	_mm_store_si128( ( __m128i * )( ddst + dspl * 2 ), xmm2 );
	_mm_store_si128( ( __m128i * )( ddst + dspl * 3 ), xmm3 );
#else
	__m256i xmm0 = _mm256_load_si256( ( const __m256i * )( dsrc + 0 ) );
	__m256i xmm2 = _mm256_load_si256( ( const __m256i * )( dsrc + 8 ) );

	_mm256_store_si256( ( __m256i * )( ddst + dspl * 0 ), xmm0 );
	_mm256_store_si256( ( __m256i * )( ddst + dspl * 3 ), xmm2 );
#endif
#else

	ddst[ 0 * dspl + 0 ] = dsrc[ 0 ];
	ddst[ 0 * dspl + 1 ] = dsrc[ 1 ];
	ddst[ 0 * dspl + 2 ] = dsrc[ 2 ];
	ddst[ 0 * dspl + 3 ] = dsrc[ 3 ];

	ddst[ 1 * dspl + 0 ] = dsrc[ 4 ];
	ddst[ 1 * dspl + 1 ] = dsrc[ 5 ];
	ddst[ 1 * dspl + 2 ] = dsrc[ 6 ];
	ddst[ 1 * dspl + 3 ] = dsrc[ 7 ];

	ddst[ 2 * dspl + 0 ] = dsrc[ 8 ];
	ddst[ 2 * dspl + 1 ] = dsrc[ 9 ];
	ddst[ 2 * dspl + 2 ] = dsrc[ 10 ];
	ddst[ 2 * dspl + 3 ] = dsrc[ 11 ];

	ddst[ 3 * dspl + 0 ] = dsrc[ 12 ];
	ddst[ 3 * dspl + 1 ] = dsrc[ 13 ];
	ddst[ 3 * dspl + 2 ] = dsrc[ 14 ];
	ddst[ 3 * dspl + 3 ] = dsrc[ 15 ];

#endif
}

/*
==============
idCinematicROQ::blit2_32
==============
*/
void idCinematicROQ::blit2_32( byte* src, byte* dst, int spl ) const
{
	int* dsrc = ( int* ) src;
	int* ddst = ( int* ) dst;
	int dspl = spl >> 2;

#if USE_INTRINSICS

	__m128i xmm0 = _mm_loadl_epi64( ( const __m128i * )( dsrc + 0 ) );
	__m128i xmm1 = _mm_loadl_epi64( ( const __m128i * )( dsrc + 2 ) );

	_mm_storel_epi64( ( __m128i * )( ddst + dspl * 0 ), xmm0 );
	_mm_storel_epi64( ( __m128i * )( ddst + dspl * 1 ), xmm1 );

#else

	ddst[ 0 * dspl + 0 ] = dsrc[ 0 ];
	ddst[ 0 * dspl + 1 ] = dsrc[ 1 ];
	ddst[ 1 * dspl + 0 ] = dsrc[ 2 ];
	ddst[ 1 * dspl + 1 ] = dsrc[ 3 ];

#endif
}

/*
==============
idCinematicROQ::blitVQQuad32fs
==============
*/
void idCinematicROQ::blitVQQuad32fs( byte** status, unsigned char* data )
{
	unsigned short	code;
	unsigned int	i;

	unsigned short newd = 0;
	unsigned short celdata = 0;
	unsigned int index = 0;

	do {
		if( !newd )
		{
			newd = 7;
			celdata = data[ 0 ] + data[ 1 ] * 256;
			data += 2;
		}
		else
		{
			newd--;
		}

		code = ( unsigned short ) ( celdata & 0xc000 );
		celdata <<= 2;

		switch( code )
		{
			case 0x8000: // vq code
				blit8_32( ( byte* ) &vq8[ ( *data ) * 128 ], status[ index ], samplesPerLine );
				data++;
				index += 5;
				break;

			case 0xc000: // drop
				index++; // skip 8x8
				for( i = 0; i < 4; i++ )
				{
					if( !newd )
					{
						newd = 7;
						celdata = data[ 0 ] + data[ 1 ] * 256;
						data += 2;
					}
					else
					{
						newd--;
					}

					code = ( unsigned short ) ( celdata & 0xc000 );
					celdata <<= 2;

					switch( code ) // code in top two bits of code
					{
						case 0x8000: // 4x4 vq code
							blit4_32( ( byte* ) &vq4[ ( *data ) * 32 ], status[ index ], samplesPerLine );
							data++;
							break;

						case 0xc000: // 2x2 vq code
							blit2_32( ( byte* ) &vq2[ ( *data ) * 8 ], status[ index ], samplesPerLine );
							data++;
							blit2_32( ( byte* ) &vq2[ ( *data ) * 8 ], status[ index ] + 8, samplesPerLine );
							data++;
							blit2_32( ( byte* ) &vq2[ ( *data ) * 8 ], status[ index ] + samplesPerLine * 2, samplesPerLine );
							data++;
							blit2_32( ( byte* ) &vq2[ ( *data ) * 8 ], status[ index ] + samplesPerLine * 2 + 8, samplesPerLine );
							data++;
							break;

						case 0x4000: // motion compensation
							move4_32( status[ index ] + mcomp[ ( *data ) ], status[ index ], samplesPerLine );
							data++;
							break;
					}
					index++;
				}
				break;

			case 0x4000: // motion compensation
				move8_32( status[ index ] + mcomp[ ( *data ) ], status[ index ], samplesPerLine );
				data++;
				index += 5;
				break;

			case 0x0000:
				index += 5;
				break;
		}

	} while( status[ index ] != NULL );
}

#define VQ2TO4(a,b,c,d) { \
	*c++ = a[0];	\
	*d++ = a[0];	\
	*d++ = a[0];	\
	*c++ = a[1];	\
	*d++ = a[1];	\
	*d++ = a[1];	\
	*c++ = b[0];	\
	*d++ = b[0];	\
	*d++ = b[0];	\
	*c++ = b[1];	\
	*d++ = b[1];	\
	*d++ = b[1];	\
	*d++ = a[0];	\
	*d++ = a[0];	\
	*d++ = a[1];	\
	*d++ = a[1];	\
	*d++ = b[0];	\
	*d++ = b[0];	\
	*d++ = b[1];	\
	*d++ = b[1];	\
	a += 2; b += 2; }

#define VQ2TO2(a,b,c,d) { \
	*c++ = *a;	\
	*d++ = *a;	\
	*d++ = *a;	\
	*c++ = *b;	\
	*d++ = *b;	\
	*d++ = *b;	\
	*d++ = *a;	\
	*d++ = *a;	\
	*d++ = *b;	\
	*d++ = *b;	\
	a++; b++; }

/*
==============
idCinematicROQ::yuv_to_rgb
==============
*/
unsigned short idCinematicROQ::yuv_to_rgb( int y, int u, int v ) const
{
	int YY = ( int ) ( ROQ_YY_tab[ ( y ) ] );

	int r = ( YY + ROQ_VR_tab[ v ] ) >> 9;
	int g = ( YY + ROQ_UG_tab[ u ] + ROQ_VG_tab[ v ] ) >> 8;
	int b = ( YY + ROQ_UB_tab[ u ] ) >> 9;

	if( r < 0 ) r = 0;
	if( g < 0 ) g = 0;
	if( b < 0 ) b = 0;
	if( r > 31 ) r = 31;
	if( g > 63 ) g = 63;
	if( b > 31 ) b = 31;

	return ( unsigned short ) ( ( r << 11 ) + ( g << 5 ) + ( b ) );
}

/*
==============
idCinematicROQ::yuv_to_rgb24
==============
*/
unsigned int idCinematicROQ::yuv_to_rgb24( int y, int u, int v ) const
{
	int YY = ( int ) ( ROQ_YY_tab[ ( y ) ] );

	int r = ( YY + ROQ_VR_tab[ v ] ) >> 6;
	int g = ( YY + ROQ_UG_tab[ u ] + ROQ_VG_tab[ v ] ) >> 6;
	int b = ( YY + ROQ_UB_tab[ u ] ) >> 6;

	if( r < 0 ) r = 0;
	if( g < 0 ) g = 0;
	if( b < 0 ) b = 0;
	if( r > 255 ) r = 255;
	if( g > 255 ) g = 255;
	if( b > 255 ) b = 255;

	return LittleLong( ( r ) +( g << 8 ) + ( b << 16 ) );
	//Win return LittleLong ((r)|(g<<8)|(b<<16)|(255<<24));
	//Mac return ((r<<24)|(g<<16)|(b<<8))|(255);	//+(255<<24));
}

/*
==============
idCinematicROQ::decodeCodeBook
==============
*/
void idCinematicROQ::decodeCodeBook( byte* input, unsigned short roq_flags )
{
	int	i, j, two, four;
	unsigned short*	aptr, *bptr, *cptr, *dptr;
	int	y0, y1, y2, y3, cr, cb;
	unsigned int* iaptr, *ibptr, *icptr, *idptr;

	if( !roq_flags )
	{
		two = four = 256;
	}
	else
	{
		two = roq_flags >> 8;
		if( !two ) two = 256;
		four = roq_flags & 0xff;
	}

	four *= 2;

	bptr = ( unsigned short* ) vq2;

	if( !half )
	{
		if( !smootheddouble )
		{
			//
			// normal height
			//
			if( samplesPerPixel == 2 )
			{
				for( i = 0; i < two; i++ )
				{
					y0 = ( int ) * input++;
					y1 = ( int ) * input++;
					y2 = ( int ) * input++;
					y3 = ( int ) * input++;

					cr = ( int ) * input++;
					cb = ( int ) * input++;

					*bptr++ = yuv_to_rgb( y0, cr, cb );
					*bptr++ = yuv_to_rgb( y1, cr, cb );
					*bptr++ = yuv_to_rgb( y2, cr, cb );
					*bptr++ = yuv_to_rgb( y3, cr, cb );
				}

				cptr = ( unsigned short* ) vq4;
				dptr = ( unsigned short* ) vq8;

				for( i = 0; i < four; i++ )
				{
					aptr = ( unsigned short* ) vq2 + ( *input++ ) * 4;
					bptr = ( unsigned short* ) vq2 + ( *input++ ) * 4;
					for( j = 0; j < 2; j++ )
						VQ2TO4( aptr, bptr, cptr, dptr );
				}
			}
			else if( samplesPerPixel == 4 )
			{
				// Convert 2x2 pixel vectors from YCbCr to RGB
				ibptr = ( unsigned int* ) bptr;
				for( i = 0; i < two; i++ )
				{
					y0 = ( int ) * input++;
					y1 = ( int ) * input++;
					y2 = ( int ) * input++;
					y3 = ( int ) * input++;

					cr = ( int ) * input++;
					cb = ( int ) * input++;

				#if USE_INTRINSICS

					const __m128i YY = _mm_set_epi32( ROQ_YY_tab[ y3 ], ROQ_YY_tab[ y2 ], ROQ_YY_tab[ y1 ], ROQ_YY_tab[ y0 ] );
					const __m128i VR = _mm_set1_epi32( ROQ_VR_tab[ cb ] );
					const __m128i UG = _mm_set1_epi32( ROQ_UG_tab[ cr ] );
					const __m128i VG = _mm_set1_epi32( ROQ_VG_tab[ cb ] );
					const __m128i UB = _mm_set1_epi32( ROQ_UB_tab[ cr ] );

					//int r0 = ( YY0 + ROQ_VR_tab[ cb ] ) >> 6;
					//int r1 = ( YY1 + ROQ_VR_tab[ cb ] ) >> 6;
					//int r2 = ( YY2 + ROQ_VR_tab[ cb ] ) >> 6;
					//int r3 = ( YY3 + ROQ_VR_tab[ cb ] ) >> 6;
					__m128i _rrrr = _mm_srai_epi32( _mm_add_epi32( YY, VR ), 6 );

					//int g0 = ( YY0 + ROQ_UG_tab[ cr ] + ROQ_VG_tab[ cb ] ) >> 6;
					//int g1 = ( YY1 + ROQ_UG_tab[ cr ] + ROQ_VG_tab[ cb ] ) >> 6;
					//int g2 = ( YY2 + ROQ_UG_tab[ cr ] + ROQ_VG_tab[ cb ] ) >> 6;
					//int g3 = ( YY3 + ROQ_UG_tab[ cr ] + ROQ_VG_tab[ cb ] ) >> 6;
					__m128i _gggg = _mm_srai_epi32( _mm_add_epi32( YY, _mm_add_epi32( UG, VG ) ), 6 );

					//int b0 = ( YY0 + ROQ_UB_tab[ cr ] ) >> 6;
					//int b1 = ( YY1 + ROQ_UB_tab[ cr ] ) >> 6;
					//int b2 = ( YY2 + ROQ_UB_tab[ cr ] ) >> 6;
					//int b3 = ( YY3 + ROQ_UB_tab[ cr ] ) >> 6;
					__m128i _bbbb = _mm_srai_epi32( _mm_add_epi32( YY, UB ), 6 );

					//if( r < 0 ) r = 0;
					//if( g < 0 ) g = 0;
					//if( b < 0 ) b = 0;
					//if( r > 255 ) r = 255;
					//if( g > 255 ) g = 255;
					//if( b > 255 ) b = 255;
					//const __m128i _max = _mm_set1_epi32( 255 );
					//const __m128i _min = _mm_set1_epi32( 0 );
					//_rrrr = _mm_max_epi32( _min, _mm_min_epi32( _max, _rrrr ) );
					//_gggg = _mm_max_epi32( _min, _mm_min_epi32( _max, _gggg ) );
					//_bbbb = _mm_max_epi32( _min, _mm_min_epi32( _max, _bbbb ) );
					//return LittleLong( ( r0 ) +( g0 << 8 ) + ( b0 << 16 ) );
					//return LittleLong( ( r1 ) +( g1 << 8 ) + ( b1 << 16 ) );
					//return LittleLong( ( r2 ) +( g2 << 8 ) + ( b2 << 16 ) );
					//return LittleLong( ( r3 ) +( g3 << 8 ) + ( b3 << 16 ) );

					__m128i t0 = _mm_packs_epi32( _rrrr, _gggg );					// rrrrgggg	 saturate 16bit
					__m128i t1 = _mm_packs_epi32( _bbbb, _mm_set1_epi32( 255 ) );	// bbbb0000	 saturate 16bit

					__m128i t2 = _mm_unpacklo_epi16( t0, t1 );						// rbrbrbrb
					__m128i t3 = _mm_unpackhi_epi16( t0, t1 );						// g0g0g0g0
					__m128i t4 = _mm_unpacklo_epi16( t2, t3 );						// rgb0rgb0  01
					__m128i t5 = _mm_unpackhi_epi16( t2, t3 );						// rgb0rgb0  23

					_mm_store_si128( ( __m128i * )ibptr, _mm_packus_epi16( t4, t5 ) ); // rgb0_rgb0_rgb0_rgb0  8bit  saturate 0,255

					ibptr += 4;
				#else
					*ibptr++ = yuv_to_rgb24( y0, cr, cb );
					*ibptr++ = yuv_to_rgb24( y1, cr, cb );
					*ibptr++ = yuv_to_rgb24( y2, cr, cb );
					*ibptr++ = yuv_to_rgb24( y3, cr, cb );
				#endif
				}

				// Set up 4x4 and 8x8 pixel vectors
				icptr = ( unsigned int* ) vq4;
				idptr = ( unsigned int* ) vq8;

				for( i = 0; i < four; i++ )
				{
					iaptr = ( unsigned int* ) vq2 + ( *input++ ) * 4;
					ibptr = ( unsigned int* ) vq2 + ( *input++ ) * 4;
					for( j = 0; j < 2; j++ )
						VQ2TO4( iaptr, ibptr, icptr, idptr );
					/*{
					*icptr++ = iaptr[ 0 ];
					*idptr++ = iaptr[ 0 ];
					*idptr++ = iaptr[ 0 ];
					*icptr++ = iaptr[ 1 ];
					*idptr++ = iaptr[ 1 ];
					*idptr++ = iaptr[ 1 ];

					*icptr++ = ibptr[ 0 ];
					*idptr++ = ibptr[ 0 ];
					*idptr++ = ibptr[ 0 ];
					*icptr++ = ibptr[ 1 ];
					*idptr++ = ibptr[ 1 ];
					*idptr++ = ibptr[ 1 ];

					*idptr++ = iaptr[ 0 ];
					*idptr++ = iaptr[ 0 ];
					*idptr++ = iaptr[ 1 ];
					*idptr++ = iaptr[ 1 ];

					*idptr++ = ibptr[ 0 ];
					*idptr++ = ibptr[ 0 ];
					*idptr++ = ibptr[ 1 ];
					*idptr++ = ibptr[ 1 ];

					iaptr += 2;
					ibptr += 2;
					}*/
				}
			}
		}
		else
		{
			//
			// double height, smoothed
			//
			if( samplesPerPixel == 2 )
			{
				for( i = 0; i < two; i++ )
				{
					y0 = ( int ) * input++;
					y1 = ( int ) * input++;
					y2 = ( int ) * input++;
					y3 = ( int ) * input++;

					cr = ( int ) * input++;
					cb = ( int ) * input++;

					*bptr++ = yuv_to_rgb( y0, cr, cb );
					*bptr++ = yuv_to_rgb( y1, cr, cb );
					*bptr++ = yuv_to_rgb( ( ( y0 * 3 ) + y2 ) / 4, cr, cb );
					*bptr++ = yuv_to_rgb( ( ( y1 * 3 ) + y3 ) / 4, cr, cb );
					*bptr++ = yuv_to_rgb( ( y0 + ( y2 * 3 ) ) / 4, cr, cb );
					*bptr++ = yuv_to_rgb( ( y1 + ( y3 * 3 ) ) / 4, cr, cb );
					*bptr++ = yuv_to_rgb( y2, cr, cb );
					*bptr++ = yuv_to_rgb( y3, cr, cb );
				}

				cptr = ( unsigned short* ) vq4;
				dptr = ( unsigned short* ) vq8;

				for( i = 0; i < four; i++ )
				{
					aptr = ( unsigned short* ) vq2 + ( *input++ ) * 8;
					bptr = ( unsigned short* ) vq2 + ( *input++ ) * 8;
					for( j = 0; j < 2; j++ )
					{
						VQ2TO4( aptr, bptr, cptr, dptr );
						VQ2TO4( aptr, bptr, cptr, dptr );
					}
				}
			}
			else if( samplesPerPixel == 4 )
			{
				ibptr = ( unsigned int* ) bptr;
				for( i = 0; i < two; i++ )
				{
					y0 = ( int ) * input++;
					y1 = ( int ) * input++;
					y2 = ( int ) * input++;
					y3 = ( int ) * input++;

					cr = ( int ) * input++;
					cb = ( int ) * input++;

					*ibptr++ = yuv_to_rgb24( y0, cr, cb );
					*ibptr++ = yuv_to_rgb24( y1, cr, cb );
					*ibptr++ = yuv_to_rgb24( ( ( y0 * 3 ) + y2 ) / 4, cr, cb );
					*ibptr++ = yuv_to_rgb24( ( ( y1 * 3 ) + y3 ) / 4, cr, cb );
					*ibptr++ = yuv_to_rgb24( ( y0 + ( y2 * 3 ) ) / 4, cr, cb );
					*ibptr++ = yuv_to_rgb24( ( y1 + ( y3 * 3 ) ) / 4, cr, cb );
					*ibptr++ = yuv_to_rgb24( y2, cr, cb );
					*ibptr++ = yuv_to_rgb24( y3, cr, cb );
				}

				icptr = ( unsigned int* ) vq4;
				idptr = ( unsigned int* ) vq8;

				for( i = 0; i < four; i++ )
				{
					iaptr = ( unsigned int* ) vq2 + ( *input++ ) * 8;
					ibptr = ( unsigned int* ) vq2 + ( *input++ ) * 8;
					for( j = 0; j < 2; j++ )
					{
						VQ2TO4( iaptr, ibptr, icptr, idptr );
						VQ2TO4( iaptr, ibptr, icptr, idptr );
					}
				}
			}
		}
	}
	else
	{
		//
		// 1/4 screen
		//
		if( samplesPerPixel == 2 )
		{
			for( i = 0; i < two; i++ )
			{
				y0 = ( int ) * input; input += 2;
				y2 = ( int ) * input; input += 2;

				cr = ( int ) * input++;
				cb = ( int ) * input++;

				*bptr++ = yuv_to_rgb( y0, cr, cb );
				*bptr++ = yuv_to_rgb( y2, cr, cb );
			}

			cptr = ( unsigned short* ) vq4;
			dptr = ( unsigned short* ) vq8;

			for( i = 0; i < four; i++ )
			{
				aptr = ( unsigned short* ) vq2 + ( *input++ ) * 2;
				bptr = ( unsigned short* ) vq2 + ( *input++ ) * 2;
				for( j = 0; j < 2; j++ )
				{
					VQ2TO2( aptr, bptr, cptr, dptr );
				}
			}
		}
		else if( samplesPerPixel == 4 )
		{
			ibptr = ( unsigned int* ) bptr;
			for( i = 0; i < two; i++ )
			{
				y0 = ( int ) * input; input += 2;
				y2 = ( int ) * input; input += 2;

				cr = ( int ) * input++;
				cb = ( int ) * input++;

				*ibptr++ = yuv_to_rgb24( y0, cr, cb );
				*ibptr++ = yuv_to_rgb24( y2, cr, cb );
			}

			icptr = ( unsigned int* ) vq4;
			idptr = ( unsigned int* ) vq8;

			for( i = 0; i < four; i++ )
			{
				iaptr = ( unsigned int* ) vq2 + ( *input++ ) * 2;
				ibptr = ( unsigned int* ) vq2 + ( *input++ ) * 2;
				for( j = 0; j < 2; j++ )
				{
					VQ2TO2( iaptr, ibptr, icptr, idptr );
				}
			}
		}
	}
}

/*
==============
idCinematicROQ::recurseQuad
==============
*/
void idCinematicROQ::recurseQuad( int startX, int startY, int quadSize, int xOff, int yOff )
{
	int offset = screenDelta;
	const int lowx = 0;
	const int lowy = 0;
	int bigx = xsize;
	int bigy = ysize;

	if( bigx > CIN_WIDTH ) bigx = CIN_WIDTH;
	if( bigy > CIN_HEIGHT ) bigy = CIN_HEIGHT;

	if( ( startX >= lowx ) &&
		( startY >= lowy ) &&
		( startX + quadSize ) <= ( bigx ) &&
		( startY + quadSize ) <= ( bigy ) &&
		quadSize <= MAXSIZE )
	{
		int useY = startY;
		byte* scroff = imageData + ( useY + ( ( CIN_HEIGHT - bigy ) >> 1 ) + yOff ) * ( samplesPerLine ) + ( ( ( startX + xOff ) ) * samplesPerPixel );

		qStatus[ 0 ][ onQuad ] = scroff;
		qStatus[ 1 ][ onQuad++ ] = scroff + offset;
	}

	if( quadSize != MINSIZE )
	{
		quadSize >>= 1;
		recurseQuad( startX, startY, quadSize, xOff, yOff );
		recurseQuad( startX + quadSize, startY, quadSize, xOff, yOff );
		recurseQuad( startX, startY + quadSize, quadSize, xOff, yOff );
		recurseQuad( startX + quadSize, startY + quadSize, quadSize, xOff, yOff );
	}
}

/*
==============
idCinematicROQ::setupQuad
==============
*/
void idCinematicROQ::setupQuad( int xOff, int yOff )
{
	int numQuadCels = ( CIN_WIDTH * CIN_HEIGHT ) / 16;
	numQuadCels += numQuadCels / 4 + numQuadCels / 16;
	numQuadCels += 64; // for overflow

	numQuadCels = ( xsize * ysize ) / ( 16 );
	numQuadCels += numQuadCels / 4;
	numQuadCels += 64; // for overflow

	onQuad = 0;

	for( int y = 0; y < ( int ) ysize; y += 16 )
		for( int x = 0; x < ( int ) xsize; x += 16 )
			recurseQuad( x, y, 16, xOff, yOff );

	byte *temp = NULL;

	for( int i = ( numQuadCels - 64 ); i < numQuadCels; i++ )
	{
		qStatus[ 0 ][ i ] = temp;	// eoq
		qStatus[ 1 ][ i ] = temp;	// eoq
	}
}

/*
==============
idCinematicROQ::readQuadInfo
==============
*/
void idCinematicROQ::readQuadInfo( byte* qData )
{
	xsize = qData[ 0 ] + qData[ 1 ] * 256;
	ysize = qData[ 2 ] + qData[ 3 ] * 256;
	maxsize = qData[ 4 ] + qData[ 5 ] * 256;
	minsize = qData[ 6 ] + qData[ 7 ] * 256;

	CIN_HEIGHT = ysize;
	CIN_WIDTH = xsize;

	samplesPerLine = CIN_WIDTH * samplesPerPixel;
	screenDelta = CIN_HEIGHT * samplesPerLine;

	if( !imageData )
	{
		imageData = ( byte* ) Mem_Alloc( CIN_WIDTH * CIN_HEIGHT * samplesPerPixel * 2, TAG_CINEMATIC );
	}

	half = false;
	smootheddouble = false;

	// RB: 64 bit fixes, changed unsigned int to ptrdiff_t
	t[ 0 ] = ( 0 - ( ptrdiff_t ) imageData ) + ( ptrdiff_t ) imageData + screenDelta;
	t[ 1 ] = ( 0 - ( ( ptrdiff_t ) imageData + screenDelta ) ) + ( ptrdiff_t ) imageData;
	// RB end

	drawX = CIN_WIDTH;
	drawY = CIN_HEIGHT;
}

/*
==============
idCinematicROQ::RoQPrepMcomp
==============
*/
void idCinematicROQ::RoQPrepMcomp( int xoff, int yoff )
{
	int i = samplesPerLine;
	int j = samplesPerPixel;
	if( xsize == ( ysize * 4 ) && !half )
	{
		j = j + j;
		i = i + i;
	}

	for( int y = 0; y < 16; y++ )
	{
		int temp2 = ( y + yoff - 8 ) * i;
		for( int x = 0; x < 16; x++ )
		{
			int temp = ( x + xoff - 8 ) * j;
			mcomp[ ( x * 16 ) + y ] = normalBuffer0 - ( temp2 + temp );
		}
	}
}

/*
==============
idCinematicROQ::RoQReset
==============
*/
void idCinematicROQ::RoQReset()
{
	iFile->Seek( 0, FS_SEEK_SET );
	iFile->Read( file, 16 );
	RoQInit();
	status = FMV_LOOPED;
}

struct my_source_mgr {
	struct jpeg_source_mgr pub;	/* public fields */

	byte*   infile;		/* source stream */
	JOCTET* buffer;		/* start of buffer */
	boolean start_of_file;	/* have we gotten any data yet? */
	int	memsize;
};

typedef my_source_mgr* my_src_ptr;

#define INPUT_BUF_SIZE  32768	/* choose an efficiently fread'able size */

/* jpeg error handling */
struct jpeg_error_mgr jerr;

/*
* Fill the input buffer --- called whenever buffer is emptied.
*
* In typical applications, this should read fresh data into the buffer
* (ignoring the current state of next_input_byte & bytes_in_buffer),
* reset the pointer & count to the start of the buffer, and return TRUE
* indicating that the buffer has been reloaded.  It is not necessary to
* fill the buffer entirely, only to obtain at least one more byte.
*
* There is no such thing as an EOF return.  If the end of the file has been
* reached, the routine has a choice of ERREXIT() or inserting fake data into
* the buffer.  In most cases, generating a warning message and inserting a
* fake EOI marker is the best course of action --- this will allow the
* decompressor to output however much of the image is there.  However,
* the resulting error message is misleading if the real problem is an empty
* input file, so we handle that case specially.
*
* In applications that need to be able to suspend compression due to input
* not being available yet, a FALSE return indicates that no more data can be
* obtained right now, but more may be forthcoming later.  In this situation,
* the decompressor will return to its caller (with an indication of the
* number of scanlines it has read, if any).  The application should resume
* decompression after it has loaded more data into the input buffer.  Note
* that there are substantial restrictions on the use of suspension --- see
* the documentation.
*
* When suspending, the decompressor will back up to a convenient restart point
* (typically the start of the current MCU). next_input_byte & bytes_in_buffer
* indicate where the restart point will be if the current call returns FALSE.
* Data beyond this point must be rescanned after resumption, so move it to
* the front of the buffer rather than discarding it.
*/

#ifdef USE_NEWER_JPEG
METHODDEF( boolean ) fill_input_buffer( j_decompress_ptr cinfo )
#else
METHODDEF boolean fill_input_buffer( j_decompress_ptr cinfo )
#endif
{
	my_src_ptr src = ( my_src_ptr ) cinfo->src;
	int nbytes;

	nbytes = INPUT_BUF_SIZE;
	if( nbytes > src->memsize ) nbytes = src->memsize;
	if( nbytes == 0 )
	{
		/* Insert a fake EOI marker */
		src->buffer[ 0 ] = ( JOCTET ) 0xFF;
		src->buffer[ 1 ] = ( JOCTET ) JPEG_EOI;
		nbytes = 2;
	}
	else
	{
		memcpy( src->buffer, src->infile, INPUT_BUF_SIZE );
		src->infile = src->infile + nbytes;
		src->memsize = src->memsize - INPUT_BUF_SIZE;
	}
	src->pub.next_input_byte = src->buffer;
	src->pub.bytes_in_buffer = nbytes;
	src->start_of_file = FALSE;

	return TRUE;
}
/*
* Initialize source --- called by jpeg_read_header
* before any data is actually read.
*/

#ifdef USE_NEWER_JPEG
METHODDEF( void  init_source( j_decompress_ptr cinfo )
	   #else
METHODDEF void init_source( j_decompress_ptr cinfo )
#endif
{
	my_src_ptr src = ( my_src_ptr ) cinfo->src;

	/* We reset the empty-input-file flag for each image,
	* but we don't clear the input buffer.
	* This is correct behavior for reading a series of images from one source.
	*/
	src->start_of_file = TRUE;
}

/*
* Skip data --- used to skip over a potentially large amount of
* uninteresting data (such as an APPn marker).
*
* Writers of suspendable-input applications must note that skip_input_data
* is not granted the right to give a suspension return.  If the skip extends
* beyond the data currently in the buffer, the buffer can be marked empty so
* that the next read will cause a fill_input_buffer call that can suspend.
* Arranging for additional bytes to be discarded before reloading the input
* buffer is the application writer's problem.
*/

#ifdef USE_NEWER_JPEG
METHODDEF( void ) skip_input_data( j_decompress_ptr cinfo, long num_bytes )
#else
METHODDEF void skip_input_data( j_decompress_ptr cinfo, long num_bytes )
#endif
{
	my_src_ptr src = ( my_src_ptr ) cinfo->src;

	/* Just a dumb implementation for now.  Could use fseek() except
	* it doesn't work on pipes.  Not clear that being smart is worth
	* any trouble anyway --- large skips are infrequent.
	*/
	if( num_bytes > 0 )
	{
		src->infile = src->infile + num_bytes;
		src->pub.next_input_byte += ( size_t ) num_bytes;
		src->pub.bytes_in_buffer -= ( size_t ) num_bytes;
	}
}

/*
* An additional method that can be provided by data source modules is the
* resync_to_restart method for error recovery in the presence of RST markers.
* For the moment, this source module just uses the default resync method
* provided by the JPEG library.  That method assumes that no backtracking
* is possible.
*/

/*
* Terminate source --- called by jpeg_finish_decompress
* after all data has been read.  Often a no-op.
*
* NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
* application must deal with any cleanup that should happen even
* for error exit.
*/

#ifdef USE_NEWER_JPEG
METHODDEF( void ) term_source( j_decompress_ptr cinfo )
#else
METHODDEF void term_source( j_decompress_ptr cinfo )
#endif
{
	cinfo = cinfo;
	/* no work necessary here */
}

#ifdef USE_NEWER_JPEG
GLOBAL( void ) jpeg_memory_src( j_decompress_ptr cinfo, byte* infile, int size )
#else
GLOBAL void jpeg_memory_src( j_decompress_ptr cinfo, byte* infile, int size )
#endif
{
	my_src_ptr src;

	/* The source object and input buffer are made permanent so that a series
	* of JPEG images can be read from the same file by calling jpeg_stdio_src
	* only before the first one.  (If we discarded the buffer at the end of
	* one image, we'd likely lose the start of the next one.)
	* This makes it unsafe to use this manager and a different source
	* manager serially with the same JPEG object.  Caveat programmer.
	*/
	if( cinfo->src == NULL )  /* first time for this JPEG object? */
	{
		cinfo->src = ( struct jpeg_source_mgr* ) ( *cinfo->mem->alloc_small )( ( j_common_ptr ) cinfo, JPOOL_PERMANENT, sizeof( my_source_mgr ) );
		src = ( my_src_ptr ) cinfo->src;
		src->buffer = ( JOCTET* ) ( *cinfo->mem->alloc_small )( ( j_common_ptr ) cinfo, JPOOL_PERMANENT, INPUT_BUF_SIZE * sizeof( JOCTET ) );
	}

	src = ( my_src_ptr ) cinfo->src;
	src->pub.init_source = init_source;
	src->pub.fill_input_buffer = fill_input_buffer;
	src->pub.skip_input_data = skip_input_data;
	src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
	src->pub.term_source = term_source;
	src->infile = infile;
	src->memsize = size;
	src->pub.bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
	src->pub.next_input_byte = NULL; /* until buffer loaded */
}

static int JPEGBlit( byte* wStatus, byte* data, int datasize )
{
	/* This struct contains the JPEG decompression parameters and pointers to
	* working space (which is allocated as needed by the JPEG library).
	*/
	struct jpeg_decompress_struct cinfo;
	/* We use our private extension JPEG error handler.
	* Note that this struct must live as long as the main JPEG parameter
	* struct, to avoid dangling-pointer problems.
	*/
	/* More stuff */
	JSAMPARRAY buffer;		/* Output row buffer */
	int row_stride;		/* physical row width in output buffer */

						/* Step 1: allocate and initialize JPEG decompression object */

						/* We set up the normal JPEG error routines, then override error_exit. */
	cinfo.err = jpeg_std_error( &jerr );

	/* Now we can initialize the JPEG decompression object. */
	jpeg_create_decompress( &cinfo );

	/* Step 2: specify data source (eg, a file) */

	jpeg_memory_src( &cinfo, data, datasize );

	/* Step 3: read file parameters with jpeg_read_header() */

	( void ) jpeg_read_header( &cinfo, TRUE );
	/* We can ignore the return value from jpeg_read_header since
	*   (a) suspension is not possible with the stdio data source, and
	*   (b) we passed TRUE to reject a tables-only JPEG file as an error.
	* See libjpeg.doc for more info.
	*/

	/* Step 4: set parameters for decompression */

	/* In this example, we don't need to change any of the defaults set by
	* jpeg_read_header(), so we do nothing here.
	*/

	/* Step 5: Start decompressor */

	cinfo.dct_method = JDCT_IFAST;
	cinfo.dct_method = JDCT_FASTEST;
	cinfo.dither_mode = JDITHER_NONE;
	cinfo.do_fancy_upsampling = FALSE;
	//	cinfo.out_color_space = JCS_GRAYSCALE;

	( void ) jpeg_start_decompress( &cinfo );
	/* We can ignore the return value since suspension is not possible
	* with the stdio data source.
	*/

	/* We may need to do some setup of our own at this point before reading
	* the data.  After jpeg_start_decompress() we have the correct scaled
	* output image dimensions available, as well as the output colormap
	* if we asked for color quantization.
	* In this example, we need to make an output work buffer of the right size.
	*/
	/* JSAMPLEs per row in output buffer */
	row_stride = cinfo.output_width * cinfo.output_components;

	/* Make a one-row-high sample array that will go away when done with image */
	buffer = ( *cinfo.mem->alloc_sarray ) ( ( j_common_ptr ) &cinfo, JPOOL_IMAGE, row_stride, 1 );

	/* Step 6: while (scan lines remain to be read) */
	/*           jpeg_read_scanlines(...); */

	/* Here we use the library's state variable cinfo.output_scanline as the
	* loop counter, so that we don't have to keep track ourselves.
	*/

	wStatus += ( cinfo.output_height - 1 ) * row_stride;
	while( cinfo.output_scanline < cinfo.output_height )
	{
		/* jpeg_read_scanlines expects an array of pointers to scanlines.
		* Here the array is only one element long, but you could ask for
		* more than one scanline at a time if that's more convenient.
		*/
		( void ) jpeg_read_scanlines( &cinfo, &buffer[ 0 ], 1 );

		/* Assume put_scanline_someplace wants a pointer and sample count. */
		memcpy( wStatus, &buffer[ 0 ][ 0 ], row_stride );
		/*
		int x;
		unsigned int *buf = (unsigned int *)&buffer[0][0];
		unsigned int *out = (unsigned int *)wStatus;
		for(x=0;x<cinfo.output_width;x++) {
		unsigned int pixel = buf[x];
		byte *roof = (byte *)&pixel;
		byte temp = roof[0];
		roof[0] = roof[2];
		roof[2] = temp;
		out[x] = pixel;
		}
		*/
		wStatus -= row_stride;
	}

	/* Step 7: Finish decompression */

	( void ) jpeg_finish_decompress( &cinfo );
	/* We can ignore the return value since suspension is not possible
	* with the stdio data source.
	*/

	/* Step 8: Release JPEG decompression object */

	/* This is an important step since it will release a good deal of memory. */
	jpeg_destroy_decompress( &cinfo );

	/* At this point you may want to check to see whether any corrupt-data
	* warnings occurred (test whether jerr.pub.num_warnings is nonzero).
	*/

	/* And we're done! */
	return 1;
};

/*
==============
idCinematicROQ::RoQInterrupt
==============
*/
void idCinematicROQ::RoQInterrupt()
{
	iFile->Read( file, RoQFrameSize + 8 );
	if( RoQPlayed >= ROQSize )
	{
		if( looping )
		{
			RoQReset();
		}
		else
		{
			status = FMV_EOF;
		}
		return;
	}

	byte* framedata = file;
	//
	// new frame is ready
	//
redump:
	switch( roq_id )
	{
		case ROQ_QUAD_VQ:
			if( ( numQuads & 1 ) )
			{
				normalBuffer0 = t[ 1 ];
				RoQPrepMcomp( roqF0, roqF1 );
				blitVQQuad32fs( qStatus[ 1 ], framedata );
				buf = imageData + screenDelta;
			}
			else
			{
				normalBuffer0 = t[ 0 ];
				RoQPrepMcomp( roqF0, roqF1 );
				blitVQQuad32fs( qStatus[ 0 ], framedata );
				buf = imageData;
			}
			if( numQuads == 0 ) // first frame
			{
				memcpy( imageData + screenDelta, imageData, samplesPerLine * ysize );
			}
			numQuads++;
			dirty = true;
			break;

		case ROQ_CODEBOOK:
			decodeCodeBook( framedata, ( unsigned short ) roq_flags );
			break;

		case ZA_SOUND_MONO:
			break;
		case ZA_SOUND_STEREO:
			break;

		case ROQ_QUAD_INFO:
			if( numQuads == -1 )
			{
				readQuadInfo( framedata );
				setupQuad( 0, 0 );
			}
			if( numQuads != 1 ) numQuads = 0;
			break;

		case ROQ_PACKET:
			inMemory = ( roq_flags != 0 );
			RoQFrameSize = 0; // for header
			break;

		case ROQ_QUAD_HANG:
			RoQFrameSize = 0;
			break;

		case ROQ_QUAD_JPEG:
			if( !numQuads )
			{
				normalBuffer0 = t[ 0 ];
				JPEGBlit( imageData, framedata, RoQFrameSize );
				memcpy( imageData + screenDelta, imageData, samplesPerLine * ysize );
				numQuads++;
			}
			break;

		default:
			status = FMV_EOF;
			break;
	}
	//
	// read in next frame data
	//
	if( RoQPlayed >= ROQSize )
	{
		if( looping )
		{
			RoQReset();
		}
		else
		{
			status = FMV_EOF;
		}
		return;
	}

	framedata += RoQFrameSize;
	roq_id		 = framedata[ 0 ] + framedata[ 1 ] * 256;
	RoQFrameSize = framedata[ 2 ] + framedata[ 3 ] * 256 + framedata[ 4 ] * 65536;
	roq_flags	 = framedata[ 6 ] + framedata[ 7 ] * 256;
	roqF0 = ( signed char ) framedata[ 7 ];
	roqF1 = ( signed char ) framedata[ 6 ];

	if( RoQFrameSize > 65536 || roq_id == ROQ_FILE )
	{
		common->DPrintf( "roq_size > 65536 || roq_id == ROQ_FILE\n" );
		status = FMV_EOF;
		if( looping )
		{
			RoQReset();
		}
		return;
	}
	if( inMemory && ( status != FMV_EOF ) )
	{
		inMemory = false;
		framedata += 8;
		goto redump;
	}
	//
	// one more frame hits the dust
	//
	//	assert(RoQFrameSize <= 65536);
	//	r = Sys_StreamedRead( file, RoQFrameSize+8, 1, iFile );
	RoQPlayed += RoQFrameSize + 8;
}

/*
==============
idCinematicROQ::RoQInit
==============
*/
void idCinematicROQ::RoQInit()
{
	RoQPlayed = 24;

	// get frame rate
	roqFPS = file[ 6 ] + file[ 7 ] * 256;

	if( !roqFPS ) roqFPS = 30;

	numQuads = -1;

	roq_id = file[ 8 ] + file[ 9 ] * 256;
	RoQFrameSize = file[ 10 ] + file[ 11 ] * 256 + file[ 12 ] * 65536;
	roq_flags = file[ 14 ] + file[ 15 ] * 256;
}

/*
==============
idCinematicROQ::RoQShutdown
==============
*/
void idCinematicROQ::RoQShutdown()
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

	fileName = "";
}
