#pragma once

#if defined(_WIN32) || defined(_WIN64)
#ifdef TAPIRU_BUILD_INTERNAL
#define TAPIRU_API __declspec(dllexport)
#else
#define TAPIRU_API __declspec(dllimport)
#endif
#else
#ifdef TAPIRU_BUILD_INTERNAL
#define TAPIRU_API __attribute__((visibility("default")))
#else
#define TAPIRU_API
#endif
#endif
