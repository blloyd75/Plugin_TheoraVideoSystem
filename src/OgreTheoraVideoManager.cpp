/************************************************************************************
This source file is part of the Ogre3D Theora Video Plugin
For latest info, see http://ogrevideo.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2010 Kresimir Spes (kreso@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/
#ifndef OGRE_MAC_FRAMEWORK
  #include "OgreRoot.h"
#else
  #include <Ogre/OgreRoot.h>
#endif
#include "OgreTheoraVideoManager.h"
#include "OgreTheoraDataStream.h"

#ifndef OGRE_MAC_FRAMEWORK
#include "OgreTextureManager.h"
#include "OgreMaterialManager.h"
#include "OgreMaterial.h"
#include "OgreTechnique.h"
#include "OgreStringConverter.h"
#include "OgreLogManager.h"
#include "OgreHardwarePixelBuffer.h"
#else
#include <Ogre/OgreTextureManager.h>
#include <Ogre/OgreMaterialManager.h>
#include <Ogre/OgreMaterial.h>
#include <Ogre/OgreTechnique.h>
#include <Ogre/OgreStringConverter.h>
#include <Ogre/OgreLogManager.h>
#include <Ogre/OgreHardwarePixelBuffer.h>
#endif
#include "TheoraVideoManager.h"
#include "TheoraVideoFrame.h"
#include "TheoraTimer.h"
#include <vector>
#include <map>

namespace Ogre
{
	int nextPow2(int x)
	{
		int y;
		for (y=1;y<x;y*=2);
		return y;
	}

	static void ogrevideo_log(std::string message)
	{
		Ogre::LogManager::getSingleton().logMessage("ogg_video: " + message);
	}

	OgreTheoraVideoManager::OgreTheoraVideoManager(int num_worker_threads)
	{
		mDictionaryName = "TheoraVideoSystem";
		mbInit=false;
		mTechniqueLevel=mPassLevel=mStateLevel=0;
		initialise();
	}

	bool OgreTheoraVideoManager::initialise()
	{
		if (mbInit) return false;
		
		theoraVideoManager = TheoraVideoManager::getSingletonPtr();
		theoraVideoManagerOwned = !theoraVideoManager;
		if (theoraVideoManagerOwned) {
			theoraVideoManager = new ::TheoraVideoManager(1);
			TheoraVideoManager::setLogFunction(ogrevideo_log);
		}
		
		mbInit = true;
		addBaseParams(); // ExternalTextureSource's function
		Root::getSingleton().addFrameListener(this);
		return true;
	}
	
	OgreTheoraVideoManager::~OgreTheoraVideoManager()
	{
		shutDown();
	}

	void OgreTheoraVideoManager::shutDown()
	{
		if (!mbInit) return;

		videos_map::iterator i = movies.begin();
		videos_map::iterator i2 = movies.end();
		while (i != i2) {
			videos_list::iterator i2 = i->second.begin();
			videos_list::iterator i2e = i->second.end();
			while (i2 != i2e) {
				video_data &dat = *i2;
				if (dat.clip) theoraVideoManager->destroyVideoClip(dat.clip);
				dat.clip = NULL;
				// Ogre will destroy texture when it is no longer used.
				++i2;
			}
			++i;
		}
		movies.clear();
		if (theoraVideoManagerOwned) {
			delete theoraVideoManager;
			theoraVideoManagerOwned = false;
		}
		Root::getSingleton().removeFrameListener(this);
		mbInit=false;
	}
    
	bool OgreTheoraVideoManager::setParameter(const String &name, const String &value)
    {
        return ExternalTextureSource::setParameter(name, value);
    }
    
	String OgreTheoraVideoManager::getParameter(const String &name) const
    {
        return ExternalTextureSource::getParameter(name);
    }

	void OgreTheoraVideoManager::createDefinedTexture(const String& material_name, const String& group_name)
	{
		static unsigned long x = 0;
		std::pair<std::string, std::string> key = std::make_pair(material_name, group_name);
		char idbuffer[50];
		sprintf(idbuffer, "ogg_video:%ul", ++x);
		std::string id=idbuffer;
		TheoraVideoClip* clip=theoraVideoManager->createVideoClip(new OgreTheoraDataStream(mInputFileName,group_name,id),TH_BGRA,0,1);
		int w=nextPow2(clip->getWidth()),h=nextPow2(clip->getHeight());

		TexturePtr t = TextureManager::getSingleton().createManual(id,group_name,TEX_TYPE_2D,w,h,1,0,PF_X8R8G8B8,TU_DYNAMIC_WRITE_ONLY);
		
		if (t->getFormat() != PF_X8R8G8B8) 
			theoraVideoManager->logMessage("ERROR: Pixel format is not X8R8G8B8 which is what was requested!");
		// clear it to black

		unsigned char* texData=(unsigned char*) t->getBuffer()->lock(HardwareBuffer::HBL_DISCARD);
		memset(texData,255,w*h*4);
		t->getBuffer()->unlock();
		movies[key].push_back(video_data(t, clip));
		if (mMode == TextureEffectPlay_ASAP) {
			clip->setAutoRestart(false);
			clip->play();
		}
		else if (mMode == TextureEffectPause) {
			clip->setAutoRestart(false);
			clip->pause();
		}
		else if (mMode == TextureEffectPlay_Looping) {
			clip->setAutoRestart(true);
			clip->play();
		}

		// attach it to a material
		MaterialPtr material = MaterialManager::getSingleton().getByName(material_name);
		TextureUnitState* ts = material->getTechnique(mTechniqueLevel)->getPass(mPassLevel)->getTextureUnitState(mStateLevel);

		//Now, attach the texture to the material texture unit (single layer) and setup properties
		ts->setTextureName(id,TEX_TYPE_2D);
		ts->setTextureFiltering(Ogre::TFO_NONE);
		//ts->setTextureFiltering(FO_LINEAR, FO_LINEAR, FO_NONE);
		ts->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);

		// scale tex coords to fit the 0-1 uv range
		//Matrix4 mat=Matrix4::IDENTITY;
		//mat.setScale(Vector3((float) clip->getWidth()/w, (float) clip->getHeight()/h,1));
		//ts->setTextureTransform(mat);
	}

	void OgreTheoraVideoManager::destroyAdvancedTexture(const String& material_name, const String& group_name)
	{
		std::pair<std::string, std::string> key = std::make_pair(material_name, group_name);
		videos_map::iterator i = movies.find(key);
		if (i != movies.end()) {
			videos_list::iterator i2 = i->second.begin();
			videos_list::iterator i2e = i->second.begin();
			while (i2 != i2e) {
				video_data &dat = *i2;
				if (dat.clip) theoraVideoManager->destroyVideoClip(dat.clip);
				dat.clip = NULL;
				// Ogre will destroy texture when it is no longer used.
				Ogre::TextureManager::getSingleton().remove(dat.texture->getName());
				++i2;
			}
			movies.erase(i);
		}
	}

	bool OgreTheoraVideoManager::frameStarted(const FrameEvent& evt)
	{
		if (evt.timeSinceLastFrame > 0.3f)
			theoraVideoManager->update(0.3f);
		else
			theoraVideoManager->update(evt.timeSinceLastFrame);

		// update playing videos
		TheoraVideoFrame* f;
		videos_map::iterator i = movies.begin();
		videos_map::iterator ie = movies.end();
		while (i != ie) {
			videos_list::iterator i2 = i->second.begin();
			videos_list::iterator i2e = i->second.end();
			while (i2 != i2e) {
				video_data &dat = *i2;
				if (dat.clip) {
					f = dat.clip->getNextFrame();
					if (f && false)
					{
						int w = f->getStride(), h = f->getHeight();
						TexturePtr t = dat.texture;

						unsigned char *texData = (unsigned char*)t->getBuffer()->lock(HardwareBuffer::HBL_DISCARD);
						unsigned char *videoData = f->getBuffer();

						memcpy(texData, videoData, w*h * 4);
						memset(texData, 255, w*h * 4);
						t->getBuffer()->unlock();
						dat.clip->popFrame();
					}
				}
				++i2;
			}
			++i;
		}
		return true;
	}

} // end namespace Ogre
