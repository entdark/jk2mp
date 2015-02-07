
/*****************************************************************************
 * name:		snd_dma.c
 *
 * desc:		main control for any streaming sound output device
 *
 * $Archive: /MissionPack/code/client/snd_dma.c $
 * $Author: Mrelusive $ 
 * $Revision: 117 $
 * $Modtime: 6/06/01 2:35p $
 * $Date: 6/06/01 7:11p $
 *
 *****************************************************************************/

#include "snd_local.h"
#include "snd_mix.h"
#include "client.h"

void S_Play_f(void);
void S_SoundList_f(void);
static void S_Music_f(void);

void S_StopAllSounds(void);
extern int RE_RegisterMedia_GetLevel(void);


// =======================================================================
// Internal sound data & structures
// =======================================================================

// only begin attenuating sound volumes when outside the FULLVOLUME range

// MAX_SFX may be larger than MAX_SOUNDS because
// of custom player sounds

#define		SFX_HASH		256

sfxEntry_t		sfxEntries[SFX_SOUNDS];
sfxEntry_t		*sfxHash[SFX_HASH];
int				sfxEntryCount;

backgroundSound_t s_background;
channelQueue_t	s_channelQueue[MAX_SNDQUEUE];
int				s_channelQueueCount;
entitySound_t	s_entitySounds[MAX_GENTITIES];
loopQueue_t		s_loopQueue[MAX_LOOPQUEUE];
int				s_loopQueueCount;

static int	s_soundStarted;
qboolean	s_soundMuted;
int			s_listenNumber;
vec3_t		s_listenOrigin;
vec3_t		s_listenVelocity;
vec3_t		s_listenAxis[3];
float		s_playScale;
qboolean	s_hadSpatialize;
qboolean	s_underWater;

cvar_t		*s_volume;
cvar_t		*s_volumeVoice;
cvar_t		*s_musicVolume;
cvar_t		*s_language;

cvar_t		*s_doppler;
cvar_t		*s_dopplerSpeed;
cvar_t		*s_dopplerFactor;

cvar_t		*s_timescale;
cvar_t		*s_forceScale;
cvar_t		*s_attenuate;


// ====================================================================
// User-setable variables
// ====================================================================

void S_SoundInfo_f(void) {	
	Com_Printf("----- Sound Info -----\n" );
	if (!s_soundStarted) {
		Com_Printf ("sound system not started\n");
	} else {
		if ( s_soundMuted ) {
			Com_Printf ("sound system is muted\n");
		}

		Com_Printf("%5d stereo\n", dma.channels - 1);
		Com_Printf("%5d samples\n", dma.samples);
		Com_Printf("%5d samplebits\n", dma.samplebits);
		Com_Printf("%5d submission_chunk\n", dma.submission_chunk);
		Com_Printf("%5d speed\n", dma.speed);
		Com_Printf("0x%x dma buffer\n", dma.buffer);
		if ( s_background.playing ) {
			Com_Printf("Background file: %s\n", s_background.loopName );
		} else {
			Com_Printf("No background file.\n" );
		}

	}
	Com_Printf("----------------------\n" );
}



/*
================
S_Init
================
*/
void S_Init( void ) {
	cvar_t	*cv;
	Com_Printf("\n------- sound initialization -------\n");

	s_volume = Cvar_Get ("s_volume", "0.8", CVAR_ARCHIVE);
	s_volumeVoice= Cvar_Get ("s_volumeVoice", "0.8", CVAR_ARCHIVE);
	s_musicVolume = Cvar_Get ("s_musicvolume", "0.5", CVAR_ARCHIVE);

	s_language = Cvar_Get("s_language","english",CVAR_ARCHIVE | CVAR_NORESTART);

	s_doppler = Cvar_Get ("s_doppler", "1", CVAR_ARCHIVE);
	s_dopplerSpeed = Cvar_Get ("s_dopplerSpeed", "4000", CVAR_ARCHIVE);
	s_dopplerFactor = Cvar_Get ("s_dopplerFactor", "1", CVAR_ARCHIVE);
	s_timescale = Cvar_Get ("s_timescale", "1", CVAR_ARCHIVE);
	s_attenuate = Cvar_Get ("s_attenuate", "1.7", CVAR_ARCHIVE);
	s_playScale = 1.0f;
	s_forceScale = Cvar_Get ("s_forceScale", "0", CVAR_TEMP);

	cv = Cvar_Get ("s_initsound", "1", 0);
	if ( !cv->integer ) {
		Com_Printf ("not initializing.\n");
		Com_Printf("------------------------------------\n");
		return;
	}

	Cmd_AddCommand("play", S_Play_f);
	Cmd_AddCommand("music", S_Music_f);
	Cmd_AddCommand("soundlist", S_SoundList_f);
	Cmd_AddCommand("soundinfo", S_SoundInfo_f);
	Cmd_AddCommand("soundstop", S_StopAllSounds);

	Com_Printf("------------------------------------\n");

	S_DMAInit();
	
	s_soundStarted = 1;
	s_soundMuted = qtrue;
	S_StopAllSounds ();
	S_SoundInfo_f();
	s_underWater = qfalse;
}


// =======================================================================
// Shutdown sound engine
// =======================================================================

void S_Shutdown( void ) {
	if ( !s_soundStarted ) {
		return;
	}

	SNDDMA_Shutdown();

	s_soundStarted = 0;

    Cmd_RemoveCommand("play");
	Cmd_RemoveCommand("music");
	Cmd_RemoveCommand("stopsound");
	Cmd_RemoveCommand("soundlist");
	Cmd_RemoveCommand("soundinfo");
}


/*
===================
S_DisableSounds

Disables sounds until the next S_BeginRegistration.
This is called when the hunk is cleared and the sounds
are no longer valid.
===================
*/
void S_DisableSounds( void ) {
	S_StopAllSounds();
	s_soundMuted = qtrue;
}

/*
=====================
S_BeginRegistration

=====================
*/
void S_BeginRegistration( void ) {
	s_soundMuted = qfalse;		// we can play again

	/* Skip the first sound for 0 handle */
	Com_Memset(sfxHash, 0, sizeof(sfxHash));
	Com_Memset(sfxEntries, 0, sizeof(sfxEntries));
	sfxEntryCount = 1;
	
	S_MixInit();
}



/*
==================
S_RegisterSound

Creates a default buzz sound if the file can't be loaded
==================
*/
sfxHandle_t	S_RegisterSound( const char *name) {
	const char *fileExt;
	char fileName[MAX_QPATH];
	int len, lenExt;
	unsigned long hashIndex;
	sfxEntry_t *entry;

	if (!s_soundStarted) {
		return 0;
	}

	if ( !name ) {
		Com_Error (ERR_FATAL, "S_RegisterSound: null name pointer\n");
	}

	if (!name[0]) {
		Com_Printf( "S_RegisterSound: empty name\n");
		return 0;
	}

	fileExt = strchr(name, '.');
	lenExt = 0;
	if (!fileExt) {
		fileExt = ".mp3";
		lenExt = 4;
	}
	hashIndex = 0;
	for ( len = 0; name[0] && len < (sizeof( fileName ) - 1); name ++, len++ ) {
		char c = tolower( name [0] );
		hashIndex = (hashIndex << 5 ) ^ (hashIndex >> 27) ^ c;
		fileName[len] = c;
	}
	for ( ; fileExt[0] && len < (sizeof( fileName ) - 1) && lenExt > 0; lenExt--, fileExt++, len++ ) {
		char c = tolower( fileExt [0] );
		hashIndex = (hashIndex << 5 ) ^ (hashIndex >> 27) ^ c;
		fileName[len] = c;
	}
	hashIndex = ( hashIndex ^ (hashIndex >> 10) ^ (hashIndex >> 20) ) & (SFX_HASH - 1);
	fileName[len] = 0;

	if (!S_FileExists(fileName)) {
		return 0;
	}
	entry = sfxHash[ hashIndex ];
	while (entry) {
		if (!strcmp( entry->name, fileName ))
			return entry - sfxEntries;
		entry = entry->next;
	}
	if (sfxEntryCount >= SFX_SOUNDS) {
		Com_Printf( "SFX:Sound max %d reached\n", SFX_SOUNDS );
		return 0;
	}
	entry = sfxEntries + sfxEntryCount;
	Com_Memcpy( entry->name, fileName, len + 1);
	entry->next = sfxHash[hashIndex];
	sfxHash[hashIndex] = entry;
	return sfxEntryCount++;
}

/*
====================
S_StopSound

Stops sound on specified channel for specified entity with specified sfx.
====================
*/
void S_StopSound(int entityNum, int entchannel, sfxHandle_t sfxHandle ) {
	S_DMAStopSound(entityNum, entchannel, sfxHandle);
	S_MMEStopSound(entityNum, entchannel, sfxHandle);
}


// =======================================================================
// Start a sound effect
// =======================================================================

/*
====================
S_StartSound

Validates the parms and ques the sound up
if pos is NULL, the sound will be dynamically sourced from the entity
Entchannel 0 will never override a playing sound
====================
*/
void S_StartSound(vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfxHandle ) {
	channelQueue_t *q;

	if ( !s_soundStarted || s_soundMuted ) {
		return;
	}
	if ( !origin && ( entityNum < 0 || entityNum > MAX_GENTITIES ) ) {
		Com_Error( ERR_DROP, "S_StartSound: bad entitynum %i", entityNum );
	}
	if ( s_channelQueueCount >= MAX_SNDQUEUE ) {
		Com_Printf( "S_StartSound: Queue overflow, dropping\n");
		return;
	}
	if ( sfxHandle <= 0 || sfxHandle >= sfxEntryCount) {
		Com_DPrintf( "S_StartSound: Illegal sfxhandle %d\n", sfxHandle );
		return;
	}

	q = s_channelQueue + s_channelQueueCount++;
	q->entChan = entchannel;
	q->entNum = entityNum;
	q->handle = sfxHandle;
	if (origin) {
		VectorCopy( origin, q->origin );
		q->hasOrigin = qtrue;
	} else {
		q->hasOrigin = qfalse;
	}
}


/*
==================
S_StartLocalSound
==================
*/
void S_StartLocalSound( sfxHandle_t sfxHandle, int channelNum ) {
	if ( !s_soundStarted || s_soundMuted ) {
		return;
	}

	S_StartSound (NULL, s_listenNumber, channelNum, sfxHandle );
}


/*
==================
S_ClearSoundBuffer

If we are about to perform file access, clear the buffer
so sound doesn't stutter.
==================
*/
void S_ClearSoundBuffer( void ) {
	if (!s_soundStarted)
		return;

	s_playScale = 1.0f;
	// stop looping sounds
	Com_Memset(s_entitySounds, 0, sizeof(s_entitySounds));
	VectorClear(s_listenOrigin);
	// Signal the real sound mixer to stop any sounds
	S_DMAClearBuffer();
}


/*
==================
S_StopAllSounds
==================
*/
void S_StopAllSounds(void) {
	if ( !s_soundStarted ) {
		return;
	}

	// stop the background music
	S_StopBackgroundTrack();
	S_ClearSoundBuffer ();
}

/*
==============================================================

continuous looping sounds are added each frame

==============================================================
*/

void S_StopLoopingSound(const void *parent) {
	loopQueue_t *lq;
	int queueLeft;

	for (queueLeft = s_loopQueueCount, lq = s_loopQueue;queueLeft > 0;queueLeft--, lq++ ) {
		/* Found the same parent */
		if ( lq && lq->parent == parent ) {
			lq->handle = 0;
			VectorClear( lq->origin );
			VectorClear( lq->velocity );
			lq->volume = 0;
			lq->parent = 0;
		}
	}
}

/*
==================
S_ClearLoopingSounds

==================
*/
void S_ClearLoopingSounds( qboolean killall ) {
	s_loopQueueCount = 0;
}

/*
==================
S_AddLoopingSound

Called during entity generation for a frame
Include velocity in case I get around to doing doppler...
==================
*/
void S_AddLoopingSound( const void *parent, int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfxHandle, int volume ) {
	loopQueue_t *lq;

	if ( !s_soundStarted || s_soundMuted ) {
		return;
	}

	if ( sfxHandle <= 0 || sfxHandle >= sfxEntryCount) {
		Com_DPrintf( "S_AddLoopingSound: Illegal sfxhandle %d\n", sfxHandle );
		return;
	}
	
	if ( s_loopQueueCount >= MAX_LOOPQUEUE ) {
		Com_Printf( "S_AddLoopingSound: Queue overflow %d\n", sfxHandle );
		return;
	}

	lq = s_loopQueue + s_loopQueueCount++;

	lq->handle = sfxHandle;
	VectorCopy( origin, lq->origin );
	VectorCopy( velocity, lq->velocity );
	lq->volume = volume;
	lq->parent = parent;
}



/*
============
S_RawSamples

Music streaming
============
*/
void S_RawSamples( int samples, int rate, int width, int s_channels, const byte *data, float volume ) {

}

//=============================================================================

/*
=====================
S_UpdateEntityPosition

let the sound system know where an entity currently is
======================
*/
void S_UpdateEntityPosition( int entityNum, const vec3_t origin ) {
	if ( entityNum < 0 || entityNum > MAX_GENTITIES ) {
		Com_Error( ERR_DROP, "S_UpdateEntityPosition: bad entitynum %i", entityNum );
	}
	VectorCopy( origin, s_entitySounds[entityNum].origin );
}


/*
============
S_Respatialize

Change the volumes of all the playing sounds for changes in their positions
============
*/
void S_Respatialize( int entityNum, const vec3_t head, vec3_t axis[3], int inwater ) {
	if (s_listenNumber == entityNum ) {
		vec3_t delta;
		float deltaTime;
		
		VectorSubtract( head, s_listenOrigin, delta );
		deltaTime = cls.frametime * 0.001f;
		VectorScale( delta, deltaTime, s_listenVelocity );
	} else {
		VectorClear( s_listenVelocity );
	}
	s_listenNumber = entityNum;
	VectorCopy(head, s_listenOrigin);
	VectorCopy(axis[0], s_listenAxis[0]);
	VectorCopy(axis[1], s_listenAxis[1]);
	VectorCopy(axis[2], s_listenAxis[2]);

	s_hadSpatialize = qtrue;
	s_playScale *= com_timescale->value;

	s_underWater = (qboolean)inwater;
}

/*
============
S_Update

Called once each time through the main loop
============
*/
void S_Update( void ) {
	float scale;
	if ( !s_soundStarted || s_soundMuted ) {
		Com_DPrintf ("not started or muted\n");
		s_channelQueueCount = 0;
		s_loopQueueCount = 0;
		return;
	}
	if (s_timescale->integer) {
		scale = (s_hadSpatialize) ? s_playScale : com_timescale->value;
		s_hadSpatialize = qfalse;
	} else {
		scale = 1.0f;
	}
	S_DMA_Update( scale );
	S_MMEUpdate( scale );

	s_channelQueueCount = 0;
	s_loopQueueCount = 0;
	s_background.reload = qfalse;
	s_underWater = qfalse;
}

/*
===============================================================================

console functions

===============================================================================
*/

void S_Play_f( void ) {
	int 		i;
	sfxHandle_t	h;
	char		name[256];
	
	i = 1;
	while ( i<Cmd_Argc() ) {
		if ( !Q_strrchr(Cmd_Argv(i), '.') ) {
			Com_sprintf( name, sizeof(name), "%s.wav", Cmd_Argv(1) );
		} else {
			Q_strncpyz( name, Cmd_Argv(i), sizeof(name) );
		}
		h = S_RegisterSound( name );
		if( h ) {
			S_StartLocalSound( h, CHAN_LOCAL_SOUND );
		}
		i++;
	}
}

static void S_Music_f( void ) {
	int		c;

	if ( s_background.override ) {
		Com_Printf( "Can't start music in mme player mode" );
		return;
	}
	c = Cmd_Argc();

	if ( c == 2 ) {
		S_StartBackgroundTrack( Cmd_Argv(1), Cmd_Argv(1), qfalse );
	} else if ( c == 3 ) {
		S_StartBackgroundTrack( Cmd_Argv(1), Cmd_Argv(2), qfalse );		
	} else {
		Com_Printf ("music <musicfile> [loopfile]\n");
		return;
	}
}

void S_SoundList_f( void ) {

}


/*
===============================================================================

background music functions

===============================================================================
*/

// Basic logic here is to see if the intro file specified actually exists, and if so, then it's not dynamic music,
//	but if it doesn't, and it DOES exist by strcat()ing "_fast" and "_slow" (.mp3) onto 2 copies of it, then it's dynamic.
//
// In either case, open it if it exits, and just set some vars that'll inhibit dynamicness if needed.
//
void S_StartBackgroundTrack( const char *intro, const char *loop, qboolean bReturnWithoutStarting ) {
	qboolean soundExists;
	if ( !intro || !intro[0] || s_background.override )
		return;
	if ( !loop || !loop[0] ) 
		loop = intro;

	Com_DPrintf( "S_StartBackgroundTrack( %s, %s )\n", intro, loop );

	Q_strncpyz( s_background.startName, intro, sizeof( s_background.startName ));
	Q_strncpyz( s_background.loopName, loop, sizeof( s_background.loopName ));
	/* S_FileExists sets correct extension */
	soundExists = S_FileExists(s_background.startName);
	soundExists = (qboolean)(S_FileExists(s_background.loopName) || soundExists);
	if (soundExists) {
		s_background.playing = qtrue;
		s_background.reload = qtrue;
	}
}

void S_StopBackgroundTrack( void ) {
	if (!s_background.override)
		s_background.playing = qfalse;
}

void S_UpdateScale(float scale) {
	if (s_timescale->integer) {
		if (s_forceScale->value > 0) {
			s_playScale = s_forceScale->value;
		} else {
			s_playScale = scale;
		}
		if (s_playScale > 5)
			s_playScale = 5;
	} else {
		s_playScale = 1;
	}
}
