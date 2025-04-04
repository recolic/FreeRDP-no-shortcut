/* test_YCoCg.c
 * vi:ts=4 sw=4
 *
 * (c) Copyright 2014 Hewlett-Packard Development Company, L.P.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <freerdp/config.h>

#include <winpr/sysinfo.h>
#include "prim_test.h"
#include <freerdp/utils/profiler.h>

/* ------------------------------------------------------------------------- */
static BOOL test_YCoCgRToRGB_8u_AC4R_func(UINT32 width, UINT32 height)
{
	pstatus_t status = -1;
	BYTE* out_sse = NULL;
	BYTE* in = NULL;
	BYTE* out_c = NULL;
	const UINT32 srcStride = width * 4;
	const UINT32 size = srcStride * height;
	const UINT32 formats[] = { PIXEL_FORMAT_ARGB32, PIXEL_FORMAT_ABGR32, PIXEL_FORMAT_RGBA32,
		                       PIXEL_FORMAT_RGBX32, PIXEL_FORMAT_BGRA32, PIXEL_FORMAT_BGRX32 };
	PROFILER_DEFINE(genericProf)
	PROFILER_DEFINE(optProf)
	in = winpr_aligned_calloc(1, size, 16);
	out_c = winpr_aligned_calloc(1, size, 16);
	out_sse = winpr_aligned_calloc(1, size, 16);

	if (!in || !out_c || !out_sse)
		goto fail;

	winpr_RAND(in, size);

	for (size_t x = 0; x < sizeof(formats) / sizeof(formats[0]); x++)
	{
		const UINT32 format = formats[x];
		const UINT32 dstStride = width * FreeRDPGetBytesPerPixel(format);
		const char* formatName = FreeRDPGetColorFormatName(format);
		PROFILER_CREATE(genericProf, "YCoCgRToRGB_8u_AC4R-GENERIC")
		PROFILER_CREATE(optProf, "YCoCgRToRGB_8u_AC4R-OPT")
		PROFILER_ENTER(genericProf)
		status = generic->YCoCgToRGB_8u_AC4R(in, WINPR_ASSERTING_INT_CAST(int, srcStride), out_c,
		                                     format, WINPR_ASSERTING_INT_CAST(int, dstStride),
		                                     width, height, 2, TRUE);
		PROFILER_EXIT(genericProf)

		if (status != PRIMITIVES_SUCCESS)
			goto loop_fail;

		PROFILER_ENTER(optProf)
		status = optimized->YCoCgToRGB_8u_AC4R(
		    in, WINPR_ASSERTING_INT_CAST(int, srcStride), out_sse, format,
		    WINPR_ASSERTING_INT_CAST(int, dstStride), width, height, 2, TRUE);
		PROFILER_EXIT(optProf)

		if (status != PRIMITIVES_SUCCESS)
			goto loop_fail;

		if (memcmp(out_c, out_sse, 1ULL * dstStride * height) != 0)
		{
			for (size_t i = 0; i < 1ull * width * height; ++i)
			{
				const UINT32 c = FreeRDPReadColor(out_c + 4 * i, format);
				const UINT32 sse = FreeRDPReadColor(out_sse + 4 * i, format);

				if (c != sse)
				{
					printf("optimized->YCoCgRToRGB FAIL[%s] [%" PRIuz "]: 0x%08" PRIx32
					       " -> C 0x%08" PRIx32 " vs optimized 0x%08" PRIx32 "\n",
					       formatName, i, in[i + 1], c, sse);
					status = -1;
				}
			}
		}

		printf("--------------------------- [%s] [%" PRIu32 "x%" PRIu32
		       "] ---------------------------\n",
		       formatName, width, height);
		PROFILER_PRINT_HEADER
		PROFILER_PRINT(genericProf)
		PROFILER_PRINT(optProf)
		PROFILER_PRINT_FOOTER
	loop_fail:
		PROFILER_FREE(genericProf)
		PROFILER_FREE(optProf)

		if (status != PRIMITIVES_SUCCESS)
			goto fail;
	}

fail:
	winpr_aligned_free(in);
	winpr_aligned_free(out_c);
	winpr_aligned_free(out_sse);
	return status == PRIMITIVES_SUCCESS;
}

int TestPrimitivesYCoCg(int argc, char* argv[])
{
	WINPR_UNUSED(argc);
	WINPR_UNUSED(argv);
	prim_test_setup(FALSE);

	/* Random resolution tests */
	if (argc < 2)
	{
		for (UINT32 x = 0; x < 10; x++)
		{
			UINT32 w = 0;
			UINT32 h = 0;

			do
			{
				winpr_RAND(&w, sizeof(w));
				w %= 2048 / 4;
			} while (w < 16);

			do
			{
				winpr_RAND(&h, sizeof(h));
				h %= 2048 / 4;
			} while (h < 16);

			if (!test_YCoCgRToRGB_8u_AC4R_func(w, h))
				return 1;
		}
	}

	/* Test once with full HD/4 */
	if (!test_YCoCgRToRGB_8u_AC4R_func(1920 / 4, 1080 / 4))
		return 1;

	return 0;
}
