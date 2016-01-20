/************************************************************************************
This source file is part of the Ogre3D Theora Video Plugin
For latest info, see http://ogrevideo.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2010 Kresimir Spes (kreso@cateia.com)
This program is free software; you can redistribute it and/or modify it under
the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
*************************************************************************************/

#ifndef OGRE_MAC_FRAMEWORK
  #include "OgreExternalTextureSourceManager.h"
  #include "OgreRoot.h"
#else
  #include <Ogre/OgreExternalTextureSourceManager.h>
  #include <Ogre/OgreRoot.h>
#endif
#include "OgreTheoraVideoManager.h"
#include <stdio.h>

#ifdef OGRE_MAC_FRAMEWORK
#define OGREVIDEO_EXPORT extern "C"
#else
#define OGREVIDEO_EXPORT extern "C" __declspec(dllexport)
#endif

static Ogre::OgreTheoraVideoManager* theoraVideoPlugin;
OGREVIDEO_EXPORT void dllStartPlugin()
{
	theoraVideoPlugin = new Ogre::OgreTheoraVideoManager();
	// Register with Manager
	Ogre::ExternalTextureSourceManager::getSingleton().setExternalTextureSource("ogg_video", theoraVideoPlugin);
}
OGREVIDEO_EXPORT void dllStopPlugin()
{
	delete theoraVideoPlugin;
}
