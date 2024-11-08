#ifndef __EXPORT_H_
#define __EXPORT_H_

#ifndef ERKIR_STATIC
#define ERKIR_STATIC
#endif

#ifdef _WIN32
#ifdef ERKIR_STATIC
#define ERKIR_EXPORT
#else
#ifdef MAKEDLL
#define ERKIR_EXPORT __declspec(dllexport)
#else
#define ERKIR_EXPORT __declspec(dllimport)
#endif
#endif
#else
#define ERKIR_EXPORT
#endif

#endif // __EXPORT_H_