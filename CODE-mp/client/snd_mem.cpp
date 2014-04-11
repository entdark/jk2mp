
/*****************************************************************************
 * name:		snd_mem.c
 *
 * desc:		sound caching
 *
 * $Archive: /MissionPack/code/client/snd_mem.c $
 * $Author: Raduffy $ 
 * $Revision: 39 $
 * $Modtime: 12/04/00 10:58a $
 * $Date: 12/04/00 11:06a $
 *
 *****************************************************************************/
#include "snd_local.h"

#include "snd_mp3.h"

// Open AL
extern int s_UseOpenAL;

/*
===============================================================================

WAV loading

===============================================================================
*/

static	byte	*data_p;
static	byte 	*iff_end;
static	byte 	*last_chunk;
static	byte 	*iff_data;
static	int 	iff_chunk_len;

static short GetLittleShort(void)
{
	short val = 0;
	val = *data_p;
	val = val + (*(data_p+1)<<8);
	data_p += 2;
	return val;
}

static int GetLittleLong(void)
{
	int val = 0;
	val = *data_p;
	val = val + (*(data_p+1)<<8);
	val = val + (*(data_p+2)<<16);
	val = val + (*(data_p+3)<<24);
	data_p += 4;
	return val;
}

static void FindNextChunk(char *name)
{
	while (1)
	{
		data_p=last_chunk;

		if (data_p >= iff_end)
		{	// didn't find the chunk
			data_p = NULL;
			return;
		}
		
		data_p += 4;
		iff_chunk_len = GetLittleLong();
		if (iff_chunk_len < 0)
		{
			data_p = NULL;
			return;
		}
		data_p -= 8;
		last_chunk = data_p + 8 + ( (iff_chunk_len + 1) & ~1 );
		if (!strncmp((char *)data_p, name, 4))
			return;
	}
}

static void FindChunk(char *name)
{
	last_chunk = iff_data;
	FindNextChunk (name);
}

/*
============
GetWavinfo
============
*/
static wavinfo_t GetWavinfo (char *name, byte *wav, int wavlength)
{
	wavinfo_t	info;

	Com_Memset (&info, 0, sizeof(info));

	if (!wav)
		return info;
		
	iff_data = wav;
	iff_end = wav + wavlength;

// find "RIFF" chunk
	FindChunk("RIFF");
	if (!(data_p && !strncmp((char *)data_p+8, "WAVE", 4)))
	{
		Com_Printf("Missing RIFF/WAVE chunks\n");
		return info;
	}

// get "fmt " chunk
	iff_data = data_p + 12;
// DumpChunks ();

	FindChunk("fmt ");
	if (!data_p)
	{
		Com_Printf("Missing fmt chunk\n");
		return info;
	}
	data_p += 8;
	info.format = GetLittleShort();
	info.channels = GetLittleShort();
	info.rate = GetLittleLong();
	data_p += 4+2;
	info.width = GetLittleShort() / 8;

	if (info.format != 1)
	{
		Com_Printf("Microsoft PCM format only\n");
		return info;
	}


// find data chunk
	FindChunk("data");
	if (!data_p)
	{
		Com_Printf("Missing data chunk\n");
		return info;
	}

	data_p += 4;
	info.samples = GetLittleLong () / info.width;
	info.dataofs = data_p - wav;

	return info;
}


/*
================
ResampleSfx

resample / decimate to the current source rate
================
*/
static 
void ResampleSfx (sfx_t *sfx, int iInRate, int iInWidth, byte *pData)
{
	int		iOutCount;
	int		iSrcSample;
	float	fStepScale;
	int		i;
	int		iSample;
	unsigned int uiSampleFrac, uiFracStep;	// uiSampleFrac MUST be unsigned, or large samples (eg music tracks) crash
	
	fStepScale = (float)iInRate / dma.speed;	// this is usually 0.5, 1, or 2

	// When stepscale is > 1 (we're downsampling), we really ought to run a low pass filter on the samples

	iOutCount = (int)(sfx->iSoundLengthInSamples / fStepScale);
	sfx->iSoundLengthInSamples = iOutCount;

	sfx->pSoundData = (short *) SND_malloc( sfx->iSoundLengthInSamples*2 ,sfx );

	sfx->fVolRange	= 0;
	uiSampleFrac	= 0;
	uiFracStep		= (int)(fStepScale*256);

	for (i=0 ; i<sfx->iSoundLengthInSamples ; i++)
	{
		iSrcSample = uiSampleFrac >> 8;
		uiSampleFrac += uiFracStep;
		if (iInWidth == 2) {
			iSample = LittleShort ( ((short *)pData)[iSrcSample] );			
		} else {
			iSample = (int)( (unsigned char)(pData[iSrcSample]) - 128) << 8;			
		}

		sfx->pSoundData[i] = (short)iSample;

		// work out max vol for this sample...
		//
		if (iSample < 0)
			iSample = -iSample;
		if (sfx->fVolRange < (iSample >> 8) )
		{
			sfx->fVolRange =  iSample >> 8;
		}
	}
}

// (MP3 helper func)
//
void S_LoadSound_Finalize(wavinfo_t	*info, sfx_t *sfx, byte *data)
{				   
	float	stepscale	= (float)info->rate / dma.speed;	
	int		len			= (int)(info->samples / stepscale);

	len *= info->width;

	sfx->eSoundCompressionMethod = ct_16;
	sfx->iSoundLengthInSamples	 = info->samples;
	ResampleSfx( sfx, info->rate, info->width, data + info->dataofs );	
}



// adjust filename for foreign languages and WAV/MP3 issues. 
//
// returns qfalse if failed to load, else fills in *pData
//
extern	cvar_t	*com_buildScript;
extern	cvar_t* s_language;
static qboolean S_LoadSound_FileLoadAndNameAdjuster(char *psFilename, byte **pData, int *piSize, int iNameStrlen)
{
	char *psVoice = strstr(psFilename,"chars");
	if (psVoice)
	{
		// cache foreign voices...
		//		
		if (com_buildScript->integer)
		{
			fileHandle_t hFile;
			//German
			strncpy(psVoice,"chr_d",5);	// same number of letters as "chars"
			FS_FOpenFileRead(psFilename, &hFile, qfalse);		//cache the wav
			if (!hFile)
			{
				strcpy(&psFilename[iNameStrlen-3],"mp3");		//not there try mp3
				FS_FOpenFileRead(psFilename, &hFile, qfalse);	//cache the mp3
			}
			if (hFile)
			{
				FS_FCloseFile(hFile);
			}
			strcpy(&psFilename[iNameStrlen-3],"wav");	//put it back to wav

			//French
			strncpy(psVoice,"chr_f",5);	// same number of letters as "chars"
			FS_FOpenFileRead(psFilename, &hFile, qfalse);		//cache the wav
			if (!hFile)
			{
				strcpy(&psFilename[iNameStrlen-3],"mp3");		//not there try mp3
				FS_FOpenFileRead(psFilename, &hFile, qfalse);	//cache the mp3
			}
			if (hFile)
			{
				FS_FCloseFile(hFile);
			}
			strcpy(&psFilename[iNameStrlen-3],"wav");	//put it back to wav

			//Spanish
			strncpy(psVoice,"chr_e",5);	// same number of letters as "chars"
			FS_FOpenFileRead(psFilename, &hFile, qfalse);		//cache the wav
			if (!hFile)
			{
				strcpy(&psFilename[iNameStrlen-3],"mp3");		//not there try mp3
				FS_FOpenFileRead(psFilename, &hFile, qfalse);	//cache the mp3
			}
			if (hFile)
			{
				FS_FCloseFile(hFile);
			}
			strcpy(&psFilename[iNameStrlen-3],"wav");	//put it back to wav

			strncpy(psVoice,"chars",5);	//put it back to chars
		}

		// account for foreign voices...
		if (s_language && Q_stricmp("DEUTSCH",s_language->string)==0)
		{				
			strncpy(psVoice,"chr_d",5);	// same number of letters as "chars"
		}
		else if (s_language && Q_stricmp("FRANCAIS",s_language->string)==0)
		{				
			strncpy(psVoice,"chr_f",5);	// same number of letters as "chars"
		}
		else if (s_language && Q_stricmp("ESPANOL",s_language->string)==0)
		{				
			strncpy(psVoice,"chr_e",5);	// same number of letters as "chars"
		}
		else
		{
			psVoice = NULL;	// use this ptr as a flag as to whether or not we substituted with a foreign version
		}
	}

	*piSize = FS_ReadFile( psFilename, (void **)pData );	// try WAV
	if ( !*pData ) {
		psFilename[iNameStrlen-3] = 'm';
		psFilename[iNameStrlen-2] = 'p';
		psFilename[iNameStrlen-1] = '3';
		*piSize = FS_ReadFile( psFilename, (void **)pData );	// try MP3

		if ( !*pData ) 
		{
			//hmmm, not found, ok, maybe we were trying a foreign noise ("arghhhhh.mp3" that doesn't matter?) but it
			// was missing?   Can't tell really, since both types are now in sound/chars. Oh well, fall back to English for now...
			
			if (psVoice)	// were we trying to load foreign?
			{
				// yep, so fallback to re-try the english...
				//
#ifndef FINAL_BUILD
				Com_Printf(S_COLOR_YELLOW "Foreign file missing: \"%s\"! (using English...)\n",psFilename);
#endif

				strncpy(psVoice,"chars",5);

				psFilename[iNameStrlen-3] = 'w';
				psFilename[iNameStrlen-2] = 'a';
				psFilename[iNameStrlen-1] = 'v';
				*piSize = FS_ReadFile( psFilename, (void **)pData );	// try English WAV
				if ( !*pData ) 
				{						
					psFilename[iNameStrlen-3] = 'm';
					psFilename[iNameStrlen-2] = 'p';
					psFilename[iNameStrlen-1] = '3';
					*piSize = FS_ReadFile( psFilename, (void **)pData );	// try English MP3
				}
			}

			if (!*pData)
			{
				return qfalse;	// sod it, give up...
			}
		}
	}

	return qtrue;
}


static qboolean S_SwitchLang(char *psFilename) {
	int iNameStrlen = strlen(psFilename);
	char *psVoice = strstr(psFilename,"chars");
	if (psVoice) {
		// cache foreign voices...
		//		
		if (com_buildScript->integer) {
			fileHandle_t hFile;
			//German
			strncpy(psVoice,"chr_d",5);	// same number of letters as "chars"
			FS_FOpenFileRead(psFilename, &hFile, qfalse);		//cache the wav
			if (!hFile) {
				strcpy(&psFilename[iNameStrlen-3],"mp3");		//not there try mp3
				FS_FOpenFileRead(psFilename, &hFile, qfalse);	//cache the mp3
			}
			if (hFile) {
				FS_FCloseFile(hFile);
			}
			strcpy(&psFilename[iNameStrlen-3],"wav");	//put it back to wav

			//French
			strncpy(psVoice,"chr_f",5);	// same number of letters as "chars"
			FS_FOpenFileRead(psFilename, &hFile, qfalse);		//cache the wav
			if (!hFile) {
				strcpy(&psFilename[iNameStrlen-3],"mp3");		//not there try mp3
				FS_FOpenFileRead(psFilename, &hFile, qfalse);	//cache the mp3
			}
			if (hFile) {
				FS_FCloseFile(hFile);
			}
			strcpy(&psFilename[iNameStrlen-3],"wav");	//put it back to wav

			//Spanish
			strncpy(psVoice,"chr_e",5);	// same number of letters as "chars"
			FS_FOpenFileRead(psFilename, &hFile, qfalse);		//cache the wav
			if (!hFile) {
				strcpy(&psFilename[iNameStrlen-3],"mp3");		//not there try mp3
				FS_FOpenFileRead(psFilename, &hFile, qfalse);	//cache the mp3
			}
			if (hFile) {
				FS_FCloseFile(hFile);
			}
			strcpy(&psFilename[iNameStrlen-3],"wav");	//put it back to wav

			strncpy(psVoice,"chars",5);	//put it back to chars
		}

		// account for foreign voices...
		//		

		if (s_language && stricmp("DEUTSCH",s_language->string)==0) {				
			strncpy(psVoice,"chr_d",5);	// same number of letters as "chars"
		}
		else if (s_language && stricmp("FRANCAIS",s_language->string)==0) {				
			strncpy(psVoice,"chr_f",5);	// same number of letters as "chars"
		}
		else if (s_language && stricmp("ESPANOL",s_language->string)==0) {				
			strncpy(psVoice,"chr_e",5);	// same number of letters as "chars"
		}
		else {
			psVoice = NULL;	// use this ptr as a flag as to whether or not we substituted with a foreign version
		}
		return qfalse;
	} else {
		psVoice = strstr(psFilename,"chr_d");
		if (psVoice) {
			strncpy(psVoice,"chars",5);
			return qtrue;
		} else {
			psVoice = strstr(psFilename,"chr_f");
			if (psVoice) {
				strncpy(psVoice,"chars",5);
				return qtrue;
			} else {
				psVoice = strstr(psFilename,"chr_f");
				if (psVoice) {
					strncpy(psVoice,"chars",5);
					return qtrue;
				}
			}
		}
		return qfalse; //has to reach that only if it's not "chars" related sound, 1 - replaced sound
	}
}



//=============================================================================

/*
==============
S_LoadSound

The filename may be different than sfx->name in the case
of a forced fallback of a player specific sound
==============
*/
qboolean gbInsideLoadSound = qfalse;	// important to default to this!!!
static qboolean S_LoadSound_Actual( sfx_t *sfx )
{
	byte		*data;
	short		*samples;
	wavinfo_t	info;
	int			size;
	ALuint		Buffer;

	// player specific sounds are never directly loaded...
	//
	if ( sfx->sSoundName[0] == '*') {
		return qfalse;
	}

	// make up a local filename to try wav/mp3 substitutes...
	//
	char sRootName[MAX_QPATH];
	char sLoadName[MAX_QPATH];

	COM_StripExtension(sfx->sSoundName, sRootName);
	Com_sprintf(sLoadName, MAX_QPATH, "%s.wav", sRootName);

	const char *psExt = &sLoadName[strlen(sLoadName)-4];

	if (!S_LoadSound_FileLoadAndNameAdjuster(sLoadName, &data, &size, strlen(sLoadName)))
	{
		return qfalse;
	}

	SND_TouchSFX(sfx);
	sfx->iLastTimeUsed = Com_Milliseconds()+1;	// why +1? Hmmm, leave it for now I guess	

//=========
	if (strnicmp(psExt,".mp3",4)==0)
	{
		// load MP3 file instead...
		//		
		if (MP3_IsValid(sfx->sSoundName,data, size))
		{
			int iRawPCMDataSize = MP3_GetUnpackedSize(sfx->sSoundName,data,size);
			
			if (MP3Stream_InitFromFile(sfx, data, size, sfx->sSoundName, iRawPCMDataSize + 2304 /* + 1 MP3 frame size, jic */)
				)
			{
//				Com_DPrintf("(Keeping file \"%s\" as MP3)\n",altname);
			}
			else
			{
				// small file, not worth keeping as MP3 since it would increase in size (with MP3 header etc)...
				//
				Com_DPrintf("S_LoadSound: Unpacking MP3 file \"%s\" to wav.\n",sfx->sSoundName);
				//
				// unpack and convert into WAV...
				//
				byte *pbUnpackBuffer = (byte *) Z_Malloc ( iRawPCMDataSize+10 +2304 /* <g> */, TAG_TEMP_WORKSPACE );	// won't return if fails

				int iResultBytes = MP3_UnpackRawPCM( sfx->sSoundName, data, size, pbUnpackBuffer );
				if (iResultBytes!= iRawPCMDataSize){
					Com_Printf("**** MP3 final unpack size %d different to previous value %d\n",iResultBytes,iRawPCMDataSize);
					//assert (iResultBytes == iRawPCMDataSize);
				}

				// fake up a WAV structure so I can use the other post-load sound code such as volume calc for lip-synching
				//
				// (this is a bit crap really, but it lets me drop through into existing code)...
				//
				MP3_FakeUpWAVInfo( sfx->sSoundName, data, size, iResultBytes,
									// these params are all references...
									info.format, info.rate, info.width, info.channels, info.samples, info.dataofs
								);

				S_LoadSound_Finalize(&info,sfx,pbUnpackBuffer);

				// Open AL
				if (s_UseOpenAL)
				{
					// Clear Open AL Error state
					alGetError();

					// Generate AL Buffer
					alGenBuffers(1, &Buffer);
					if (alGetError() == AL_NO_ERROR)
					{
						// Copy audio data to AL Buffer
						alBufferData(Buffer, AL_FORMAT_MONO16, sfx->pSoundData, sfx->iSoundLengthInSamples*2, 22050);
						if (alGetError() == AL_NO_ERROR)
						{
							sfx->Buffer = Buffer;
							Z_Free(sfx->pSoundData);
							sfx->pSoundData = NULL;
						}
					}
				}

				Z_Free(pbUnpackBuffer);
			}
		}
		else
		{
			// MP3_IsValid() will already have printed any errors via Com_Printf at this point...
			//			
			FS_FreeFile (data);
			return qfalse;
		}
	}
	else
	{
		// loading a WAV, presumably...

//=========

		info = GetWavinfo( sfx->sSoundName, data, size );
#ifndef SND_MME
		if ( info.channels != 1 ) {
			Com_Printf ("%s is a stereo wav file\n", sfx->sSoundName);
			FS_FreeFile (data);
			return qfalse;
		}
#endif
#ifdef _DEBUG
		if ( info.width == 1 ) {
			Com_Printf(S_COLOR_YELLOW "WARNING: %s is an 8 bit wav file\n", sfx->sSoundName);
		}

		if ( info.rate != 22050 ) {
			Com_Printf(S_COLOR_YELLOW "WARNING: %s is not a 22kHz wav file\n", sfx->sSoundName);
		}
#endif

		samples = (short *)Z_Malloc(info.samples * sizeof(short) * 2, TAG_TEMP_WORKSPACE);		

		// each of these compression schemes works just fine
		// but the 16bit quality is much nicer and with a local
		// install assured we can rely upon the sound memory
		// manager to do the right thing for us and page
		// sound in as needed
		//
		sfx->eSoundCompressionMethod= ct_16;
		sfx->iSoundLengthInSamples	= info.samples;
		sfx->pSoundData = NULL;
		ResampleSfx( sfx, info.rate, info.width, data + info.dataofs );		

		// Open AL
		if (s_UseOpenAL)
		{
			// Clear Open AL Error State
			alGetError();

			// Generate AL Buffer
			alGenBuffers(1, &Buffer);
			if (alGetError() == AL_NO_ERROR)
			{
				// Copy audio data to AL Buffer
				alBufferData(Buffer, AL_FORMAT_MONO16, sfx->pSoundData, sfx->iSoundLengthInSamples*2, 22050);
				if (alGetError() == AL_NO_ERROR)
				{
					// Store AL Buffer in sfx struct, and release sample data
					sfx->Buffer = Buffer;
					Z_Free(sfx->pSoundData);
					sfx->pSoundData = NULL;
				}
			}
		}

		Z_Free(samples);
	}
	
	FS_FreeFile( data );

	return qtrue;
}


qboolean S_LoadSound( sfx_t *sfx )
{
	gbInsideLoadSound = qtrue;	// !!!!!!!!!!!!!!
		
		qboolean bReturn = S_LoadSound_Actual( sfx );

	gbInsideLoadSound = qfalse;	// !!!!!!!!!!!!!!

	return bReturn;
}



//-----------------------------------//
//	     MME SOUND FILE SYSTEM		 //
//-----------------------------------//

#define	WAV_FORMAT_PCM		1

typedef struct {
	int				channels;
	int				bits;
	int				dataStart;
} wavOpen_t;

static unsigned short readLittleShort(byte *p) {
	unsigned short val;
	val = p[0];
	val += p[1] << 8;
	return val;
}

static unsigned int readLittleLong(byte *p) {
	unsigned int val;
	val = p[0];
	val += p[1] << 8;
	val += p[2] << 16;
	val += p[3] << 24;
	return val;
}

static openSound_t *S_StreamOpen( const char *fileName, int dataSize ) {
	fileHandle_t fileHandle = 0;
	int fileSize = 0;
	openSound_t *open;

	fileSize = FS_FOpenFileRead( fileName, &fileHandle, qtrue );
	if ( fileSize <= 0 || fileHandle <= 0)
		return 0;
	open = (openSound_t *)Z_Malloc( sizeof( openSound_t ) + dataSize, TAG_SND_RAWDATA );	//???
	memset( open, 0, sizeof( openSound_t ) + dataSize);
	open->fileSize = fileSize;
	open->fileHandle = fileHandle;
	return open;
}

static int S_StreamRead( openSound_t *open, int size, void *data ) {
	char *dataWrite = (char *)data;
	int done;
	if (size + open->filePos > open->fileSize)
		size = (open->fileSize - open->filePos);
	if (size <= 0)
		return 0;

	done = 0;
	while( 1  ) {
		if (!open->bufUsed ) {
			int toRead; 
			if ( size >= sizeof( open->buf )) {
				FS_Read( dataWrite + done, size, open->fileHandle );
				open->filePos += size;
				done += size;
				return done;
			}
			toRead = open->fileSize - open->filePos;
			if (toRead <= 0) {
				//wtf error
				return done;
			}
			if (toRead > sizeof( open->buf ))
				toRead = sizeof( open->buf );
			open->bufUsed = FS_Read( open->buf, toRead, open->fileHandle );
			if (open->bufUsed <= 0) {
				open->bufUsed = 0;
				return done;
			}
			open->bufPos = 0;
		} 
		if ( size > open->bufUsed ) {
			Com_Memcpy( dataWrite + done, open->buf + open->bufPos, open->bufUsed );
			open->filePos += open->bufUsed;
			size -= open->bufUsed;
			done += open->bufUsed;
			open->bufUsed = 0;
			continue;
		}
		Com_Memcpy( dataWrite + done, open->buf + open->bufPos, size );
		open->bufPos += size;
		open->bufUsed -= size;
		open->filePos += size;
		done += size;
		break;
	}
	return done;
}

static int S_StreamSeek( openSound_t *open, int pos ) {
	if ( pos < 0)
		pos = 0;
	else if (pos >= open->fileSize)
		pos = open->fileSize;
	open->bufUsed = 0;
	open->filePos = pos;
	return FS_Seek( open->fileHandle, pos, FS_SEEK_SET );
}

static void S_StreamClose( openSound_t *open ) {
	if ( open->fileHandle > 0) {
		FS_FCloseFile( open->fileHandle );
		open->fileHandle = 0;
	}
}

static void S_WavClose( openSound_t *open ) {
	if (!open )
		return;
	S_StreamClose( open );
	return;
}

static int S_WavSeek( struct openSound_s *open, int samples ) {
	wavOpen_t *wav;
	int wantPos;

	if (!open || !open->data )
		return 0;
	wav = (wavOpen_t *)(open->data);
	if (wav->bits == 8)
		wantPos = samples + wav->dataStart;
	else if (wav->bits == 16)
		wantPos = samples * 2 + wav->dataStart;
	S_StreamSeek( open, wantPos );
	return wantPos;
}

static int S_WavRead( openSound_t *open, qboolean stereo, int size, short *data ) {
	short buf[2048];
	int bufSize, i, done;
	wavOpen_t *wav;
	
	if (!open || !open->data )
		return 0;

	wav = (wavOpen_t *)(open->data);
	if ( wav->bits == 16 ) {
		size *= 2;
		bufSize = sizeof( buf );
	} else {
		/* Use half the bufsize for 8bits so we can first convert the buf */
		bufSize = sizeof( buf ) / 2;
	}
	if ( wav->channels == 2)
		size *= 2;

	done = 0;
	while ( size > 0) {
		int read = size > bufSize ? bufSize : size;
		read = S_StreamRead( open, read, buf );
		if (!read)
			break;
		size -= read;
		/* Convert buffer from to 8u to 16s in reverse */
		if (wav->bits == 8) {
			for (i = read - 1;i >= 0 ;i--) 
				buf[i] = (((char *)buf)[i] ^ 0x80) << 8;
			read *= 2;
		}
		/* Stereo wav input */
		if ( wav->channels == 2) {
			/* Stereo output, just copy */
			if (stereo) {
				Com_Memcpy( &data[done * 2], buf, read );
				done += read / 4;
			/* Mono output sum up and /2 for output */
			} else {
				read /= 4;
				for ( i = 0;i < read;i++)  {
					data[done++] = (buf[i * 2 + 0] + buf[i*2 + 1]) >> 1;
				}
			}
		/* Mono wav input */
		} else {
			/* Stereo output, both channels the same */
			if (stereo) {
				read /= 2;
				for ( i = 0;i < read;i++)  {
					data[done*2 + 0] = data[done*2 + 1] = buf[i];
					done++;
				}
			/* Mono output just copy */
			} else {
				Com_Memcpy( &data[done], buf, read );
				done += read / 2;
			}
		}
	}

	return done;
}

static int S_WavFindChunk( openSound_t *open, const char *chunk ) {
	char data[8];
	int readSize;
	while (1) {
		readSize = S_StreamRead( open, 8, data );
		if (readSize != 8) {
			return 0;
		}
		readSize = readLittleLong( (byte *)data + 4 ) ;		
		if (!memcmp(data + 0x00, chunk, 4))
			break;
		S_StreamSeek( open, open->filePos + readSize );
	}
	return readSize;
}

static openSound_t * S_WavOpen( const char *fileName ) {
	openSound_t *open;
	wavOpen_t *wav;
	byte buf[16];
	int	readSize;
	int format;
	int riffSize, fmtSize, dataSize;

	// load it in
	open = S_StreamOpen( fileName, sizeof( wavOpen_t ));
	if (!open) {
		if (!doNotYell)
			Com_Printf("WavOpen:File %s failed to open\n", fileName);
		return 0;
	}
	open->read = S_WavRead;
	open->close = S_WavClose;
	open->seek = S_WavSeek;
	wav = (wavOpen_t *)open->data;
	riffSize = S_WavFindChunk( open, "RIFF" );
	if (!riffSize ) {
		Com_Printf("WavOpen:File %s failed to find RIFF chunk\n", fileName);
		S_SoundClose( open );
		return 0;
	}
	readSize = S_StreamRead( open, 4, buf );
	if (!readSize || memcmp( buf, "WAVE", 4)) {
		Com_Printf("WavOpen:File %s failed to find RIFF WAVE type\n", fileName);
		S_SoundClose( open );
		return 0;
	}
	fmtSize = S_WavFindChunk( open, "fmt " );
	if (!fmtSize || fmtSize < 16 ) {
		Com_Printf("WavOpen:File %s failed to find fmt chunk\n", fileName);
		S_SoundClose( open );
		return 0;
	}
	readSize = S_StreamRead( open, 16, buf );
	if (readSize < 16) {
		Com_Printf("WavOpen:unexpected end of file while reading fmt chunk\n", fileName);
		S_SoundClose( open );
		return 0;
	}
	format = readLittleShort( buf + 0x00 );
	wav->channels = readLittleShort( buf + 0x2 );
	open->rate = readLittleLong( buf + 0x4 );
	wav->bits = readLittleShort( buf + 0xe );
	if (format != 1) {
		Com_Printf("WavOpen:Microsoft PCM format only, %s\n", fileName);
		S_SoundClose( open );
		return 0;
	}
	if ((wav->channels != 1) && (wav->channels != 2)) {
		Com_Printf("WavOpen:can't open file with %d channels, %s\n", wav->channels, fileName);
		S_SoundClose( open );
		return 0;
	}
	/* Skip the remainder of the format if any */
	fmtSize -= readSize;
	if (fmtSize)
		S_StreamSeek( open, open->filePos + fmtSize );
	/* Scan the cunks till you find the "data" chunk and find sample count */
	/* Finish reading the format if needed */
	dataSize = S_WavFindChunk( open, "data" );
	if (!dataSize) {
		Com_Printf("WavOpen:failed %s no data\n", fileName);
		S_SoundClose( open );
		return 0;
	}
	/* Try to find the data chunk */
	/* Read the chunksize of the data chunk */
	open->totalSamples = dataSize / (wav->bits / 8);
	wav->dataStart = open->filePos;
	return open;
}

#define HAVE_LIBMAD

//=============================================================================

#ifdef HAVE_LIBMAD 
#include <mad.h>
//#pragma comment (lib, "libmad.lib")
//#pragma comment (lib, "libmadd.lib")
#define MP3_SEEKINTERVAL 16 
#define MP3_SEEKMAX 4096

typedef struct {
	struct	mad_stream stream;
	struct	mad_frame frame;
	struct	mad_synth synth;
	char	buf[8 * 1024];
	int		pcmLeft;
	long	seekStart[MP3_SEEKMAX];
	int		seekCount;
	int		frameCount, frameSamples;
} mp3Open_t;


static int S_MP3Fill( openSound_t *open ) {
	int bufLeft, readSize;
	unsigned char *readStart;
	mp3Open_t *mp3;

	if (!open || !open->data)
		return 0;

	mp3 = (mp3Open_t *) open->data;

	if (mp3->stream.next_frame != NULL) {
		bufLeft = mp3->stream.bufend - mp3->stream.next_frame;
		memmove (mp3->buf, mp3->stream.next_frame, bufLeft);
		readStart = (unsigned char *)mp3->buf + bufLeft;
		readSize = sizeof(mp3->buf) - bufLeft;
	} else {
		readStart = (unsigned char *)mp3->buf;
		readSize = sizeof(mp3->buf);
		bufLeft = 0;
	}

	readSize = S_StreamRead( open, readSize, readStart );
	if (readSize <= 0)
		return 0;
	mad_stream_buffer(&mp3->stream, (unsigned char *)mp3->buf, readSize + bufLeft);
	mp3->stream.error = (mad_error)0;
	return readSize;
}

static inline int S_MadSample (mad_fixed_t sample) {
  /* round */
  sample += (1L << (MAD_F_FRACBITS - 16));

  /* clip */
  if (sample >= MAD_F_ONE)
    sample = MAD_F_ONE - 1;
  else if (sample < -MAD_F_ONE)
    sample = -MAD_F_ONE;

  /* quantize */
  return sample >> (MAD_F_FRACBITS + 1 - 16);
}

static int S_MP3Read( openSound_t *open, qboolean stereo, int size, short *data ) {
	mp3Open_t *mp3;
	int done;

	if (!open || !open->data )
		return 0;
	mp3 = (mp3Open_t *)(open->data);
	done = 0;
	while ( size > 0) {
		/* Have we got any pcm data waiting */
		if (mp3->pcmLeft) {
			int i, todo;
			if ( mp3->pcmLeft <= size ) {
				todo = mp3->pcmLeft;
			} else {
				todo = size;
			}
			if ( data ) {
				const struct mad_pcm *pcm = &mp3->synth.pcm;
				if ( pcm->channels == 1 ) {
					const mad_fixed_t *left = &pcm->samples[0][ pcm->length - mp3->pcmLeft ];
					if ( stereo ) {
						for (i = 0;i<todo;i++) {
							data[0] = data[1] = S_MadSample( left[i]  );
							data += 2;
						}
					} else{
						for (i = 0;i<todo;i++) {
							*data++ = S_MadSample( left[i]  );
						}
					}
				} else if ( pcm->channels == 2 ) {
					const mad_fixed_t *left =  &pcm->samples[0][ pcm->length - mp3->pcmLeft ];
					const mad_fixed_t *right = &pcm->samples[1][ pcm->length - mp3->pcmLeft ];
					if ( stereo ) {
						for (i = 0;i<todo;i++) {
							data[0] = S_MadSample( left[i] );
							data[1] = S_MadSample( right[i] );
							data+=2;
						}
					} else {
						for (i = 0;i<todo;i++) {
							int addSample;
							addSample = S_MadSample( left[i] );
							addSample += S_MadSample( right[i] );
							*data++ = addSample >> 1;
						}
					}
				} else {
					return done;
				}
			} 
			done += todo;
			size -= todo;
			mp3->pcmLeft -= todo;
			if ( mp3->pcmLeft )
				return done;
		}
		/* is there still pcm data left then we must have reached size */
		if (mp3->stream.next_frame == NULL || mp3->stream.error == MAD_ERROR_BUFLEN) {
			if (!S_MP3Fill( open ))
				return done;
		}
		if (mad_frame_decode (&mp3->frame, &mp3->stream)) {
			if (MAD_RECOVERABLE(mp3->stream.error)) {
				continue;
			} else if (mp3->stream.error == MAD_ERROR_BUFLEN) {
				continue;
			} else {
				return done;
			}
		}
		/* Sound parameters. */
		if (mp3->frame.header.samplerate != open->rate ) {
			Com_Printf( "mp3read, samplerate change?!?!?!\n");
			return done;
		}
		mad_synth_frame (&mp3->synth, &mp3->frame);
		mad_stream_sync (&mp3->stream);
		mp3->pcmLeft = mp3->synth.pcm.length;
	}
	return done;
}

static int S_MP3Seek( struct openSound_s *open, int samples ) {
	mp3Open_t *mp3;
	int index, frame, seek;

	if (!open || !open->data )
		return 0;
	mp3 = (mp3Open_t *)(open->data);
	frame = samples / mp3->frameSamples;
	if ( frame >= mp3->frameCount) 
		frame = mp3->frameCount - 1;
	index = frame / MP3_SEEKINTERVAL;
	frame %= MP3_SEEKINTERVAL;
	if ( index >= mp3->seekCount) 
		index = mp3->seekCount - 1;
	seek = index * MP3_SEEKINTERVAL * mp3->frameSamples;
	samples -= seek;
	S_StreamSeek( open, mp3->seekStart[ index ] );

	//mad_frame_mute (&mp3->frame);
	mad_frame_init( &mp3->frame );
	//mad_synth_mute (&mp3->synth);
	mad_synth_init( &mp3->synth );
	mp3->stream.sync = 0;
	mp3->stream.next_frame = NULL;
	mp3->pcmLeft = 0;
	seek += S_MP3Read( open, qfalse, samples, 0 );
	return seek;
}

static void S_MP3Close( openSound_t *open) {
	mp3Open_t *mp3;
	if (!open || !open->data )
		return;
	mp3 = (mp3Open_t *)(open->data);
	mad_stream_finish (&mp3->stream);
	mad_frame_finish (&mp3->frame);
	mad_synth_finish (&mp3->synth);
}

static openSound_t *S_MP3Open( const char *fileName ) {
	mp3Open_t *mp3;
	openSound_t *open;
	struct mad_header header;
	mad_timer_t duration;

	open = S_StreamOpen( fileName, sizeof( mp3Open_t ) );
	if (!open) {
		if (!doNotYell)
			Com_Printf("MP3Open:File %s failed to open\n", fileName);
		return 0;
	}
	mp3 = (mp3Open_t *)open->data;
	mad_header_init (&header);
	mad_stream_init (&mp3->stream);
	mad_frame_init (&mp3->frame);
	mad_synth_init (&mp3->synth);
	mad_stream_options (&mp3->stream, MAD_OPTION_IGNORECRC);
	open->read = S_MP3Read;
	open->seek = S_MP3Seek;
	open->close = S_MP3Close;
	/* Determine file length in samples */
	duration = mad_timer_zero;
	while (1) {
		if (mp3->stream.buffer == NULL || mp3->stream.error == MAD_ERROR_BUFLEN) {
			if (!S_MP3Fill( open ))
				break;
		}
		if (mad_header_decode(&header, &mp3->stream) == -1) {
			if (MAD_RECOVERABLE(mp3->stream.error))
				continue;
			else if (mp3->stream.error == MAD_ERROR_BUFLEN)
				continue;
			else {
				/* Another error, let's just stop! */
				break;
			}
		}
		mad_timer_add (&duration, header.duration);
		/* Should probably just count the amount of frames and do a multiply */
		if ( 0 || mp3->frameCount % MP3_SEEKINTERVAL == 0 ) {
			if (mp3->seekCount < MP3_SEEKMAX) {
				int pos = open->filePos;
				if ( mp3->stream.this_frame ) {
					pos -= mp3->stream.bufend - mp3->stream.this_frame;
				}
				mp3->seekStart[mp3->seekCount++] = pos;
			}
		}
		mp3->frameCount++;
	}
	/* Reset to the beginning and finalize setting of values */
	S_StreamSeek( open, 0 );
	mp3->frameSamples = 32 * MAD_NSBSAMPLES( &header );
	open->totalSamples = mp3->frameSamples * mp3->frameCount;
	open->rate = header.samplerate;
	return open;
}

#endif

//ZTM's super replacer
static void S_SoundChangeExt(char *fileName, const char *ext) {
	char *p = strchr(fileName, '.');			/* find first '.' */
	if (p != NULL) {							/* found '.' */
		Q_strncpyz(p, ext, strlen(ext) + 1);	/* change ext */
	} else {									/* should never happen */
		strcat(fileName, ext);					/* has no ext, so just add it */
	}
}

qboolean doNotYell;

openSound_t *S_SoundOpen(const char *constFileName) {
	const char *fileExt;
	char temp[MAX_QPATH];
	char *fileName;
	openSound_t *open;
	qboolean wasHere = qfalse;

#ifdef FINAL_BUILD
	doNotYell = qtrue;
#else
	doNotYell = qfalse;
#endif

	Q_strncpyz(temp, constFileName, sizeof(temp));
	fileName = temp;

	if (!fileName || !fileName[0]) {
		Com_Printf("SoundOpen:Filename is empty\n");
		return 0;
	}

	// do we still need language switcher
	// and sound/mp_generic_female/sound -> sound/chars/mp_generic_female/misc/sound (or male) converter?

	/* switch language: /chars/ -> /chr_f/, /chr_d/, /chr_e/ */
	if (Q_stricmp(s_language->string, "english"))
		S_SwitchLang(fileName);

	/* sound/mp_generic_female/sound -> sound/chars/mp_generic_female/misc/sound */
	char *match = strstr(fileName, "sound/mp_generic_female");
	if (match) {
		char out[MAX_QPATH];
		Q_strncpyz(out, fileName, MAX_QPATH);
		char *pos = strstr(match, "le/");
		pos = strchr(pos, '/');
		Q_strncpyz(out, "sound/chars/mp_generic_female/misc", MAX_QPATH);
		Q_strcat(out, sizeof(out), pos);
		fileName = out;
	} else {
	/* or sound/mp_generic_male/sound -> sound/chars/mp_generic_male/misc/sound */
		match = strstr(fileName, "sound/mp_generic_male");
		if (match) {
			char out[MAX_QPATH];
			Q_strncpyz(out, fileName, MAX_QPATH);
			char *pos = strstr(match, "le/");
			pos = strchr(pos, '/');
			Q_strncpyz(out, "sound/chars/mp_generic_male/misc", MAX_QPATH);
			Q_strcat(out, sizeof(out), pos);
			fileName = out;
		}
	}

	fileExt = Q_strrchr(fileName, '.');
	if (!fileExt) {
		strcat(fileName, ".wav");
	} else if (!Q_stricmp(fileExt, ".mp3")) { //we want to start seeking .wav at first
		S_SoundChangeExt(fileName, ".wav");
	} else if (Q_stricmp(fileExt, ".wav")
#ifdef HAVE_LIBMAD
		&& Q_stricmp(fileExt, ".mp3")
#endif
		) {
		Com_Printf("SoundOpen:File %s has unknown extension %s\n", fileName, fileExt );
		return 0;
	}

tryAgainThisSound:
	open = S_WavOpen(fileName);
	if (open) {
		return open;
	} else {
#ifdef HAVE_LIBMAD
		S_SoundChangeExt(fileName, ".mp3");
		open = S_MP3Open(fileName);
		if (open) {
			return open;
		} else
#endif
		if (Q_stricmp(s_language->string, "english") && !wasHere) {
			if (S_SwitchLang(fileName)) {
				wasHere = qtrue;
				if (!doNotYell)
					Com_Printf("SoundOpen:File %s doesn't exist in %s, retrying with english\n", fileName, s_language->string );
#ifdef HAVE_LIBMAD
				//set back to wav
				S_SoundChangeExt(fileName, ".wav");
#endif
				goto tryAgainThisSound;
			}
		}
	}
	Com_Printf("S_SoundOpen:File %s failed to open\n", constFileName);
	return 0;
}

int S_SoundRead( openSound_t *open, qboolean stereo, int samples, short *data ){
	int read ;
	if (!open)
		return 0;
	if ( samples + open->doneSamples > open->totalSamples )
		samples = open->totalSamples - open->doneSamples;
	if ( samples <= 0)
		return 0;
	read = open->read( open, stereo, samples, data );
    open->doneSamples += read;
	return read;
}

int S_SoundSeek( openSound_t *open, int samples ) {
	if ( samples > open->totalSamples)
		samples = open->totalSamples;
	if ( samples < 0)
		samples = 0;
	open->doneSamples = open->seek( open, samples );
	return open->doneSamples;
}

void S_SoundClose( openSound_t *open ) {
	open->close( open );
	S_StreamClose( open );
	Z_Free( open );
}
