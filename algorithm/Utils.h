#pragma once

#include <cassert>
#include <cstdlib>
#include <cstdio>

#pragma warning(push)
#pragma warning(disable:4996) // for sprintf

#ifdef DEBUG
#define STRINGIFY(x) #x
#define __ASSERT_CT_DELAY(expr, file, line, exprStr) \
	static_assert(expr, "### [" __FILE__ ":" STRINGIFY(line) \
	"][Assertion failed: \"" #expr "\" is false]\n")
#define AssertCT(expr) __ASSERT_CT_DELAY(expr, __FILE__, __LINE__, #expr)
#else
#define AssertCT(expr) expr
#endif

#ifdef DEBUG
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
inline void DebugWrite(const char* msg){
	OutputDebugStringA/*NSI*/(msg);
}
#else // _WIN32
inline void DebugWrite(const char* msg){
	//std::fputs(msg, stderr);
	std::fputs(msg, stdout);
}
#endif // _WIN32
#define ExpectRT(expr) \
	__wrapper_for_expect_rt(expr, #expr, __FILE__, __LINE__, "Expectation not met")
inline bool __wrapper_for_expect_rt(
	const bool& expression,
	const char* exprStr,
	const char* file,
	int line,
	const char* msg
){
	if (expression == false){
		static char outStr[1024] = {0};
		std::sprintf(
			outStr,
			"### [%s:%d][%s: \"%s\" is false]\n",
			file, line, msg, exprStr
		);
		DebugWrite(outStr);
		return false;
	}
	return true;
}
#define AssertRT(expr) \
	__wrapper_for_assert_rt(expr, #expr, __FILE__, __LINE__)
inline void __wrapper_for_assert_rt(
	const bool& expression,
	const char* exprStr,
	const char* file,
	int line
){
	if (
		__wrapper_for_expect_rt(
			expression, exprStr, file, line,
			"Assertion failed"
		) == false
		){
		std::exit(EXIT_FAILURE);
	}
}
#else // DEBUG

#define ExpectRT(expr)
#define AssertRT(expr)

#endif // DEBUG

#pragma warning(pop)

template <typename T, typename... Args>
T max(T head, Args... tail) {
	return max(head, max(tail...));
}

template <typename T>
T max(T first, T second) {
	return first > second ? first : second;
}

template <typename T, typename... Args>
T min(T head, Args... tail) {
	return min(head, min(tail...));
}

template <typename T>
T min(T first, T second) {
	return first < second ? first : second;
}

inline bool IsPowerOf2(int num) {
	// SSE instruction for bit count
	AssertRT(num > 0);
	return __popcnt(num) == 1;
}