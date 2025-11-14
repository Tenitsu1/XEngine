#pragma once

#include "core/window.h"
#include "managers/logmanager.h"

namespace XEngine
{
	class App;
	class Engine
	{
	public:
		static Engine& Instance();
		~Engine() {}

		void run(App* app);
		inline void quit() { mIsRunning = false;  }

		inline App& getApp() { return *mApp; }
		inline core::Window& getWindow() { return mWindow; }


	private:
		void getInfo();
		[[nodiscard]] bool initialize();
		void shutdown();
	private:
		bool mIsRunning;
		bool mIsInitialized;
		core::Window mWindow;
		App* mApp;

		// Managers
		managers::LogManager mLogManager;

		// Singleton
		Engine();
		static Engine* mInstance;
	};
}
