/* SPDX-License-Identifier: Apache-2.0 */
/* Generic baseline + threshold change detector shared by the analog inputs. */
#ifndef BOPIT_DETECTOR_H_
#define BOPIT_DETECTOR_H_

#include <stdbool.h>
#include <stdint.h>

enum detector_dir { DETECT_RISE, DETECT_FALL, DETECT_EITHER };

struct detector {
	enum detector_dir dir;
	int32_t abs_threshold;   /* minimum change in raw units */
	uint8_t pct_threshold;   /* change relative to baseline (0 = unused) */
	uint8_t hits_needed;     /* consecutive samples beyond threshold */
	int32_t baseline;
	uint8_t hits;
};

void detector_arm(struct detector *d, int32_t baseline);
/* Returns true once the value has differed enough from the baseline. */
bool detector_update(struct detector *d, int32_t value);

#endif
