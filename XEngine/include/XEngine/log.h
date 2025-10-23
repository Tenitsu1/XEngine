#pragma once
#include "spdlog/spdlog.h"
#define XENGINE_DEFAULT_LOGGER_NAME "XEnginelogger"

#if defined(XENGINE_PLATFORM_WINDOWS)
#define XENGINE_BREAK __debugbreak();
#elif defined(XENGINE_PLATFORM_MAC)
#define XENGINE_BREAK __builtin_debugtrap();
#else
#define XENGINE_BREAK __builtin_trap();
#endif


#ifndef XENGINE_CONFIG_RELEASE
#define XENGINE_TRACE(...) if(spdlog::get(XENGINE_DEFAULT_LOGGER_NAME) != nullptr ) {spdlog::get(XENGINE_DEFAULT_LOGGER_NAME) -> trace(__VA_ARGS__);}
#define XENGINE_DEBUG(...) if(spdlog::get(XENGINE_DEFAULT_LOGGER_NAME) != nullptr ) {spdlog::get(XENGINE_DEFAULT_LOGGER_NAME) -> debug(__VA_ARGS__);}
#define XENGINE_INFO(...)  if(spdlog::get(XENGINE_DEFAULT_LOGGER_NAME) != nullptr ) {spdlog::get(XENGINE_DEFAULT_LOGGER_NAME) -> info(__VA_ARGS__);}
#define XENGINE_WARN(...)  if(spdlog::get(XENGINE_DEFAULT_LOGGER_NAME) != nullptr ) {spdlog::get(XENGINE_DEFAULT_LOGGER_NAME) -> warn(__VA_ARGS__);}
#define XENGINE_ERROR(...) if(spdlog::get(XENGINE_DEFAULT_LOGGER_NAME) != nullptr ) {spdlog::get(XENGINE_DEFAULT_LOGGER_NAME) -> error(__VA_ARGS__);}
#define XENGINE_FATAL(...) if(spdlog::get(XENGINE_DEFAULT_LOGGER_NAME) != nullptr ) {spdlog::get(XENGINE_DEFAULT_LOGGER_NAME) -> critical(__VA_ARGS__);}
#define XENGINE_ASSERT(x, msg) if((x)) {} else {XENGINE_FATAL("ASSERT - {}\n\t{}\n\tin file: {}\n\ton line :{}", #x, msg, __FILE__, __LINE__); XENGINE_BREAK}
#else
#define XENGINE_TRACE(...) (void)0
#define XENGINE_DEBUG(...) (void)0
#define XENGINE_INFO(...)  (void)0
#define XENGINE_WARN(...)  (void)0
#define XENGINE_ERROR(...) (void)0
#define XENGINE_FATAL(...) (void)0
#endif

