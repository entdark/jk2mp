// Nerevar's way to produce dismemberment
#include "cg_demos.h" 


/*=========================================================================================
===========================================================================================
																DISMEMBERMENT
===========================================================================================
=========================================================================================*/

void demoSaberDismember(centity_t *cent, vec3_t dir) {
	localEntity_t	*le;
	refEntity_t		*re;
	vec3_t saberorigin, saberangles;
	clientInfo_t *ci;
	
	if (!cent->ghoul2)
		return;
	
	////////////INIT
	le = CG_AllocLocalEntity();
	re = &le->refEntity;
	
	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = cg.time + 20000;
	le->lifeRate = 1.0 / (le->endTime - le->startTime);
	
	VectorCopy(cg_entities[cent->currentState.saberEntityNum].currentState.pos.trBase,saberorigin);
	VectorCopy(cg_entities[cent->currentState.saberEntityNum].currentState.apos.trBase,saberangles);
	
	VectorCopy( saberorigin, re->origin );
	AnglesToAxis( saberangles, re->axis );
	
	le->pos.trType = TR_GRAVITY;
	le->angles.trType = TR_GRAVITY;
	VectorCopy( saberorigin, le->pos.trBase );
	VectorCopy( saberangles, le->angles.trBase ); 
	le->pos.trTime = cg.time;
	le->angles.trTime = cg.time;

	le->bounceFactor = 0.6f;
	
	VectorCopy(dir, le->pos.trDelta );
	le->angles.trDelta[0] = Q_irand(-20,20);
	le->angles.trDelta[1] = Q_irand(-20,20);
	le->angles.trDelta[2] = Q_irand(-20,20);
	le->angles.trDelta[Q_irand(0,3)] = 0;
	
	le->leFragmentType = LEFT_SABER;

	/////////SABER GHOUL2
	ci = &cgs.clientinfo[cent->currentState.clientNum];
	if (ci->saberModel && ci->saberModel[0])
		trap_G2API_InitGhoul2Model(&re->ghoul2, va("models/weapons2/%s/saber_w.glm", ci->saberModel), 0, 0, 0, 0, 0);
	else
		trap_G2API_InitGhoul2Model(&re->ghoul2, "models/weapons2/saber/saber_w.glm", 0, 0, 0, 0, 0);
	
	/////REMOVE SABER FROM PLAYERMODEL
	if (trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), 1))
		trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), 1);
}

//Main dismemberment function
static void demoDismember( centity_t *cent , vec3_t dir, int part, vec3_t limborg, vec3_t limbang ) {
	localEntity_t	*le;
	refEntity_t		*re;
	const char *limbBone;
	char *limbName;
	char *limbCapName;
	char *stubCapName;
	int  limb_anim;
	int clientnum = cent->currentState.number;
		
	if (!cent->ghoul2 || cg_entities[clientnum].dism.cut[part] == qtrue)
		return;
	
	if (cg_entities[clientnum].dism.cut[DISM_WAIST] == qtrue)
		if (part >= DISM_HEAD && part <= DISM_RARM)  //connected to waist
			return;
	
	if (cg_entities[clientnum].dism.cut[DISM_LARM] == qtrue) 
		if (part == DISM_LHAND) //connected to left arm
			return;
	
	if (cg_entities[clientnum].dism.cut[DISM_RARM] == qtrue)
		if (part == DISM_RHAND) //connected to right arm
			return;

	////////////INIT
	le = CG_AllocLocalEntity();
	re = &le->refEntity;
	
	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = cg.time + 20000; //limb lifetime (FIXME: cvar?)
	le->lifeRate = 1.0f / (le->endTime - le->startTime);
	
	VectorCopy( limborg, re->origin );
	AnglesToAxis( limbang, re->axis );
	
	le->pos.trType = TR_GRAVITY;
	le->angles.trType = TR_GRAVITY;
	VectorCopy( limborg, le->pos.trBase );
	VectorCopy( limbang, le->angles.trBase); 
	le->pos.trTime = cg.time;
	le->angles.trTime = cg.time;

	le->bounceFactor = 0.1f + random()*0.2;
	
	VectorCopy(dir, le->pos.trDelta );
	le->leFragmentType = LEFT_GIB;
	
	/////////DUPLICATE GHOUL2
	if (re->ghoul2 && trap_G2_HaveWeGhoul2Models(re->ghoul2))
		trap_G2API_CleanGhoul2Models(&re->ghoul2);
	if (cent->ghoul2 && trap_G2_HaveWeGhoul2Models(cent->ghoul2))
		trap_G2API_DuplicateGhoul2Instance(cent->ghoul2, &re->ghoul2);
	
	/////////ANIMATION FIXME: stop routine animations
	
	switch( part ) {
		case DISM_HEAD:
			limbBone = "cervical";
			limbName = "head";
			limbCapName = "head_cap_torso_off";
			stubCapName = "torso_cap_head_off";
			limb_anim = demo15detected?BOTH_DISMEMBER_HEAD1_15:BOTH_DISMEMBER_HEAD1;
			break;
		case DISM_WAIST:
			limbBone = "pelvis";
			limbName = "torso";
			limbCapName = "torso_cap_hips_off";
			stubCapName = "hips_cap_torso_off";
			limb_anim = demo15detected?BOTH_DISMEMBER_HEAD1_15:BOTH_DISMEMBER_TORSO1;
			break;
		case DISM_LARM:
			limbBone = "lhumerus";
			limbName = "l_arm";
			limbCapName = "l_arm_cap_torso_off";
			stubCapName = "torso_cap_l_arm_off";
			limb_anim = demo15detected?BOTH_DISMEMBER_LARM_15:BOTH_DISMEMBER_LARM;
			break;
		case DISM_RARM:
			limbBone = "rhumerus";
			limbName = "r_arm";
			limbCapName = "r_arm_cap_torso_off";
			stubCapName = "torso_cap_r_arm_off";
			limb_anim = demo15detected?BOTH_DISMEMBER_RARM_15:BOTH_DISMEMBER_RARM;
			break;
		case DISM_LHAND:
			limbBone = "lradiusX";
			limbName = "l_hand";
			limbCapName = "l_hand_cap_l_arm_off";
			stubCapName = "l_arm_cap_l_hand_off";
			limb_anim = demo15detected?BOTH_DISMEMBER_LARM_15:BOTH_DISMEMBER_LARM;
			break;
		case DISM_RHAND:
			limbBone = "rradiusX";
			limbName = "r_hand";
			limbCapName = "r_hand_cap_r_arm_off";
			stubCapName = "r_arm_cap_r_hand_off";
			limb_anim = demo15detected?BOTH_DISMEMBER_RARM_15:BOTH_DISMEMBER_RARM;
			break;
		case DISM_LLEG:
			limbBone = "lfemurYZ";
			limbName = "l_leg";
			limbCapName = "l_leg_cap_hips_off";
			stubCapName = "hips_cap_l_leg_off";
			limb_anim = demo15detected?BOTH_DISMEMBER_LLEG_15:BOTH_DISMEMBER_LLEG;
			break;
		case DISM_RLEG:
			limbBone = "rfemurYZ";
			limbName = "r_leg";
			limbCapName = "r_leg_cap_hips_off";
			stubCapName = "hips_cap_r_leg_off";
			limb_anim = demo15detected?BOTH_DISMEMBER_RLEG_15:BOTH_DISMEMBER_RLEG;
			break;
		default:
			return;	
	}

	//FIXME: FREEZE THE ANIMATION

	//////////DISMEMBER
	trap_G2API_SetRootSurface(re->ghoul2, 0, limbName);
	trap_G2API_SetNewOrigin(re->ghoul2, trap_G2API_AddBolt(re->ghoul2, 0, limbBone));
	trap_G2API_SetSurfaceOnOff(re->ghoul2, limbCapName, 0);
	
	trap_G2API_SetSurfaceOnOff(cent->ghoul2, limbName, 0x00000100);
	trap_G2API_SetSurfaceOnOff(cent->ghoul2, stubCapName, 0);
	
	le->limbpart = part;
	cg_entities[clientnum].dism.cut[part] = qtrue;	
	cg_entities[clientnum].torsoBolt = 1;
	
	////EFFECTS
	trap_S_StartSound(limborg, cent->currentState.number, CHAN_BODY, trap_S_RegisterSound(va("sound/weapons/saber/saberhit%i.mp3",Q_irand(1,4))));
	VectorNormalize(dir);
	trap_FX_PlayEffectID( trap_FX_RegisterEffect("saber/blood_sparks.efx"), limborg, dir );
}

void demoCheckDismember(vec3_t saberhitorg) {
	centity_t *attacker;
	centity_t *target;
	vec3_t dir;
	float velocity;
	int i;
	
	vec3_t boltOrg[8];
	int newBolt;
	mdxaBone_t			matrix;
	char *limbTagName;
	float limbdis[8];
	qboolean cut[8];
	int dismnum, limbnum;
	
	float bestlen = 999999;
	int best = -1;

	for (i = 0; i < MAX_CLIENTS; i++) {
		centity_t *test = &cg_entities[i];		
		if (test && test->currentState.eFlags & EF_DEAD
			&& cg_entities[i].dism.deathtime
			&& test->currentState.eType == ET_PLAYER ) {
			if (cg_entities[i].dism.deathtime == cg.time ) {
				float dist = Distance(test->lerpOrigin,saberhitorg);			
				if (dist < bestlen) {
					bestlen = dist;
					best = i;
				}
			}
		}
	}

	/* found dismembered client? */
	if (best < 0)
		return;

	target = &cg_entities[best];
	if (!target)
		return;
		
	if (cg_entities[best].dism.lastkiller >= 0 && cg_entities[best].dism.lastkiller < MAX_CLIENTS) {
		attacker = &cg_entities[cg_entities[best].dism.lastkiller];
		if (!attacker)
			return;
	} else {
		return;
	}
	
	
		
	for (i = 0 ; i < 8; i++) {				
		if (i == DISM_HEAD) {
			limbTagName = "*head_cap_torso";
		} else if (i == DISM_LHAND) {
			limbTagName = "*l_hand_cap_l_arm";
		} else if (i == DISM_RHAND) {
			limbTagName = "*r_hand_cap_r_arm";
		} else if (i == DISM_LARM) {
			limbTagName = "*l_arm_cap_torso";
		} else if (i == DISM_RARM) {
			limbTagName = "*r_arm_cap_torso";
		} else if (i == DISM_LLEG) {
			limbTagName = "*l_leg_cap_hips";
		} else if (i == DISM_RLEG) {
			limbTagName = "*r_leg_cap_hips";
		} else /*if (i == DISM_WAIST)*/ {
			limbTagName = "*torso_cap_hips";
		}
		
		newBolt = trap_G2API_AddBolt( target->ghoul2, 0, limbTagName );

		if ( newBolt != -1 ) {
			trap_G2API_GetBoltMatrix(target->ghoul2, 0, newBolt, &matrix, target->lerpAngles, target->lerpOrigin, cg.time, cgs.gameModels, target->modelScale);

			trap_G2API_GiveMeVectorFromMatrix(&matrix, ORIGIN, boltOrg[i]);
			//trap_G2API_GiveMeVectorFromMatrix(&matrix, NEGATIVE_Y, boltAng[i]);
			
			//boltAng[i][0] = random();
			//boltAng[i][0] = random();
			//boltAng[i][0] = random();
			
			//TESTING
			//trap_FX_PlayEffectID(trap_FX_RegisterEffect("marker.efx"), boltOrg, target->lerpAngles);
			//trap_FX_PlayEffectID(trap_FX_RegisterEffect("marker.efx"), saberhitorg, target->lerpAngles);
			
			//trap_SendConsoleCommand(va("echo %i: %i %i %i, %i %i %i;\n",(int)Distance(saberhitorg,boltOrg), (int)boltOrg[0], (int)boltOrg[1], (int)boltOrg[2], (int)saberhitorg[0], (int)saberhitorg[1], (int)saberhitorg[2] ));
			
			limbdis[i] = Distance(saberhitorg,boltOrg[i]);
			cut[i] = qfalse;
		}
	}
	
	dismnum = 0;
	
	//CALC LIMB NUMBER TO DISMEMBER
	if ( BG_SaberInAttack(attacker->currentState.saberMove & ~ANIM_TOGGLEBIT) ) {
		if ( BG_SaberInSpecial(attacker->currentState.saberMove & ~ANIM_TOGGLEBIT) ) {
			limbnum = 3;
		} else {			
			limbnum = 2;
		}
	} else {
		limbnum = 1;
	}
	
	if (cgs.gametype == GT_CTF || cgs.gametype == GT_CTY) {
		i = 23;
	} else {
		i = 16;
	}
	
	limbnum += (int)(((VectorLength(attacker->currentState.pos.trDelta)/100)*(VectorLength(attacker->currentState.pos.trDelta)/100))/i);
	
	if (limbnum > 7) limbnum = 7;
	///////////////////////////////

	if (limbnum == 7) {
		//CGCam_Shake( 1500, 1500 );
		trap_S_StartSound(attacker->lerpOrigin, attacker->currentState.number, CHAN_AUTO, trap_S_RegisterSound("sound/gauss_shot.wav"));
	}
	
	//CG_CenterPrint(va("%i",limbnum), SCREEN_HEIGHT * .05, 0);
	
	if (limbnum == 7) {
		for (i = 0; i < 8; i++) {
			cut[i] = qtrue;
		}
	} else {
		while (dismnum < limbnum) {
			float bestlen = 999999999;
			int best = 0;
			for (i = 0; i < 8; i++) {
					if (limbdis[i] < bestlen && !cut[i]) {
						best = i;
						bestlen = limbdis[i];
					}
			}			
			cut[best] = qtrue;
			dismnum ++;
		}
	}
	
	velocity = VectorLength(target->currentState.pos.trDelta);
		
	for (i = 0; i < 8; i++) {
		if (cut[i]) {
			if (limbnum == 7) {
				dir[0] = (-1 + random()*2);
				dir[1] = (-1 + random()*2);
				dir[2] = (-1 + random()*2);
				VectorScale(dir,velocity,dir);
			} else {
				dir[0] = target->currentState.pos.trDelta[0] * (0.5 + random());
				dir[1] = target->currentState.pos.trDelta[1] * (0.5 + random());
				dir[2] = target->currentState.pos.trDelta[2] * (0.5 + random());
			}
			demoDismember(target,dir,i,boltOrg[i],dir);
		}
	}
}

void demoCheckCorpseDism( centity_t *attacker ) {
	centity_t *saber = &cg_entities[attacker->currentState.saberEntityNum];
	vec3_t saberstart,saberend,saberang;
	int i;
	
	if (!saber || !attacker || !attacker->saberLength 
		|| cgs.clientinfo[attacker->currentState.number].team == TEAM_SPECTATOR)
		return;
		
	VectorCopy(saber->currentState.pos.trBase,saberstart);
	VectorCopy(saber->currentState.apos.trBase,saberang);
	VectorMA(saberstart,attacker->saberLength,saberang,saberend);
	
	for (i = 0; i < MAX_CLIENTS; i++) {
		if (i != attacker->currentState.number) {
			centity_t *target = &cg_entities[i];
		
			if ( target && target->currentState.eFlags & EF_DEAD && Distance(target->lerpOrigin,saberend) < 80 && Distance(target->lerpOrigin,saberend)) {
				vec3_t boltOrg, boltAng;
				int newBolt;
				mdxaBone_t			matrix;
				int part;
				char *limbTagName;
				
				for (part = 0 ; part < 8; part++) {				
					if (part == DISM_HEAD) {
						limbTagName = "*head_cap_torso";
					} else if (part == DISM_LHAND) {
						limbTagName = "*l_hand_cap_l_arm";
					} else if (part == DISM_RHAND) {
						limbTagName = "*r_hand_cap_r_arm";
					} else if (part == DISM_LARM) {
						limbTagName = "*l_arm_cap_torso";
					} else if (part == DISM_RARM) {
						limbTagName = "*r_arm_cap_torso";
					} else if (part == DISM_LLEG) {
						limbTagName = "*l_leg_cap_hips";
					} else if (part == DISM_RLEG) {
						limbTagName = "*r_leg_cap_hips";
					} else /*if (part == DISM_WAIST)*/ {
						limbTagName = "*torso_cap_hips";
					}
					
					newBolt = trap_G2API_AddBolt( target->ghoul2, 0, limbTagName );

					if ( newBolt != -1 ) {
						int length;
						int rad = 5;
						
						if (part == DISM_WAIST)
							rad = 4;
							
						trap_G2API_GetBoltMatrix(target->ghoul2, 0, newBolt, &matrix, target->lerpAngles, target->lerpOrigin, cg.time, cgs.gameModels, target->modelScale);
			
						trap_G2API_GiveMeVectorFromMatrix(&matrix, ORIGIN, boltOrg);
						trap_G2API_GiveMeVectorFromMatrix(&matrix, NEGATIVE_Y, boltAng);
						
						for (length = 0; length < 5; length++) {
							vec3_t checkorg;							
							VectorMA(saberend,-attacker->saberLength*length*0.2, saberang,checkorg);							
							if (Distance(boltOrg,checkorg) < rad) {
								demoDismember(target,saberang,part,boltOrg,boltAng);
							}
						}
					}
				}											
			}
		}		
	}
}

void demoPlayerDismember(centity_t *cent) {
	int clientnum = cent->currentState.number;
	
	if (mov_dismember.integer == 2)
		demoCheckCorpseDism(cent);
		
	if (cent->currentState.eFlags & EF_DEAD) {
		//Make some smoke
		int newBolt;
		mdxaBone_t	matrix;
		int part;
		char *stubTagName;
			
		for (part = 0; part < 8; part++) {				
			if (cg_entities[clientnum].dism.cut[part] == qfalse) 
				continue;

			if (part >= DISM_HEAD && part <= DISM_RARM && cg_entities[clientnum].dism.cut[DISM_WAIST] == qtrue)
				continue;

			if (part == DISM_RHAND && cg_entities[clientnum].dism.cut[DISM_RARM] == qtrue)
				continue;
			if (part == DISM_LHAND && cg_entities[clientnum].dism.cut[DISM_LARM] == qtrue)
				continue;

			switch(part) {
				case DISM_HEAD:
					stubTagName = "*torso_cap_head";
					break;
				case DISM_LHAND:
					stubTagName = "*l_arm_cap_l_hand";
					break;
				case DISM_RHAND:
					stubTagName = "*r_arm_cap_r_hand";
					break;
				case DISM_LARM:
					stubTagName = "*torso_cap_l_arm";
					break;
				case DISM_RARM:
					stubTagName = "*torso_cap_r_arm";
					break;
				case DISM_LLEG:
					stubTagName = "*hips_cap_l_leg";
					break;
				case DISM_RLEG:
					stubTagName = "*hips_cap_r_leg";
					break;
				case DISM_WAIST:
					stubTagName = "*hips_cap_torso";
					break;
				default:
					continue;
			}
				
			newBolt = trap_G2API_AddBolt( cent->ghoul2, 0, stubTagName );
			if ( newBolt != -1 ) {
				vec3_t boltOrg, boltAng;
		
				trap_G2API_GetBoltMatrix(cent->ghoul2, 0, newBolt, &matrix, cent->lerpAngles, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
		
				trap_G2API_GiveMeVectorFromMatrix(&matrix, ORIGIN, boltOrg);
				trap_G2API_GiveMeVectorFromMatrix(&matrix, NEGATIVE_Y, boltAng);
		
				trap_FX_PlayEffectID(trap_FX_RegisterEffect("smoke_bolton"), boltOrg, boltAng);
			}
		}		
	}
}
