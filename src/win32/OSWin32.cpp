// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "Win32Setup.h"

#include "OS.h"
#include "FileSystem.h"
#include "TextUtils.h"
#include "breakpad/exception_handler.h"
#include <SDL.h>
#include <stdio.h>
#include <wchar.h>
#include <windows.h>

using namespace google_breakpad;

ExceptionHandler* exceptionHandler = nullptr;

namespace OS {

// Notify Windows that the window may become unresponsive
void NotifyLoadBegin()
{
	// XXX MinGW doesn't know this function
#ifndef __MINGW32__
	// XXX Remove the following call when loading is moved to a background thread
	DisableProcessWindowsGhosting(); // Prevent Windows from whiting out the screen for "not responding"
#endif
}

// Since there's no way to re-enable Window ghosting, do nothing
void NotifyLoadEnd()
{
}

const char *GetIconFilename()
{
	// SDL doc says "Win32 icons must be 32x32".
	return "icons/badge32-8b.png";
}

void RedirectStdio()
{
	std::string output_path = FileSystem::JoinPath(FileSystem::GetUserDir(), "output.txt");
	std::wstring woutput_path = transcode_utf8_to_utf16(output_path);

	FILE *f;

	f = _wfreopen(woutput_path.c_str(), L"w", stderr);
	if (!f) {
		Output("ERROR: Couldn't redirect output to '%s': %s\n", output_path.c_str(), strerror(errno));
	} else {
		setvbuf(f, 0, _IOLBF, BUFSIZ);
	}
}

void EnableFPE()
{
	// clear any outstanding exceptions before enabling, otherwise they'll
	// trip immediately
#ifdef _MCW_EM
	_clearfp();
	_controlfp(_EM_INEXACT | _EM_UNDERFLOW, _MCW_EM);
#endif
}

void DisableFPE()
{
#ifdef _MCW_EM
	_controlfp(_MCW_EM, _MCW_EM);
#endif
}

Uint64 HFTimerFreq()
{
	LARGE_INTEGER i;
	QueryPerformanceFrequency(&i);
	return i.QuadPart;
}

Uint64 HFTimer()
{
	LARGE_INTEGER i;
	QueryPerformanceCounter(&i);
	return i.QuadPart;
}

int GetNumCores()
{
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	return sysinfo.dwNumberOfProcessors;
}

/////////////////////////////////////////////////////// Google Breakpad
bool ParagonFilterCallback(void* context, EXCEPTION_POINTERS* exinfo,
	MDRawAssertionInfo* assertion) 
{	
	return true;
}

bool ParagonMinidumpCallback(const wchar_t* dump_path,
	const wchar_t* minidump_id,
	void* context,
	EXCEPTION_POINTERS* exinfo,
	MDRawAssertionInfo* assertion,
	bool succeeded)
{
	std::wstring msg = L"Unhandled exception occured.\n";
	msg.append(L"Crash information dump was written to: \n    ");
	msg.append(transcode_utf8_to_utf16(FileSystem::userFiles.GetRoot() + "\\crashdumps"));
	msg.append(L"\nMini-dump id: \n    ");
	msg.append(minidump_id);
	MessageBoxW(NULL,
		msg.c_str(), L"Game crashed!", MB_OK | MB_ICONERROR);

	return succeeded;
}

void EnableBreakpad()
{
	CustomClientInfo cci;
	cci.count = 0;
	cci.entries = nullptr;
	std::wstring dumps_path;
	dumps_path = transcode_utf8_to_utf16(FileSystem::userFiles.GetRoot());
	FileSystem::userFiles.MakeDirectory("crashdumps");
	dumps_path.append(L"\\crashdumps");
	exceptionHandler = new ExceptionHandler(
		dumps_path,													// Dump path
		ParagonFilterCallback,										// Filter callback
		ParagonMinidumpCallback,									// Minidumps callback
		nullptr,													// Callback context
		ExceptionHandler::HandlerType::HANDLER_ALL,					// Handler types
		MINIDUMP_TYPE::MiniDumpWithDataSegs,
		L"",														// Minidump server pipe name
		&cci);														// Custom client information
}

} // namespace OS
