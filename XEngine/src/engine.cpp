#include "engine.h"
#include "log.h"

#include "app.h"

#include <SDL3/SDL.h>
#include <windows.h>

#include "input/mouse.h"
#include "input/keyboard.h"

namespace XEngine
{	
	
	extern "C" {
		__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
	}

	Engine& Engine::Instance()
	{
		if(!mInstance)
		{
			mInstance = new Engine();
		}

		return *mInstance;
	}

	void Engine::run(App* app)
	{
		mLogManager.initialize();
		XENGINE_ASSERT(!mApp,"Attempting to call Engine::run when a valid App already exist");
		if (mApp) { return; }
		mApp = app;
		if (initialize())
		{
			/*core loop*/
			while (mIsRunning)
			{		
				mWindow.pumpEvents();
				mApp->update();
				mWindow.beginRender();			
				mApp->render();
				mWindow.endRender();			
			}

			shutdown();
		}
	}

	
	//private

	bool Engine::initialize()
	{
		bool ret = false;
		XENGINE_ASSERT(!mIsInitialized, "Attempting to call Engine::initialize() more than once.")
		
		if (mIsInitialized) { return ret; }
		if (!(SDL_Init(SDL_INIT_VIDEO)))
		{
			XENGINE_ERROR("Error initializing SDL3: {}", SDL_GetError());
			return ret;
		}
		getInfo();
		core::Window::checkSDLVersion();

		if (mWindow.create())
		{
			// initialize Managers
			mRenderManager.initialize();

			ret = true;
			mIsRunning = true;
			mIsInitialized = true;

			// initialize input
			input::Mouse::initialize();
			input::Keyboard::initialize();

			// initialize app
			mApp->initialize();
		}


		if (!ret)
		{
			XENGINE_ERROR("Engine initialization failed. Sutting down.");
			shutdown();
		}

		return ret;
	}
	
	void Engine::shutdown()
	{
		mIsRunning = false;
		mIsInitialized = false;

		// Shutdown App
		mApp->shutdown();
		// Shutdown Managers - usually in reverse order
		mRenderManager.shutdown();
		mLogManager.shutdown();

		// Shutdown SDL
		mWindow.shutdown();

	}

	void Engine::getInfo()
	{
		XENGINE_TRACE("XEngine v{}.{}", 0, 1);
		#ifdef XENGINE_CONFIG_DEBUG
		XENGINE_DEBUG("Configuration: DEBUG");
		#endif

		#ifdef XENGINE_CONFIG_RELEASE
		XENGINE_DEBUG("Configuration: RELEASE");
		#endif

		#ifdef XENGINE_PLATFORM_WINDOWS
		XENGINE_WARN("Platform: WINDOWS");
		#endif

		#ifdef XENGINE_PLATFORM_MAC
		XENGINE_WARN("Platform: MAC");
		#endif

		#ifdef XENGINE_PLATFORM_LINUX
		XENGINE_WARN("Platform: LINUX");
		#endif
	}

	//singleton
	Engine* Engine::mInstance = nullptr;

	Engine::Engine() : 
		mIsRunning(false), 
		mIsInitialized(false), 
		mApp(nullptr)
	{}
}