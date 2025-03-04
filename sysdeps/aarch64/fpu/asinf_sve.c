/* Single-precision SVE inverse sin

   Copyright (C) 2023-2025 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

#include "sv_math.h"
#include "poly_sve_f32.h"

static const struct data
{
  float32_t poly[5];
  float32_t pi_over_2f;
} data = {
  /* Polynomial approximation of  (asin(sqrt(x)) - sqrt(x)) / (x * sqrt(x))  on
    [ 0x1p-24 0x1p-2 ] order = 4 rel error: 0x1.00a23bbp-29 .  */
  .poly = { 0x1.55555ep-3, 0x1.33261ap-4, 0x1.70d7dcp-5, 0x1.b059dp-6,
	    0x1.3af7d8p-5, },
  .pi_over_2f = 0x1.921fb6p+0f,
};

/* Single-precision SVE implementation of vector asin(x).

   For |x| in [0, 0.5], use order 4 polynomial P such that the final
   approximation is an odd polynomial: asin(x) ~ x + x^3 P(x^2).

    The largest observed error in this region is 0.83 ulps,
      _ZGVsMxv_asinf (0x1.ea00f4p-2) got 0x1.fef15ep-2
				    want 0x1.fef15cp-2.

    For |x| in [0.5, 1.0], use same approximation with a change of variable

    asin(x) = pi/2 - (y + y * z * P(z)), with  z = (1-x)/2 and y = sqrt(z).

   The largest observed error in this region is 2.41 ulps,
     _ZGVsMxv_asinf (-0x1.00203ep-1) got -0x1.0c3a64p-1
				    want -0x1.0c3a6p-1.  */
svfloat32_t SV_NAME_F1 (asin) (svfloat32_t x, const svbool_t pg)
{
  const struct data *d = ptr_barrier (&data);

  svuint32_t sign = svand_x (pg, svreinterpret_u32 (x), 0x80000000);

  svfloat32_t ax = svabs_x (pg, x);
  svbool_t a_ge_half = svacge (pg, x, 0.5);

  /* Evaluate polynomial Q(x) = y + y * z * P(z) with
   z = x ^ 2 and y = |x|            , if |x| < 0.5
   z = (1 - |x|) / 2 and y = sqrt(z), if |x| >= 0.5.  */
  svfloat32_t z2 = svsel (a_ge_half, svmls_x (pg, sv_f32 (0.5), ax, 0.5),
			  svmul_x (pg, x, x));
  svfloat32_t z = svsqrt_m (ax, a_ge_half, z2);

  /* Use a single polynomial approximation P for both intervals.  */
  svfloat32_t p = sv_horner_4_f32_x (pg, z2, d->poly);
  /* Finalize polynomial: z + z * z2 * P(z2).  */
  p = svmla_x (pg, z, svmul_x (pg, z, z2), p);

  /* asin(|x|) = Q(|x|)         , for |x| < 0.5
		 = pi/2 - 2 Q(|x|), for |x| >= 0.5.  */
  svfloat32_t y = svmad_m (a_ge_half, p, sv_f32 (-2.0), d->pi_over_2f);

  /* Copy sign.  */
  return svreinterpret_f32 (svorr_x (pg, svreinterpret_u32 (y), sign));
}
