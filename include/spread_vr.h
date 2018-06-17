#ifndef _SPREAD_VR_H
#define _SPREAD_VR_H

#include <CNFG3D.h>
#include <os_generic.h>
#include <libsurvive/survive.h>
#include <spreadgine.h>


extern float disappearing;
extern float diopter;
extern float fovie;
extern float eyez;
extern int gargc;
extern char ** gargv;
extern SurvivePose phmd;
extern SurvivePose wm0p;
extern SurvivePose wm1p;
extern Spreadgine * gspe;
extern SurviveObject * HMD;
extern SurviveObject * WM0;
extern SurviveObject * WM1;


extern og_mutex_t poll_mutex;

void SpreadSetupEyes();
void SpreadSetupVR();

#endif

