/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <kodi/AddonBase.h>
#include <kodi/Filesystem.h>
#include <dlfcn.h>

#define REGISTER_DLL_SYMBOL(functionPtr) \
  CDllHelper::RegisterSymbol(functionPtr, #functionPtr)

/// @brief Class to help with load of shared library functions
///
/// You can add them as parent to your class and to help with load of shared
/// library functions.
///
/// @note To use on Windows must you also include [dlfcn-win32](https://github.com/dlfcn-win32/dlfcn-win32) on your addon!\n\n
/// Furthermore, this allows the use of Android where the required library is copied to an EXE useable folder.
///
///
/// ----------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
///
/// #include <kodi/tools/DllHelper.h>
///
/// ...
/// class CMyInstance : public kodi::addon::CInstanceAudioDecoder,
///                     private CDllHelper
/// {
/// public:
///   CMyInstance(KODI_HANDLE instance);
///   bool Start();
///
///   ...
///
///   /* The pointers for on shared library exported functions */
///   int (*Init)();
///   void (*Cleanup)();
///   int (*GetLength)();
/// };
///
/// CMyInstance::CMyInstance(KODI_HANDLE instance)
///   : CInstanceAudioDecoder(instance)
/// {
/// }
///
/// bool CMyInstance::Start()
/// {
///   std::string lib = kodi::GetAddonPath("myLib.so");
///   if (!LoadDll(lib)) return false;
///   if (!REGISTER_DLL_SYMBOL(Init)) return false;
///   if (!REGISTER_DLL_SYMBOL(Cleanup)) return false;
///   if (!REGISTER_DLL_SYMBOL(GetLength)) return false;
///
///   Init();
///   return true;
/// }
/// ...
/// ~~~~~~~~~~~~~
///
class CDllHelper
{
public:
  CDllHelper() = default;
  virtual ~CDllHelper()
  {
    if (m_dll)
      dlclose(m_dll);
  }

  /// @brief Function to load requested library
  ///
  /// @param[in] path         The path with filename of shared library to load
  /// @return                 true if load was successful done
  ///
  bool LoadDll(std::string path)
  {
#if defined(TARGET_ANDROID)
    if (kodi::vfs::FileExists(path))
    {
      // Check already defined for "xbmcaltbinaddons", if yes no copy necassary.
      std::string xbmcaltbinaddons = kodi::vfs::TranslateSpecialProtocol("special://xbmcaltbinaddons/");
      if (path.compare(0, xbmcaltbinaddons.length(), xbmcaltbinaddons) != 0)
      {
        bool doCopy = true;
        std::string dstfile = xbmcaltbinaddons + kodi::vfs::GetFileName(path);

        STAT_STRUCTURE dstFileStat;
        if (kodi::vfs::StatFile(dstfile, dstFileStat))
        {
          STAT_STRUCTURE srcFileStat;
          if (kodi::vfs::StatFile(path, srcFileStat))
          {
            if (dstFileStat.size == srcFileStat.size && CDllHelper::timespec_get(&dstFileStat.modificationTime, TIME_UTC) > CDllHelper::timespec_get(&srcFileStat.modificationTime, TIME_UTC))
              doCopy = false;
          }
        }

        if (doCopy)
        {
          kodi::Log(ADDON_LOG_DEBUG, "Caching '%s' to '%s'", path.c_str(), dstfile.c_str());
          if (!kodi::vfs::CopyFile(path, dstfile))
          {
            kodi::Log(ADDON_LOG_ERROR, "Failed to cache '%s' to '%s'", path.c_str(), dstfile.c_str());
            return false;
          }
        }

        path = dstfile;
      }
    }
    else
    {
      return false;
    }
#endif

    m_dll = dlopen(path.c_str(), RTLD_LAZY);
    if (m_dll == nullptr)
    {
      kodi::Log(ADDON_LOG_ERROR, "Unable to load %s", dlerror());
      return false;
    }
    return true;
  }

  /// @brief Function to register requested library symbol
  ///
  /// @note This function should not be used, use instead the macro
  /// REGISTER_DLL_SYMBOL to register the symbol pointer.
  ///
  template <typename T>
  bool RegisterSymbol(T& functionPtr, const char* strFunctionPtr)
  {
    functionPtr = reinterpret_cast<T>(dlsym(m_dll, strFunctionPtr));
    if (functionPtr == nullptr)
    {
      kodi::Log(ADDON_LOG_ERROR, "Unable to assign function %s", dlerror());
      return false;
    }
    return true;
  }

private:
#if defined(TARGET_ANDROID)
  // timespec_get is broken on Android
  int timespec_get(timespec* ts, int base)
  {
    return (base == TIME_UTC && clock_gettime(CLOCK_REALTIME, ts) != -1) ? base : 0;
  }
#endif

  void* m_dll = nullptr;
};
