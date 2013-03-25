#include "scene.h"
#include "m33.h"
#include "ppc.h"
#include <float.h>
#include "CImg.h"
#include "v3.h"
#include <iostream>
#include<math.h>

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

	right = openImg("pics/testA-1.bmp");
	up = openImg("pics/testA-2.bmp");
	left = openImg("pics/testA-3.bmp");

	imgScene = new FrameBuffer(u0+350, v0+25, right->w, right->h);

	getNormals();
	
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
        else {
            tms[tmi].Render(ppc, fb); // regular filled rendering
        }
    }


    fb->redraw(); // this calls FrameBuffer::draw wich posts pixels with glDrawPixels;

}

// function linked to the DBG GUI button for testing new features
void Scene::DBG() 
{

    cerr << "INFO: DBG" << endl;

}


void Scene::lightPosX()
{
	curLightPos = curLightPos + V3(1,0,0);
}

void Scene::lightNegX()
{
	curLightPos = curLightPos + V3(-1,0,0);
}

void Scene::lightPosY()
{
	curLightPos = curLightPos + V3(0,1,0);
}

void Scene::lightNegY()
{
	curLightPos = curLightPos + V3(0,-1,0);
}

void Scene::lightPosZ()
{
	curLightPos = curLightPos + V3(0,0,1);
}

void Scene::lightNegZ()
{
	curLightPos = curLightPos + V3(0,0,-1);
}

void Scene::setLightPos()
{
	relight(curLightPos, imgScene);
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

	//img->show();
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