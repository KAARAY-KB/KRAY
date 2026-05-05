/**
 *@author: Jingle
 *@brief:  General tools
 */
#ifndef _UTILS_H_
#define _UTILS_H_
#ifdef __cplusplus
extern "C"
{
#endif

#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <math.h>
// #include <rt_misc.h>
// #include <windows.h> //Win
// #include <unistd.h> //Linux Sleep()

#ifndef SWAP
#define SWAP(x, y) ((x ^= y), (y ^= x), (x ^= y))
#endif

#ifndef __PASTE
#define __PASTE(x, y) (x##y)
#endif

#ifndef __PASTE3
#define __PASTE3(x, y, z) (x##y##z)
#endif

#ifndef _PI
#define _PI (3.1415926535897932f)
#endif

#ifndef _2PI
#define _2PI (6.2831853071795864f)
#endif

#ifndef SQRT3
#define SQRT3 (1.7320508075688773f) /* √3 */
#endif

#ifndef SQRT3_BY_2
#define SQRT3_BY_2 (0.8660254037844386f) /* √3 / 2 */
#endif

#ifndef ONE_BY_SQRT3
#define ONE_BY_SQRT3 (0.5773502691896258f) /* 1 / √3 */
#endif

#ifndef TWO_BY_SQRT3
#define TWO_BY_SQRT3 (1.1547005383792515f) /* 2 / √3 */
#endif

#ifndef ANGLE_TO_RADIN
#define ANGLE_TO_RADIN(angle) (0.017453f * (float)(angle)) /* 2.0f * PI / 360.0f * angle */
#endif

#ifndef DEC2BCD
#define DEC2BCD(x) ((((x) / 10U) << 4U) + ((x) % 10U)) /* Decimal to BCD */
#endif

#ifndef BCD2DEC
#define BCD2DEC(x) ((((x) >> 4U) * 10U) + ((x) & 0x0FU)) /* BCD to decimal */
#endif

#ifndef ABS
#define ABS(x) ((x) > (0) ? (x) : (-x))
#endif

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof((arr)) / sizeof((arr)[0]))
#endif

#ifndef ARRAY_SIZE_TWO
#define ARRAY_SIZE_TWO(arr) (sizeof((arr)) / sizeof((arr)[0][0]))
#endif

#ifndef BIT_SET
#define BIT_SET(value, bit) ((value) |= (bit))
#endif
#ifndef BIT_READ
#define BIT_READ(value, bit) ((value) & (bit)) /* this value this bit value */
#endif
#ifndef BIT_VALUE
#define BIT_VALUE(value, bit) ((value) & (bit)) /* this bit value */
#endif
#ifndef BIT_CLEAR
#define BIT_CLEAR(value, bit) ((value) &= ~(bit))
#endif
#ifndef BIT_TOGGLE
#define BIT_TOGGLE(value, bit) ((value) ^= (bit))
#endif
#ifndef BIT_MASK
#define BIT_MASK(value, bit) ((value) ^= (bit))
#endif

#ifndef LIMIT
#define LIMIT(val, low, high) (((val) < (low)) ? (low) : ((val) > (high)) ? (high) : (val)) /* limit */
#endif

#ifndef LIMIT_ABS
#define LIMIT_ABS(val, absLimit) (((val) > (absLimit)) ? (absLimit) : ((val) < (-absLimit)) ? (-absLimit) : (val)) /* the absolute limit */
#endif

#ifndef MAP
#define MAP(val, inMin, inMax, outMin, outMax) (((val) - (inMin)) * ((outMax) - (outMin)) / ((inMax) - (inMin)) + (outMin)) /* proportion of mapping */
#endif

static inline uint16_t stringToHex(const char *delimiter, const uint8_t *input, uint8_t *hexArray, uint16_t maxElements)
{
	// 使用 strtok 函数分割字符串
	char *token    = strtok((char *)input, delimiter);
	uint16_t index = 0;

	// 遍历子字符串并将其转换为十六进制
	while (token != NULL && index < maxElements)
	{
		sscanf(token, "%x", (int *)&hexArray[index]);
		index += 1;
		token  = strtok(NULL, delimiter);
	}
	return index;
}

static inline uint16_t swap_halfword(uint16_t data)
{
	return ((data & 0x00FF) << 8) | ((data & 0xFF00) >> 8);
}
static inline uint8_t reverse_byte(uint8_t data)
{
	data = (data << 4) | (data >> 4);
	data = ((data << 2) & 0xcc) | ((data >> 2) & 0x33);
	data = ((data << 1) & 0xaa) | ((data >> 1) & 0x55);
	return data;
}
static inline int mod(int dividend, int divisor)
{
	int r = dividend % divisor;
	return (r < 0) ? (r + divisor) : r;
}
static inline float fmodf_pos(float x, float y)
{
	float out = fmodf(x, y);
	if (out < 0.0f)
	{
		out += y;
	}
	return out;
}
static inline float wrap_pm(float x, float pm_range)
{
	return fmodf_pos(x + pm_range, 2.0f * pm_range) - pm_range;
}

static inline float wrap_pm_pi(float theta)
{
	return wrap_pm(theta, _PI);
}

#ifdef __cplusplus
}
#endif

#endif /* _UTILS_H_ */
