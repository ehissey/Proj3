#pragma once


#include "gui.h"
#include "framebuffer.h"
#include "ppc.h"
#include "tmesh.h"


#include "CGInterface.h"



class Scene {
public:

    // interfaces for GPU programming
    CGInterface *cgi;
    ShaderOneInterface *soi;


    PPC *ppc; // camera used to render current frame

    TMesh *tms; // geometry
    int tmsN; // number of triangle meshes
    GUI *gui; // user interface
    FrameBuffer *fb, *hwfb, *up, *left, *right,* imgScene; // SW and HW framebuffers

	M33 * lDirs; //light directions from images

	V3 * normals, curLightPos;

    Scene();
    void DBG();

    void Render(); // SW & HW
    void RenderHW(); // Fixed pipeline
    void RenderGPU(); // Programmable pipeline
    void FrameSetupHW(PPC * cam); // clearing buffers and setting view for HW rendering

    void FrameSetup(); // clearing buffers for SW rendering

    void lightPosX();
	void lightNegX();
	
    void lightPosY();
	void lightNegY();

    void lightPosZ();
	void lightNegZ();

	void setLightPos();

	FrameBuffer * openImg(string fileName);
	void getNormals();
	void relight(V3 lightPos, FrameBuffer * img);
};

extern Scene *scene;

