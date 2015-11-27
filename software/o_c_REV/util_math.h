#ifndef UTIL_MATH_H_
#define UTIL_MATH_H_

// Woo. Funky macro magic to avoid dividing by non-power-of-two.
// Essentially a quick fixed-point calculation, but only valid up to 2^exp

#define FAST_FP_DIV(n, div, exp) \
	(((n) * (((1 << exp) + 1) / div)) >> exp)

#define FAST_FP_MOD(n, div, exp) \
	((n) - FAST_FP_DIV(n, div, exp) * div)

#define DIV_8(n, div) \
	FAST_FP_DIV(n, div, 8)

#define MOD_8(n, div) \
	FAST_FP_MOD(n, div, 8)

#endif // UTIL_MATH_H_
