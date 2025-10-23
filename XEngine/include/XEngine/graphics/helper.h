#pragma once

#include "log.h"
#include "glad/glad.h"
#include <string>

namespace XEngine::graphics
{
	void checkGLError();
}

#ifndef XENGINE_CONFIG_RELEASE
#define XENGINE_CHECK_GL_ERROR XEngine::graphics::checkGLError(); 
#else
#define XENGINE_CHECK_GL_ERROR (void)0
#endif

