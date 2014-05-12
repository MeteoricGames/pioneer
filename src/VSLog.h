// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _VS_LOG_H_
#define _VS_LOG_H_

#ifdef _MSC_VER
#include <Windows.h>
#undef max
#undef min
#include <sstream>

namespace VSLog
{
	static std::ostringstream stream;
	static void writeVector(const vector3d& v) {
		stream << v.x << ", " << v.y << ", " << v.z << " ";
	}
	static void writeVector(const vector3f& v) {
		stream << v.x << ", " << v.y << ", " << v.z << " ";
	}
	static void outputLog() {
		OutputDebugStringA(stream.str().c_str());
		stream.clear();
	}
};

#endif

#endif