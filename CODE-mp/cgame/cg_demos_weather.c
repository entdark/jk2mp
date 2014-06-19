// Nerevar's way to produce dismemberment
#include "cg_demos.h" 

#define RAINDISTANCE 500

void demoDrawRain(void) {
	float rainDistance, rainNumber;
	sfxHandle_t sfx;
	if (mov_rain.integer <= 0)
		return;
	else if (mov_rain.integer > 2000)
	trap_Cvar_Set("mov_rain", "10");		
	rainDistance = mov_rainDistance.value < 10.0f ?  10.0f : mov_rainDistance.value;
	rainNumber = mov_rain.value * (rainDistance / 1000.0f);
	if (mov_rain.integer >= 1000) {
		sfx = cgs.media.heavyRain;
	} else if (mov_rain.integer >= 400) {
		sfx = cgs.media.regularRain;
	} else {
		sfx = cgs.media.lightRain;
	}
	trap_S_AddLoopingSound(ENTITYNUM_NONE, cg.refdef.vieworg, vec3_origin, sfx);
	if (cg.we.raintime <= cg.time) {
		int i;						
		for (i = 0; i < (int)rainNumber; i++) {
			vec3_t start, fwd, end;
			trace_t tr;
				
			float angle = cg.refdefViewAngles[YAW];
			int seed = trap_Milliseconds() - i * 15;
			float distance = Q_irand(0, 2.0f * rainDistance) - rainDistance;				
				
//			AngleVectors(cg.refdefViewAngles, fwd, NULL, NULL);	 //<-Viewangles YAW		
//			VectorMA(cg.refdef.vieworg, rainDistance, fwd, start);
			VectorCopy(cg.refdef.vieworg, start);
			start[2] = cg.refdef.vieworg[2];
				
			start[0] = start[0] + (cos(angle)*distance);
			start[1] = start[1] + (sin(angle)*distance);
				
			angle += DEG2RAD(90);
			distance = Q_random(&seed) * 2.0f * rainDistance - rainDistance;
				
			start[0] = start[0] + (cos(angle)*distance);
			start[1] = start[1] + (sin(angle)*distance);
				
			//GET SKY
					
			VectorCopy(start,end);
			end[2] += 9999;				
					
			CG_Trace(&tr, start, NULL, NULL, end, 0, MASK_SOLID);
				
			if (tr.surfaceFlags & SURF_SKY) {
				vec3_t angles;
				if (tr.endpos[2] - start[2] > 200)
					tr.endpos[2] = start[2] + 200;
				tr.endpos[2] += Q_crandom(&seed) * 100.0f;
				angles[PITCH] = 0;
				angles[YAW] = 90;
				angles[ROLL] = 0;
				trap_FX_PlayEffectID(cgs.effects.rain, tr.endpos, angles);
			}
		}			
		cg.we.raintime = cg.time + 30;				
	}
}

void demoDrawSun(void) {
	int i;
	float size = 0.0f;
	trace_t tr;
	refEntity_t re;
		
	if (mov_sun.integer <= 0)
		return;

	//CALCULATE SIZE //eats too much fps
	CG_Trace(&tr, cg.refdef.vieworg, NULL, NULL, cg.we.sunorigin, 0, MASK_SHOT);
	if (!(tr.surfaceFlags & SURF_SKY)) //not even worth it
		return;	
		
	for (i = 31; i > 0; i--) {
		vec3_t trmaxs,trmins;			
		VectorSet(trmaxs, i, i, i);
		VectorSet(trmins, -i, -i, -i);			
		CG_Trace(&tr, cg.refdef.vieworg, trmins, trmaxs, cg.we.sunorigin, 0, MASK_SHOT);
		size = i;
		if (tr.surfaceFlags & SURF_SKY) {
			float f, step = 1.0f / mov_sun.value;
			for (f = (float)(i + 1); f > (float)(i); f -= step) {
				VectorSet(trmaxs, f, f, f);
				VectorSet(trmins, -f, -f, -f);
				CG_Trace(&tr, cg.refdef.vieworg, trmins, trmaxs, cg.we.sunorigin, 0, MASK_SHOT);
				size = f;
				if (tr.surfaceFlags & SURF_SKY) {
					break;
				}
			}
			break;
		}
	}
	size *= mov_sunSize.value;
	if (size <= 0.0f)
		return;
	memset(&re, 0, sizeof(refEntity_t));
	re.reType = RT_SPRITE;
	re.customShader = cgs.media.saberFlare;	
	VectorCopy(tr.endpos,re.origin);
		
	re.radius = size * Distance(re.origin, cg.refdef.vieworg) / 20;
	re.renderfx = RF_NODEPTH | RF_MINLIGHT;	
				
	re.shaderRGBA[0] = 255;
	re.shaderRGBA[1] = 230; //0.9
	re.shaderRGBA[2] = 191; //0.75
	re.shaderRGBA[3] = 255; 
		
	trap_R_AddRefEntityToScene( &re );
}
