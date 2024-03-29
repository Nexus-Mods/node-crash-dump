#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <DbgHelp.h>
#include <string>
#include <napi.h>
#include <fstream>
#include <ctime>
#include "string_cast.h"


PVOID exceptionHandler = nullptr;

std::string dmpPath;
std::wstring dmpPathW;
bool writing = false;

void createMiniDump(std::ofstream &logFile, PEXCEPTION_POINTERS exceptionPtrs)
{
  typedef BOOL (WINAPI *FuncMiniDumpWriteDump)(HANDLE process, DWORD pid, HANDLE file, MINIDUMP_TYPE dumpType,
                                               const PMINIDUMP_EXCEPTION_INFORMATION exceptionParam,
                                               const PMINIDUMP_USER_STREAM_INFORMATION userStreamParam,
                                               const PMINIDUMP_CALLBACK_INFORMATION callbackParam);
  HMODULE dbgDLL = LoadLibraryW(L"dbghelp.dll");

  if (dbgDLL) {
    FuncMiniDumpWriteDump funcDump = reinterpret_cast<FuncMiniDumpWriteDump>(GetProcAddress(dbgDLL, "MiniDumpWriteDump"));
    if (funcDump) {
      HANDLE dumpFile = ::CreateFileW(dmpPathW.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, nullptr,
                                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

      if (dumpFile != INVALID_HANDLE_VALUE) {
        _MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;
        exceptionInfo.ThreadId = GetCurrentThreadId();
        exceptionInfo.ExceptionPointers = exceptionPtrs;
        exceptionInfo.ClientPointers = FALSE;

        logFile << "writing dump " << dmpPath << std::endl;

        BOOL success = funcDump(::GetCurrentProcess(), ::GetCurrentProcessId(), dumpFile, MiniDumpNormal,
                                &exceptionInfo, nullptr, nullptr);
        if (!success) { 
          logFile << "failed to write dump: " << std::hex << ::GetLastError() << std::dec << std::endl;
        } else {
          logFile << "success" << std::endl;
        }
        ::CloseHandle(dumpFile);
      } else {
        logFile << "failed to create dmp file: " << std::hex << ::GetLastError() << std::dec << std::endl;
      }
    } else {
      logFile << "wrong version of dbghelp.dll" << std::endl;
    }
    ::FreeLibrary(dbgDLL);
  } else {
    logFile << "dbghelp.dll not loaded: " << std::hex << ::GetLastError() << std::dec << std::endl;
  }
}

bool DoIgnore(DWORD code) {
  // list of exceptions that I'm fairly confident are not caused by user code
  return (code == 0x80010012)  // some COM errors, seem to be windows internal
      || (code == 0x80010105)
      || (code == 0x80010108)
      || (code == 0x8001010d)
      || (code == 0x8001010e)
      || (code == 0x80004035)
      || (code == 0x80040155)
      || (code == 0x800401fd)
      || (code == 0x800706b5)
      || (code == 0x800706ba)
      || (code == 0x800706be)  // something about rpc
      || (code == 0x80320012)  // timeout. Seems to happen in dns resolution
      || (code == 0xe0000001)  // seem to always be connected to msxml5/msxml6 and doesn't have any notable
      || (code == 0xe0000002)  //   impact on the user so probably handled internally
      || (code == 0xe06d7363)  // cpp exception (could be handled)
      || (code == 0xe0434352)  // c# exception
  ;
}

void openLogFile(std::ofstream &logFile, PEXCEPTION_POINTERS exceptionPtrs) {
  logFile.open((dmpPath + ".log").c_str(), std::fstream::out | std::fstream::app);
  logFile << "Exception time: " << time(nullptr) << std::endl;
  logFile << "Exception code: " << std::hex << exceptionPtrs->ExceptionRecord->ExceptionCode << std::dec << std::endl;
  logFile << "Exception address: " << std::hex << exceptionPtrs->ExceptionRecord->ExceptionAddress << std::dec << std::endl;
}

LONG WINAPI VEHandler(PEXCEPTION_POINTERS exceptionPtrs)
{
  if (   (exceptionPtrs->ExceptionRecord->ExceptionCode  < 0x80000000)  // non-critical
      || DoIgnore(exceptionPtrs->ExceptionRecord->ExceptionCode)) {
    // don't report non-critical exceptions
    return EXCEPTION_CONTINUE_SEARCH;
  }

  // avoid loop
  if (!writing) {
    writing = true;
    std::ofstream logFile;
    openLogFile(logFile, exceptionPtrs);
    createMiniDump(logFile, exceptionPtrs);

    writing = false;
  }

  return EXCEPTION_CONTINUE_SEARCH;
}

Napi::Value init(const Napi::CallbackInfo &info) {
  dmpPath = info[0].ToString();
  dmpPathW = toWC(dmpPath.c_str(), CodePage::UTF8, dmpPath.size());

  if (exceptionHandler == nullptr) {
    exceptionHandler = ::AddVectoredExceptionHandler(0, VEHandler);
  }
  return info.Env().Undefined();
}

Napi::Value deinit(const Napi::CallbackInfo &info) {
  ::RemoveVectoredExceptionHandler(exceptionHandler);
  return info.Env().Undefined();
}

Napi::Value crash(const Napi::CallbackInfo &info) {
  *(char*)0 = 0;
  return info.Env().Undefined();
}

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
  exports.Set("init", Napi::Function::New(env, init));
  exports.Set("deinit", Napi::Function::New(env, deinit));
  exports.Set("crash", Napi::Function::New(env, crash));
  return exports;
}

NODE_API_MODULE(CrashDump, InitAll)

