// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _VECTOR4_H
#define _VECTOR4_H

#include <math.h>
#include <stdio.h>
#include "FloatComparison.h"
#include "vector3.h"

// Need this pragma due to operator[] implementation.
#pragma pack(4)

template <typename T> struct other_floating_type_4 {};
template <> struct other_floating_type_4<float> { typedef double type; };
template <> struct other_floating_type_4<double> { typedef float type; };

template <typename T>
class vector4 {
public:
	T x,y,z,w;

	// Constructor definitions are outside class declaration to enforce that
	// only float and double versions are possible.
	vector4();
	vector4(const vector4<T> &v);
	explicit vector4(const T  vals[4]);
	explicit vector4(const T  vals[3], T _w);
	explicit vector4(T val);
	explicit vector4(const vector3<T> &v, T _w);
	vector4(T _x, T _y, T _z, T _w);

	// disallow implicit conversion between floating point sizes
	explicit vector4(const vector4<typename other_floating_type_4<T>::type> &v);
	explicit vector4(const typename other_floating_type_4<T>::type vals[4]);
	explicit vector4(const typename other_floating_type_4<T>::type vals[3], typename other_floating_type_4<T>::type _w);

	const T& operator[](const size_t i) const { return (const_cast<const T *>(&x))[i]; }
	T& operator[](const size_t i) { return (&x)[i]; }

	vector4 &operator=(const vector3<T> &v) { x=v.x; y=v.y; z=v.z; return *this; }

	vector4 operator+(const vector4 &a) const { return vector4 (a.x+x, a.y+y, a.z+z, a.w+w); }
	vector4 &operator+=(const vector4 &a) { x+=a.x; y+=a.y; z+=a.z; w+=a.w; return *this; }
	vector4 &operator-=(const vector4 &a) { x-=a.x; y-=a.y; z-=a.z; w-=a.w; return *this; }
	vector4 &operator*=(const float a) { x*=a; y*=a; z*=a; w*=a; return *this; }
	vector4 &operator*=(const double a) { x*=a; y*=a; z*=a; w*=a; return *this; }
	vector4 &operator/=(const float a) { const T inva = T(1.0/a); x*=inva; y*=inva; z*=inva; w*=inva; return *this; }
	vector4 &operator/=(const double a) { const T inva = T(1.0/a); x*=inva; y*=inva; z*=inva; w*=inva; return *this; }
	vector4 operator-(const vector4 &a) const { return vector4(x-a.x, y-a.y, z-a.z, w-a.w); }
	vector4 operator-() const { return vector4(-x, -y, -z, -w); }

	bool ExactlyEqual(const vector4 &a) const {
		return is_equal_exact(a.x, x) && is_equal_exact(a.y, y) && is_equal_exact(a.z, z) && is_equal_exact(a.w, w);
	}

	friend vector4 operator*(const vector4 &a, const float  scalar) { return vector4(T(a.x*scalar), T(a.y*scalar), T(a.z*scalar), T(a.w*scalar)); }
	friend vector4 operator*(const vector4 &a, const double scalar) { return vector4(T(a.x*scalar), T(a.y*scalar), T(a.z*scalar), T(a.w*scalar)); }
	friend vector4 operator*(const float  scalar, const vector4 &a) { return a*scalar; }
	friend vector4 operator*(const double scalar, const vector4 &a) { return a*scalar; }
	friend vector4 operator/(const vector4 &a, const float  scalar) { const T inv = 1.0/scalar; return vector4(a.x*inv, a.y*inv, a.z*inv, a.w*inv); }
	friend vector4 operator/(const vector4 &a, const double scalar) { const T inv = 1.0/scalar; return vector4(a.x*inv, a.y*inv, a.z*inv, a.w*inv); }

	// Cross is only defined for 3 and 7 dimensions. So 3 dimensions cross is used by default.
	vector4 Cross(const vector4 &b) const { return vector4 (y*b.z - z*b.y, z*b.x - x*b.z, x*b.y - y*b.x, 0); }
	T Dot(const vector4 &b) const { return x*b.x + y*b.y + z*b.z + w*b.w; }
	T Length() const { return sqrt (x*x + y*y + z*z + w*w); }
	T LengthSqr() const { return x*x + y*y + z*z + w*w; }
	vector4 Normalized() const { const T l = 1.0f / sqrt(x*x + y*y + z*z + w*w); return vector4(x*l, y*l, z*l, w*l); }
	vector4 Normalized(T& length) const { const T l = sqrt(x*x + y*y + z*z + w*w); length = l; return vector4(x/l, y/l, z/l, w/l); }
	vector4 NormalizedSafe() const {
		const T lenSqr = x*x + y*y + z*z + w*w;
		if (lenSqr < 1e-18) // sqrt(lenSqr) < 1e-9
			return vector4(1,0,0,0);
		else {
			const T l = sqrt(lenSqr);
			return vector4(x/l, y/l, z/l, w/l);
		}
	}

	void Print() const { printf("v(%f,%f,%f,%f)\n", x, y, z, w); }
	vector3<T> ToVector3() { return vector3<T>(x, y, z); }
	inline void CopyTo(vector3<T>& v) { v.x = x; v.y = y; v.z = z; }
};

// These are here in this manner to enforce that only float and double versions are possible.
template<> inline vector4<float >::vector4() {}
template<> inline vector4<double>::vector4() {}
template<> inline vector4<float >::vector4(const vector4<float > &v): x(v.x), y(v.y), z(v.z), w(v.w) {}
template<> inline vector4<float >::vector4(const vector4<double> &v): x(float(v.x)), y(float(v.y)), z(float(v.z)), w(float(v.w)) {}
template<> inline vector4<double>::vector4(const vector4<float > &v): x(v.x), y(v.y), z(v.z), w(v.w) {}
template<> inline vector4<double>::vector4(const vector4<double> &v): x(v.x), y(v.y), z(v.z), w(v.w) {}
template<> inline vector4<float >::vector4(float  val): x(val), y(val), z(val), w(val) {}
template<> inline vector4<double>::vector4(double val): x(val), y(val), z(val), w(val) {}
template<> inline vector4<float >::vector4(float  _x, float  _y, float  _z, float  _w): x(_x), y(_y), z(_z), w(_w) {}
template<> inline vector4<double>::vector4(double _x, double _y, double _z, double  _w): x(_x), y(_y), z(_z), w(_w) {}
template<> inline vector4<float >::vector4(const float  vals[4]): x(vals[0]), y(vals[1]), z(vals[2]), w(vals[3]) {}
template<> inline vector4<float >::vector4(const double vals[4]): x(float(vals[0])), y(float(vals[1])), z(float(vals[2])), w(float(vals[3])) {}
template<> inline vector4<double>::vector4(const float  vals[4]): x(vals[0]), y(vals[1]), z(vals[2]), w(vals[3]) {}
template<> inline vector4<double>::vector4(const double vals[4]) : x(vals[0]), y(vals[1]), z(vals[2]), w(vals[3]) {}
template<> inline vector4<float >::vector4(const float  vals[3], float  _w) : x(vals[0]), y(vals[1]), z(vals[2]), w(_w) {}
template<> inline vector4<float >::vector4(const double vals[3], double _w) : x(float(vals[0])), y(float(vals[1])), z(float(vals[2])), w(float(_w)) {}
template<> inline vector4<double>::vector4(const float  vals[3], float  _w) : x(vals[0]), y(vals[1]), z(vals[2]), w(_w) {}
template<> inline vector4<double>::vector4(const double vals[3], double _w) : x(vals[0]), y(vals[1]), z(vals[2]), w(_w) {}
template<> inline vector4<float >::vector4(const vector3<float > &v, float  _w) : x(v.x), y(v.y), z(v.z), w(_w) {}
template<> inline vector4<double>::vector4(const vector3<double> &v, double _w) : x(v.x), y(v.y), z(v.z), w(_w) {}

#pragma pack()

typedef vector4<float > vector4f;
typedef vector4<double> vector4d;

#endif /* _VECTOR4_H */
