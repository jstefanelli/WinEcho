#pragma once
#define WINECHO_EXPORT __declspec(dllexport)
#define WINECHO_IMPORT __declspec(dllimport)
#ifdef WINECHO_BUILD
#define WINECHO_API WINECHO_EXPORT
#else
#define WINECHO_API WINECHO_IMPORT
#endif