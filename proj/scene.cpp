#define _USE_MATH_DEFINES

#include "scene.h"
#include "m33.h"
#include "ppc.h"
#include <float.h>
#include "CImg.h"
#include "v3.h"
#include <iostream>
#include<math.h>
#include <sstream>

using namespace std;
using namespace cimg_library;

Scene *scene;

Scene::Scene() 
{
    // create interface between CPU and GPU
    cgi = new CGInterface();
    soi = new ShaderOneInterface();

    // create user interface
    gui = new GUI();
	
    gui->show();
	buff = new Fl_Text_Buffer();

	gui->lPosDisp->buffer(buff);

	buff->text("0 0 0");
	gui->lPosDisp->textsize(17);

	posDisp = "";

    // create SW framebuffer
    int u0 = 20;
    int v0 = 50;
    int sci = 2;
    int w = sci*320;
    int h = sci*175;

	
	

    fb = new FrameBuffer(u0, v0, w, h);

    fb->label("SW Framebuffer");

    // create HW framebuffer
    hwfb = new FrameBuffer(u0+fb->w+20, v0, w, h);

    hwfb->label("HW Framebuffer");
    hwfb->isHW = true;
   // hwfb->show();

	
    // position UI window
    gui->uiw->position(50, v0+25);

    // create camera for rendering scene
    float hfov = 45.0f;
    ppc = new PPC(hfov, w, h);

    // load, scale and position geometry, i.e. triangle meshes
    tmsN = 1;
    tms = new TMesh[tmsN];

    V3 center(0.0f, 0.0f, -175.0f);
    
    tms[0].Load("geometry/teapot57k.bin");
 
    AABB aabb0 = tms[0].GetAABB();

    float size0 = (aabb0.corners[1]-aabb0.corners[0]).Length();

    V3 tcenter0 = tms[0].GetCenter();
    tms[0].Translate(tcenter0*-1.0f+center);

    float sizex = 170.0f;
    tms[0].ScaleAboutCenter(sizex/size0);
    tms[0].renderWF = false;
    tms[0].shaderIsEnabled = 0;



    // render scene
    //Render();

	right = openImgGrey("pics/testA-1.bmp");
	up = openImgGrey("pics/testA-2.bmp");
	left = openImgGrey("pics/testA-3.bmp");

	tB1 = openImgGrey("pics/testB-1.bmp");
	tB2 = openImgGrey("pics/testB-2.bmp");
	tB3 = openImgGrey("pics/testB-3.bmp");
	tB4 = openImgGrey("pics/testB-4.bmp");
	tB5 = openImgGrey("pics/testB-5.bmp");

	imgScene = new FrameBuffer(u0+350, v0+25, right->w, right->h);

	
	
	curLightPos = V3(0,0,0);
}


// SW per frame setup
void Scene::FrameSetup() 
{
    fb->Set(0xFF7F0000); // clear color buffer
    fb->SetZB(0.0f); // clear z buffer
}

// render frame
void Scene::Render() 
{
    if (hwfb) 
    {
        // ask to redraw HW framebuffer; will get FrameBuffer::draw called (callback), which will call either
        //        Scene::RenderHW for fixed pipeline HW rendering, or Scene::RenderGPU for GPU HW rendering;
        //        this is needed since with FLTK one can only make GL calls from FrameBuffer::draw
        hwfb->redraw();
    }
    return;

    // SW rendering
    FrameSetup();

    // render all geometry
    for (int tmi = 0; tmi < tmsN; tmi++) 
    {
        if (!tms[tmi].enabled)
            continue;
        if (tms[tmi].renderWF) 
        {
            tms[tmi].RenderWF(ppc, fb); // wireframe
        }
        else 
		{
            tms[tmi].Render(ppc, fb); // regular filled rendering
        }
    }

	// this calls FrameBuffer::draw wich posts pixels with glDrawPixels;
    fb->redraw(); 
}

// function linked to the DBG GUI button for testing new features
void Scene::DBG() 
{
    cerr << "INFO: DBG" << endl;
}

void Scene::lightPosX()
{
	curLightPos = curLightPos + V3(1,0,0);
	changePositionDisplay();
}

void Scene::lightNegX()
{
	curLightPos = curLightPos + V3(-1,0,0);
	changePositionDisplay();
}

void Scene::lightPosY()
{
	curLightPos = curLightPos + V3(0,1,0);
	changePositionDisplay();
}

void Scene::lightNegY()
{
	curLightPos = curLightPos + V3(0,-1,0);
	changePositionDisplay();
}

void Scene::lightPosZ()
{
	curLightPos = curLightPos + V3(0,0,1);
	changePositionDisplay();
}

void Scene::lightNegZ()
{
	curLightPos = curLightPos + V3(0,0,-1);
	changePositionDisplay();
}

void Scene::setLightPos()
{
	getNormals();
	relight(curLightPos, imgScene);
}

void Scene::setLightPosFive()
{
	getNormalsFive();
	relightCol(curLightPos, imgScene);
}

void Scene::setLightPosSpecial()
{
	getNormals();
	relightSpecial(curLightPos, imgScene);
}

////////// HW rendering 

// there are two ways of rendering in HW
//1. fixed pipeline, i.e. "basic" OpenGL, no shaders, Scene::RenderHW
// 2. programmable pipeline, i.e. using GPU shaders, Scene::RenderGPU

// called by Scene::RenderHW and by Scene::RenderGPU, which is itself called by the draw callback of the HW framebuffer
void Scene::FrameSetupHW(PPC * cam)
{
    // OpenGL setup
    glEnable(GL_DEPTH_TEST);
    //  glEnable(GL_CULL_FACE);

    // frame setup
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | 
        GL_DEPTH_BUFFER_BIT);

    // sets HW view, i.e. OpenGL matrices according to our camera, PPC::ppc
    // set intrinsics
    cam->SetIntrinsicsHW();
    // set extrinsics
    cam->SetExtrinsicsHW();
}


// fixed HW pipeline
void Scene::RenderHW() 
{

    /// OpenGL frame setup
    FrameSetupHW(ppc);

    // render geometry: issue geometry for rendering
    for (int tmi = 0; tmi < 1; tmi++) {
        if (!tms[tmi].enabled)
            continue;
        tms[tmi].RenderHW();
    }
}


// gpu HW pipeline
void Scene::RenderGPU() 
{
    // per session initialization, i.e. once per run
    if (cgi->needInit) 
    {
        cgi->PerSessionInit();
        //soi->PerSessionInit(cgi);
    }

    FrameSetupHW(ppc);

    // per frame parameter setting and enabling shaders
    //soi->PerFrameInit();
    //soi->BindPrograms();
    //cgi->EnableProfiles();

    // issue geometry to be rendered with the shaders enabled above
    for(int i = 0; i < tmsN; i++)
    {
        if(tms[i].shaderIsEnabled)
        {
            cgi->EnableProfiles();
        }
        else
        {
            cgi->DisableProfiles();
        }
        tms[i].RenderHW();
    }

    // disable GPU rendering
    soi->PerFrameDisable();
    cgi->DisableProfiles();
}

FrameBuffer * Scene::openImgGrey(string fileName)
{
    CImg<unsigned char> src(fileName.c_str());

    int r, g, b;

    int width = src.width();
    int height = src.height();

    FrameBuffer * img;

    img = new FrameBuffer(0, 0, width, height);
    
    for(int h = 0; h < height; h++)
    {
        for(int w = 0; w < width; w++)
        {
            r = src.atXY(w, h, 0);
            g = src.atXY(w, h, 1);
            b = src.atXY(w, h, 2);

			float greyCol = (float)r/255 + (float)g/255 + (float)b/255;
			greyCol/= 3;
            
			V3 colorVal(greyCol, greyCol, greyCol);

			//V3 colorVal((float)r/255, (float)g/255, (float)b/255);
			
            img->Set(w, h, colorVal.GetColor());
        }
    }

    return img;
}

FrameBuffer * Scene::openImg(string fileName)
{
    CImg<unsigned char> src(fileName.c_str());

    int r, g, b;

    int width = src.width();
    int height = src.height();

    FrameBuffer * img;

    img = new FrameBuffer(0, 0, width, height);
    
    for(int h = 0; h < height; h++)
    {
        for(int w = 0; w < width; w++)
        {
            r = src.atXY(w, h, 0);
            g = src.atXY(w, h, 1);
            b = src.atXY(w, h, 2);

			float greyCol = (float)r/255 + (float)g/255 + (float)b/255;
			greyCol/= 3;
            
			V3 colorVal(greyCol, greyCol, greyCol);

			//V3 colorVal((float)r/255, (float)g/255, (float)b/255);
			
            img->Set(w, h, colorVal.GetColor());
        }
    }

    return img;
}

void Scene::getNormals()
{
	lDirs = new M33();

	lDirs->SetRow(0, V3(-1.0f, 0.0f, 1.0f).Normalized());
	lDirs->SetRow(1, V3(0.0f, 1.0f, 1.0f).Normalized());
	lDirs->SetRow(2, V3(1.0f, 0.0f, 1.0f).Normalized());

	lDirs->Invert();
	
	const int imgW = (int) right->w;
	const int imgH = (int) right->h;

    FrameBuffer * img = new FrameBuffer(0, 0, imgW, imgH);
	
	normals = new V3[imgW*imgH];	

	for(int i = 0; i < imgW; i++)
	{
		for(int j = 0; j < imgH; j++)
		{
			float r = right->Getv(i,j)[0];
			float u = up->Getv(i,j)[0];
			float l = left->Getv(i,j)[0];
			
			V3 colors = V3(r,u,l);

			normals[j*imgW + i] = *lDirs*colors;
			
			normals[j*imgW + i].Normalized();

			img->Set(i,j, normals[j*imgW + i].GetColor());
		}
	}
}

void Scene::relight(V3 lightPos, FrameBuffer * img)
{
	const int imgW = (int) right->w;
	const int imgH = (int) right->h;

	V3 lightDir = lightPos-V3(0,0,-1);
	
	lightDir.Normalized();

	for(int i = 0; i < imgW; i++)
	{
		for(int j = 0; j < imgH; j++)
		{
			float albedo = sqrt(normals[j*imgW + i] * normals[j*imgW + i]);
			
			float rColor = albedo*(normals[j*imgW + i]*lightDir);
			V3 newCol = V3(rColor,rColor,rColor);
			img->Set(i,j, newCol.GetColor());
		}
	}

	img->redraw();
	img->show();
}

void Scene::relightCol(V3 lightPos, FrameBuffer * img)
{
	const int imgW = (int) right->w;
	const int imgH = (int) right->h;

	V3 lightDir = lightPos-V3(0,0,-1);
	
	lightDir.Normalized();

	for(int i = 0; i < imgW; i++)
	{
		for(int j = 0; j < imgH; j++)
		{
			float albedo = sqrt(normals[j*imgW + i] * normals[j*imgW + i]);
			
			float rColor = albedo*(normals[j*imgW + i]*lightDir);
			V3 newCol = V3(rColor,rColor,rColor);
			img->Set(i,j, newCol.GetColor());
		}
	}

	img->redraw();
	img->show();
}

void Scene::relightSpecial(V3 lightPos, FrameBuffer * img)
{
	const int imgW = (int) right->w;
	const int imgH = (int) right->h;

	V3 lightDir = lightPos-V3(0,0,-1);
	
	lightDir.Normalized();

	for(int i = 0; i < imgW; i++)
	{
		for(int j = 0; j < imgH; j++)
		{
			float albedo = sqrt(normals[j*imgW + i] * normals[j*imgW + i]);
			
			float rColor = albedo*(normals[j*imgW + i]*lightDir);
			V3 newCol = V3(rColor,rColor,rColor);

			float eyeToNormAngle = acos(normals[j*imgW + i]*V3(0,0,1).Normalized() / 
				  (normals[j*imgW + i].Length() * V3(0,0,1).Normalized().Length()));

			eyeToNormAngle = eyeToNormAngle/(2*M_PI) * 360;

			if(eyeToNormAngle >= 0 && eyeToNormAngle <= 30)
			{
				newCol = V3(1,0,0);
			}
			else if(eyeToNormAngle >= 30 && eyeToNormAngle <= 45)
			{
				newCol = V3(0,1,0);
			}
			else if(eyeToNormAngle >= 45 && eyeToNormAngle <= 60)
			{
				newCol = V3(0,0,1);
			}

			img->Set(i,j, newCol.GetColor());
		}
	}

	img->redraw();
	img->show();
}

void Scene::changePositionDisplay()
{
	std::ostringstream oss;

	oss << curLightPos[0] << " " 
		<< curLightPos[1] << " " 
		<< curLightPos[2] << " ";

	posDisp.clear();

	posDisp = oss.str();

	buff->text(posDisp.c_str());
	gui->show();
}

void Scene::getNormalsFive()
{
	lDirs = new M33();

	V3 possibLD[5];

	possibLD[0] = V3(-1.0f, 0.0f, 1.0f); //right
	possibLD[1] = V3(0.0f, 1.0f, 1.0f); //up
	possibLD[2] = V3(1.0f, 0.0f, 1.0f); //left
	possibLD[3] = V3(-1.0f, 1.0f, 0.0f); //??
	possibLD[4] = V3(1.0f, 1.0f, 0.0f); //???

	int * set = new int[3]; 

	lDirs->Invert();
	
	const int imgW = (int) right->w;
	const int imgH = (int) right->h;

    FrameBuffer * img = new FrameBuffer(0, 0, imgW, imgH);
	
	normals = new V3[imgW*imgH];

	float * testBVals = new float[5];
	
	for(int i = 0; i < imgW; i++)
	{
		for(int j = 0; j < imgH; j++)
		{
			testBVals[0] = tB1->Getv(i,j)[0];
			testBVals[1] = tB2->Getv(i,j)[0];
			testBVals[2] = tB3->Getv(i,j)[0];
			testBVals[3] = tB4->Getv(i,j)[0];
			testBVals[4] = tB5->Getv(i,j)[0];

			getSet(testBVals, set);

			lDirs->SetColumn(0, possibLD[set[0]].Normalized());
			lDirs->SetColumn(1, possibLD[set[1]].Normalized());
			lDirs->SetColumn(2, possibLD[set[2]].Normalized());

			V3 colors = V3(testBVals[set[0]],
						   testBVals[set[1]],
						   testBVals[set[2]]);

			normals[j*imgW + i] = *lDirs*colors;
			
			normals[j*imgW + i].Normalized();

			img->Set(i,j, normals[j*imgW + i].GetColor());
		}
	}
}

void Scene::getSet(float * vals, int * set)
{
	float minVal, maxVal;
	minVal = 2;
	maxVal = -1;

	int minIndex, maxIndex;
	minIndex = maxIndex = -1;

	for(int i = 0; i < 5; i++)
	{
		if (vals[i] <= minVal)
		{
			minVal = vals[i];

			minIndex = i;
		}

		if (vals[i] >= maxVal)
		{
			maxVal = vals[i];
			maxIndex = i;
		}
	}
	
	int setVal = 0;

	for(int i = 0; i < 5; i++)
	{
		if(i != minIndex && i != maxIndex)
		{
			set[setVal] = i;
			setVal++;
		}
	}
}