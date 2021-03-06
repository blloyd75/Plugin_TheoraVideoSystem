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
#include "OgreTheoraDataStream.h"

namespace Ogre
{
	OgreTheoraDataStream::OgreTheoraDataStream(const std::string &filename,const std::string &group_name, const std::string &id)
	{
		mName = id;
		mStream = ResourceGroupManager::getSingleton().openResource(filename,group_name);
	}

	OgreTheoraDataStream::~OgreTheoraDataStream()
	{
		if (!(mStream.isNull()))
		{
			mStream->close();
			mStream.setNull();
		}
	}

	int OgreTheoraDataStream::read(void* output,int nBytes)
	{
		return mStream->read( output,nBytes); 
	}

	void OgreTheoraDataStream::seek(uint64_t byte_index)
	{
		mStream->seek(byte_index);
	}

	std::string OgreTheoraDataStream::repr()
	{
		return mName;
	}

	uint64_t OgreTheoraDataStream::size()
	{
		return mStream->size();
	}

	uint64_t OgreTheoraDataStream::tell()
	{
		return mStream->tell();
	}

} // end namespace Ogre
