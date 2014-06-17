// Copyright (C) 2009 Sjoerd van der Berg ( harekiet @ gmail.com )

#include "bg_demos.h"
#include "cg_local.h"
#include "cg_demos_math.h"

#define LOGLINES 8

typedef enum {
	editNone,
	editCamera,
	editChase,
	editLine,
	editDof,
	editLast,
} demoEditType_t;

typedef enum {
	viewCamera,
	viewChase,
	viewLast
} demoViewType_t;

typedef enum {
	findNone,
	findObituary,
	findDirect,
} demofindType_t;

typedef struct demoLinePoint_s {
	struct			demoLinePoint_s *next, *prev;
	int				time, demoTime;
} demoLinePoint_t;

typedef struct demoCameraPoint_s {
	struct			demoCameraPoint_s *next, *prev;
	vec3_t			origin, angles;
	float			fov;
	int				time, flags;
	float			len, anglesLen;
} demoCameraPoint_t;

typedef struct demoChasePoint_s {
	struct			demoChasePoint_s *next, *prev;
	vec_t			distance;
	vec3_t			angles;
	int				target;
	vec3_t			origin;
	int				time;
	float			len;
} demoChasePoint_t;

typedef struct demoDofPoint_s {
	struct			demoDofPoint_s *next, *prev;
	float			focus, radius;
	int				time;
} demoDofPoint_t;

typedef struct {
	char lines[LOGLINES][1024];
	int	 times[LOGLINES];
	int	 lastline;
} demoLog_t;

typedef struct demoMain_s {
	int				serverTime;
	float			serverDeltaTime;
	struct {
		int			start, end;
		qboolean	locked;
		float		speed;
		int			offset;
		float		timeShift;
		int			shiftWarn;
		int			time;
		demoLinePoint_t *points;
	} line;
	struct {
		int			start;
		float		range;
		int			index, total;
		int			lineDelay;
	} loop;
	struct {
		int			start, end;
		qboolean	locked;
		int			target;
		vec3_t		angles, origin, velocity;
		vec_t		distance;
		float		timeShift;
		int			shiftWarn;
		demoChasePoint_t *points;
		centity_t	*cent;
	} chase;
	struct {
		int			start, end;
		int			target, flags;
		int			shiftWarn;
		float		timeShift;
		float		fov;
		qboolean	locked;
		vec3_t		angles, origin, velocity;
		demoCameraPoint_t *points;
		posInterpolate_t	smoothPos;
		angleInterpolate_t	smoothAngles;
	} camera;
	struct {
		int			start, end;
		int			target;
		int			shiftWarn;
		float		timeShift;
		float		focus, radius;
		qboolean	locked;
		demoDofPoint_t *points;
	} dof;
	struct {
		int			time;
		int			oldTime;
		int			lastTime;
		float		fraction;
		float		speed;
		qboolean	paused;
	} play;
	struct {
		float speed, acceleration, friction;
	} move;
	vec3_t			viewOrigin, viewAngles;
	demoViewType_t	viewType;
	vec_t			viewFov;
	int				viewTarget;
	float			viewFocus, viewFocusOld, viewRadius;
	demoEditType_t	editType;

	vec3_t		cmdDeltaAngles;
	usercmd_t	cmd, oldcmd;
	int			nextForwardTime, nextRightTime, nextUpTime;
	int			deltaForward, deltaRight, deltaUp;
	struct	{
		int		start, end;
		qboolean active, locked;
	} capture;
	struct {
		qhandle_t additiveWhiteShader;
		qhandle_t mouseCursor;
		qhandle_t switchOn, switchOff;
	} media;
	demofindType_t find;
	qboolean	seekEnabled;
	qboolean	initDone;
	qboolean	autoLoad;
	demoLog_t	log;
} demoMain_t;

extern demoMain_t demo;

void demoPlaybackInit(void);
centity_t *demoTargetEntity( int num );
int demoHitEntities( const vec3_t start, const vec3_t forward );
qboolean demoCentityBoxSize( const centity_t *cent, vec3_t container );

void CG_DemosAddLog(const char *fmt, ...);

//CAMERA
void cameraMove( void );
void cameraMoveDirect( void );
void cameraUpdate( int time, float timeFraction );
void cameraDraw( int time, float timeFraction );
void demoCameraCommand_f(void);
void cameraSave( fileHandle_t fileHandle );
qboolean cameraParse( BG_XMLParse_t *parse, const struct BG_XMLParseBlock_s *fromBlock, void *data);
demoCameraPoint_t *cameraPointSynch( int time );

//MOVE
void demoMovePoint( vec3_t origin, vec3_t velocity, vec3_t angles);
void demoMoveDirection( vec3_t origin, vec3_t angles );
void demoMoveUpdateAngles( void );
void demoMoveDeltaCmd( void );

//CHASE
void demoMoveChase( void );
void demoMoveChaseDirect( void );
void demoChaseCommand_f( void );
void chaseUpdate( int time, float timeFraction );
void chaseDraw( int time, float timeFraction );
void chaseEntityOrigin( centity_t *cent, vec3_t origin );
void chaseEntityOrigin( centity_t *cent, vec3_t origin );
void chaseSave( fileHandle_t fileHandle );
qboolean chaseParse( BG_XMLParse_t *parse, const struct BG_XMLParseBlock_s *fromBlock, void *data);
demoChasePoint_t *chasePointSynch(int time );

//LINE
void demoMoveLine( void );
void demoLineCommand_f(void);
void lineAt(int playTime, float playFraction, int *demoTime, float *demoFraction, float *demoSpeed );
void lineSave( fileHandle_t fileHandle );
qboolean lineParse( BG_XMLParse_t *parse, const struct BG_XMLParseBlock_s *fromBlock, void *data);
demoLinePoint_t *linePointSynch(int playTime);

//DOF
demoDofPoint_t *dofPointSynch( int time );
void dofMove(void);
void dofUpdate( int time, float timeFraction );
void dofDraw( int time, float timeFraction );
qboolean dofParse( BG_XMLParse_t *parse, const struct BG_XMLParseBlock_s *fromBlock, void *data);
void dofSave( fileHandle_t fileHandle );
void demoDofCommand_f(void);

//HUD
void hudInitTables(void);
void hudToggleInput(void);
void hudDraw(void);
const char *demoTimeString( int time );

//CAPTURE
void demoCaptureCommand_f( void );
void demoLoadCommand_f( void );
void demoSaveCommand_f( void );
void demoSaveLine( fileHandle_t fileHandle, const char *fmt, ...);
qboolean demoProjectLoad( const char *fileName );

#define CAM_ORIGIN	0x001
#define CAM_ANGLES	0x002
#define CAM_FOV		0x004
#define CAM_TIME	0x100
