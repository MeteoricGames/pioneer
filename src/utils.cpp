// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "utils.h"
#include "libs.h"
#include "gameconsts.h"
#include "StringF.h"
#include "gui/Gui.h"
#include "Lang.h"
#include "FileSystem.h"
#include "PngWriter.h"
#include <sstream>

std::string format_money(Sint64 money)
{
	char buf[32];
	snprintf(buf, sizeof(buf), "$%.2f", 0.01*double(money));
	return std::string(buf);
}

class timedate {
public:
	timedate() : hour(0), minute(0), second(0), day(0), month(0), year(2184) {}
	timedate(int stamp) { *this = stamp; }
	timedate &operator=(int stamp);
	std::string fmt_time_date();
	std::string fmt_date();
private:
	int hour, minute, second, day, month, year;

	static const char * const months[12];
	static const unsigned char days[2][12];
};

const char * const timedate::months[] = {
	Lang::MONTH_JAN,
	Lang::MONTH_FEB,
	Lang::MONTH_MAR,
	Lang::MONTH_APR,
	Lang::MONTH_MAY,
	Lang::MONTH_JUN,
	Lang::MONTH_JUL,
	Lang::MONTH_AUG,
	Lang::MONTH_SEP,
	Lang::MONTH_OCT,
	Lang::MONTH_NOV,
	Lang::MONTH_DEC
};

const unsigned char timedate::days[2][12] = {
	{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
	{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};

timedate &timedate::operator=(int stamp)
{
	int i = int(stamp) % 86400;

	hour   = (i / 3600 + 24)%24; i %= 3600;
	minute = (i /   60 + 60)%60; i %=   60;
	second = (i+60)%60;

	int leap_years = EPOCH_START_YEAR / 4;
	int days_in_year = ((EPOCH_START_YEAR - leap_years) * 365) + (leap_years * 366);
	i = int(stamp) / 86400 + days_in_year - ((stamp < 0) ? 1 : 0); 

	int n400 = i / 146097; i %= 146097;
	int n100 = i /  36524; i %=  36524;
	int n4   = i /   1461; i %=   1461;
	int n1   = i /    365;

	year = n1 + n4 * 4 + n100 * 100 + n400 * 400 + !(n100 == 4 || n1 == 4) - 1;
	day = i % 365 + (n100 == 4 || n1 == 4) * 365;
	int leap = (year % 4 == 0 && year % 100) || (year % 400 == 0);

	month = 0;
	while (day >= days[leap][month])
		day -= days[leap][month++];
	month = month % 12;

	return *this;
}

std::string timedate::fmt_time_date()
{
	char buf[32];
	snprintf(buf, sizeof (buf), "%02d:%02d:%02d %d %s %d",
	         hour, minute, second, day + 1, months[month % 12], year);
	return buf;
}

std::string timedate::fmt_date()
{
	char buf[16];
	snprintf(buf, sizeof (buf), "%d %s %d",
	         day + 1, months[month % 12], year);
	return buf;
}

std::string format_date(double t)
{
	timedate stamp = int(t);
	return stamp.fmt_time_date();
}

std::string format_date_only(double t)
{
	timedate stamp = int(t);
	return stamp.fmt_date();
}

std::string string_join(std::vector<std::string> &v, std::string sep)
{
	std::vector<std::string>::iterator i = v.begin();
	std::string out;

	while (i != v.end()) {
		out += *i;
		++i;
		if (i != v.end()) out += sep;
	}
	return out;
}

void Error(const char *format, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);

	Output("error: %s\n", buf);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "JumpDrive error", buf, 0);

	exit(1);
}

void Warning(const char *format, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);

	Output("warning: %s\n", buf);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "JumpDrive warning", buf, 0);
}

void Output(const char *format, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);

	fputs(buf, stderr);
}

std::string format_distance(double dist, int precision)
{
	std::ostringstream ss;
	ss.setf(std::ios::fixed, std::ios::floatfield);
	if (dist < 1000) {
		ss.precision(0);
		ss << dist << " m";
	} else {
		ss.precision(precision);
		if (dist < AU*0.1) {
			ss << (dist*0.001) << " km";
		} else {
			ss << (dist/AU) << " AU";
		}
	}
	return ss.str();
}

void Screendump(const char* destFile, const int width, const int height)
{
	const std::string dir = "screenshots";
	FileSystem::userFiles.MakeDirectory(dir);
	const std::string fname = FileSystem::JoinPathBelow(dir, destFile);

	// pad rows to 4 bytes, which is the default row alignment for OpenGL
	const int stride = (3*width + 3) & ~3;

	std::vector<Uint8> pixel_data(stride * height);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glPixelStorei(GL_PACK_ALIGNMENT, 4); // never trust defaults
	glReadBuffer(GL_FRONT);
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, &pixel_data[0]);
	glFinish();

	write_png(FileSystem::userFiles, fname, &pixel_data[0], width, height, stride, 3);

	Output("Screenshot %s saved\n", fname.c_str());
}

// strcasestr() adapted from gnulib
// (c) 2005 FSF. GPL2+

#define TOLOWER(c) (isupper(c) ? tolower(c) : (c))

const char *pi_strcasestr (const char *haystack, const char *needle)
{
	if (!*needle)
		return haystack;

	// cache the first character for speed
	char b = TOLOWER(*needle);

	needle++;
	for (;; haystack++) {
		if (!*haystack)
			return 0;
		// XXX Temporary fix for comparing with non-ASCII characters
		char d = *haystack > 0? TOLOWER(*haystack) : *haystack;
		if (d == b) {
			const char *rhaystack = haystack + 1;
			const char *rneedle = needle;

			for (;; rhaystack++, rneedle++) {
				if (!*rneedle)
					return haystack;

				if (!*rhaystack)
					return 0;

				if (TOLOWER(*rhaystack) != TOLOWER(*rneedle))
					break;
			}
		}
	}
}

static const int HEXDUMP_CHUNK = 16;
void hexdump(const unsigned char *buf, int len)
{
	int count;

	for (int i = 0; i < len; i += HEXDUMP_CHUNK) {
		Output("0x%06x  ", i);

		count = ((len-i) > HEXDUMP_CHUNK ? HEXDUMP_CHUNK : len-i);

		for (int j = 0; j < count; j++) {
			if (j == HEXDUMP_CHUNK/2) Output(" ");
			Output("%02x ", buf[i+j]);
		}

		for (int j = count; j < HEXDUMP_CHUNK; j++) {
			if (j == HEXDUMP_CHUNK/2) Output(" ");
			Output("   ");
		}

		Output(" ");

		for (int j = 0; j < count; j++)
			Output("%c", isprint(buf[i+j]) ? buf[i+j] : '.');

		Output("\n");
	}
}

std::string glerr_to_string(GLenum err)
{
	switch (err)
	{
		case GL_INVALID_ENUM:
			return "GL_INVALID_ENUM";
		case GL_INVALID_VALUE:
			return "GL_INVALID_VALUE";
		case GL_INVALID_OPERATION:
			return "GL_INVALID_OPERATION";
		case GL_OUT_OF_MEMORY:
			return "GL_OUT_OF_MEMORY";
		case GL_STACK_OVERFLOW: //deprecated in GL3
			return "GL_STACK_OVERFLOW";
		case GL_STACK_UNDERFLOW: //deprecated in GL3
			return "GL_STACK_UNDERFLOW";
		case GL_INVALID_FRAMEBUFFER_OPERATION_EXT:
			return "GL_INVALID_FRAMEBUFFER_OPERATION";
		default:
			return stringf("Unknown error 0x0%0{x}", err);
	}
}

void CheckGLError() 
{
#ifndef NDEBUG
	// Check if an error occurred during the frame. This is not very useful for
	// determining *where* the error happened. For that purpose, try GDebugger or
	// the GL_KHR_DEBUG extension
	GLenum err;
	err = glGetError();
	if (err != GL_NO_ERROR) {
		std::stringstream ss;
		ss << "OpenGL error(s) during frame:\n";
		while (err != GL_NO_ERROR) {
			ss << glerr_to_string(err) << '\n';
			err = glGetError();
		}
		Error("%s", ss.str().c_str());
	}
#endif
}

