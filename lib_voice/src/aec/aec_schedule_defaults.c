
#include "aec_schedule.h"

#if (AEC_LIB_MAX_CHANNELS == 2)
const aec_task_distribution_t aec_tdist_chans2_threads1 = {
	.thread_count = 1,
	.passes_for_3_tasks_and_channels = 6,
	.par_3_tasks_and_channels = {
		{{0, 0, 1},{1, 0, 1},{2, 0, 1},{0, 1, 1},{1, 1, 1},{2, 1, 1},},
	},
	.passes_for_2_tasks_and_channels = 4,
	.par_2_tasks_and_channels = {
		{{0, 0, 1},{1, 0, 1},{0, 1, 1},{1, 1, 1},},
	},
	.passes_for_1_tasks_and_channels = 2,
	.par_1_tasks_and_channels = {
		{{0, 0, 1},{0, 1, 1},},
	},
	.passes_for_3_tasks = 3,
	.par_3_tasks = {
		{{0, 1},{1, 1},{2, 1},},
	},
	.passes_for_2_tasks = 2,
	.par_2_tasks = {
		{{0, 1},{1, 1},},
	},
};

const aec_task_distribution_t aec_tdist_chans2_threads2 = {
	.thread_count = 2,
	.passes_for_3_tasks_and_channels = 3,
	.par_3_tasks_and_channels = {
		{{0, 0, 1},{2, 0, 1},{1, 1, 1},},
		{{1, 0, 1},{0, 1, 1},{2, 1, 1},},
	},
	.passes_for_2_tasks_and_channels = 2,
	.par_2_tasks_and_channels = {
		{{0, 0, 1},{0, 1, 1},},
		{{1, 0, 1},{1, 1, 1},},
	},
	.passes_for_1_tasks_and_channels = 1,
	.par_1_tasks_and_channels = {
		{{0, 0, 1},},
		{{0, 1, 1},},
	},
	.passes_for_3_tasks = 2,
	.par_3_tasks = {
		{{0, 1},{2, 1},},
		{{1, 1},{0, 0},},
	},
	.passes_for_2_tasks = 1,
	.par_2_tasks = {
		{{0, 1},},
		{{1, 1},},
	},
};
#endif
