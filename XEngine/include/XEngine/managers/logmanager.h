#pragma once

namespace XEngine::managers
{
	class LogManager
	{
	public:
		LogManager() = default;
		~LogManager() = default;

		void initialize();
		void shutdown();
	};
};