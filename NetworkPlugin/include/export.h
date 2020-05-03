#pragma once

#ifdef NETWORK_PLUGIN_EXPORT
#define NETWORK_PLUGIN_API __declspec(dllexport)
#else
#define NETWORK_PLUGIN_API __declspec(dllimport)
#endif