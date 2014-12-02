// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

// Adapted from Robert Penner's easing equations
// and Jesus Gollonet's implementation for C++
//
// http://www.robertpenner.com/easing/
// https://github.com/jesusgollonet/ofpennereasing

#ifndef EASING_H
#define EASING_H

#include "FloatComparison.h"
#include <cmath>
#include <functional>

namespace Easing {

// args are:
// t: time point to calculate
// b: value at beginning of range
// c: change over range (ie end-begin)
// d: duration of range


// p(t) = t
namespace Linear {
	template <typename T> T EaseIn(T t, T b, T c, T d) {
		return c*t/d + b;
	}
	template <typename T> T EaseOut(T t, T b, T c, T d) {
		return c*t/d + b;
	}
	template <typename T> T EaseInOut(T t, T b, T c, T d) {
		return c*t/d + b;
	}
}

// p(t) = t^2
namespace Quad {
	template <typename T> T EaseIn(T t, T b, T c, T d) {
		t/=d;
		return c*t*t + b;
	}
	template <typename T> T EaseOut(T t, T b, T c, T d) {
		t/=d;
		return -c *t*(t-2) + b;
	}
	template <typename T> T EaseInOut(T t, T b, T c, T d) {
		t/=d;
		if (t/2 < 1) return ((c/2)*(t*t)) + b;
		return -c/2 * (((t-2)*(t-1)) - 1) + b;
	}
}

// p(t) = t^3
namespace Cubic {
	template <typename T> T EaseIn(T t, T b, T c, T d) {
		t/=d;
		return c*t*t*t + b;
	}
	template <typename T> T EaseOut(T t, T b, T c, T d) {
		t=t/d-1;
		return c*(t*t*t + 1) + b;
	}
	template <typename T> T EaseInOut(T t, T b, T c, T d) {
		t/=d/2;
		if (t < 1) return c/2*t*t*t + b;
		t-=2;
		return c/2*(t*t*t + 2) + b;
	}
}

// p(t) = t^4
namespace Quart {
	template <typename T> T EaseIn(T t, T b, T c, T d) {
		t/=d;
		return c*t*t*t*t + b;
	}
	template <typename T> T EaseOut(T t, T b, T c, T d) {
		t=t/d-1;
		return -c * (t*t*t*t - 1) + b;
	}
	template <typename T> T EaseInOut(T t, T b, T c, T d) {
		t/=d/2;
		if (t < 1) return c/2*t*t*t*t + b;
		t-=2;
		return -c/2 * (t*t*t*t - 2) + b;
	}
}

// p(t) = t^5
namespace Quint {
	template <typename T> T EaseIn(T t, T b, T c, T d) {
		t/=d;
		return c*t*t*t*t*t + b;
	}
	template <typename T> T EaseOut(T t, T b, T c, T d) {
		t=t/d-1;
		return c*(t*t*t*t*t + 1) + b;
	}
	template <typename T> T EaseInOut(T t, T b, T c, T d) {
		t/=d/2;
		if (t < 1) return c/2*t*t*t*t*t + b;
		t-=2;
		return c/2*(t*t*t*t*t + 2) + b;
	}
}

// p(t) = sin(t*pi/2)
namespace Sine {
	template <typename T> T EaseIn(T t, T b, T c, T d) {
		return -c * cos(t/d * (M_PI/2)) + c + b;
	}
	template <typename T> T EaseOut(T t, T b, T c, T d) {
		return c * sin(t/d * (M_PI/2)) + b;
	}
	template <typename T> T EaseInOut(T t, T b, T c, T d) {
		return -c/2 * (cos(M_PI*t/d) - 1) + b;
	}
}

// p(t) = 2^(10*(t-1))
namespace Expo {
	template <typename T> T EaseIn(T t, T b, T c, T d) {
		return (is_zero_general(t)) ? b : c * pow(2, 10 * (t/d - 1)) + b;
	}
	template <typename T> T EaseOut(T t, T b, T c, T d) {
		return (is_equal_general(t,d)) ? b+c : c * (-pow(2, -10 * t/d) + 1) + b;
	}
	template <typename T> T EaseInOut(T t, T b, T c, T d) {
		if (is_zero_general(t)) return b;
		if (is_equal_general(t,d)) return b+c;
		t/=d/2;
		if (t < 1) return c/2 * pow(2, 10 * (t - 1)) + b;
		return c/2 * (-pow(2, -10 * --t) + 2) + b;
	}
}

// p(t) = 1-sqrt(1-t^2)
namespace Circ {
	template <typename T> T EaseIn(T t, T b, T c, T d) {
		t/=d;
		return -c * (sqrt(1 - t*t) - 1) + b;
	}
	template <typename T> T EaseOut(T t, T b, T c, T d) {
		t/=d-1;
		return c * sqrt(1 - t*t) + b;
	}
	template <typename T> T EaseInOut(T t, T b, T c, T d) {
		t/=d/2;
		if (t < 1) return -c/2 * (sqrt(1 - t*t) - 1) + b;
		return c/2 * (sqrt(1 - t*(t-2)) + 1) + b;
	}
}

template <typename T>
class NumericTweener
{
public:
	NumericTweener() : 
		m_value(0), m_startValue(0), m_endValue(0),
		m_easingFunction(Linear::EaseInOut), m_duration(0.0f), m_time(0.0f) { }
	NumericTweener(T initial_value, std::function<T(T, T, T, T)> easing_function) : 
		m_value(initial_value), m_startValue(initial_value), m_endValue(initial_value), 
		m_easingFunction(easing_function), m_duration(0.0f), m_time(0.0f) { }
	virtual ~NumericTweener() {}

	void EaseTo(T end_value, float time_duration) {
		m_startValue = m_value;
		m_endValue = end_value; 
		m_duration = time_duration;
		m_time = 0.0f;
	}

	void Reset(T new_value) {
		m_value = new_value;
		m_startValue = new_value;
		m_endValue = new_value;
		m_duration = 0.0f;
		m_time = 0.0f;
	}

	T Update(float time_delta) { 
		if(IsTweening()) {
			m_time += time_delta;
			if(m_time >= m_duration) {
				m_value = m_endValue;
				m_startValue = m_value;
				m_time = 0.0f;
			} else {
				m_value = m_easingFunction(m_time, m_startValue, m_endValue - m_startValue, m_duration);
			}
		}
		return m_value;
	}

	bool IsTweening() const {
		return std::abs(m_value - m_endValue) > std::numeric_limits<T>::epsilon()? true : false;
	}

	void SetEasingFunction(std::function<T(T, T, T, T)> easing_function) {
		m_easingFunction = easing_function;
	}

	// Ends tweening: value = end_value
	void End() {
		if(IsTweening()) {
			m_value = m_endValue;
			m_startValue = m_value;
			m_time = 0.0f;
		}
	}

	// Stops tweening: value = value
	void Stop() {
		if(IsTweening()) {
			m_endValue = m_value;
			m_startValue = m_value;
			m_time = 0.0f;
		}
	}

	void SetValue(T new_value) { m_value = m_startValue = new_value; }
	void SetEndValue(T new_value) { m_endValue = new_value; }

	T GetValue() const { return m_value; }
	T GetStartValue() const { return m_startValue; }
	T GetEndValue() const { return m_endValue; }

protected:
	T m_value;
	T m_startValue;
	T m_endValue;
	float m_time;
	float m_duration;
	std::function<T(T, T, T, T)> m_easingFunction;
};

typedef NumericTweener<float > NumericTweenerF;
typedef NumericTweener<double> NumericTweenerD;

} // namespace Easing

#endif
