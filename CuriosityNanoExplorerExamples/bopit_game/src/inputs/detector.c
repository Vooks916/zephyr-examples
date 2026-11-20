/* SPDX-License-Identifier: Apache-2.0 */
#include <stdlib.h>
#include <zephyr/sys/util.h>
#include "detector.h"

void detector_arm(struct detector *d, int32_t baseline)
{
	d->baseline = baseline;
	d->hits = 0;
}

bool detector_update(struct detector *d, int32_t value)
{
	int32_t delta = value - d->baseline;
	int32_t threshold = d->abs_threshold;
	bool beyond;

	if (d->pct_threshold != 0U) {
		threshold = MAX(threshold, (abs(d->baseline) * d->pct_threshold) / 100);
	}

	switch (d->dir) {
	case DETECT_RISE:
		beyond = delta >= threshold;
		break;
	case DETECT_FALL:
		beyond = -delta >= threshold;
		break;
	default:
		beyond = abs(delta) >= threshold;
		break;
	}

	d->hits = beyond ? MIN(d->hits + 1, 255) : 0;
	return d->hits >= MAX(d->hits_needed, 1);
}
