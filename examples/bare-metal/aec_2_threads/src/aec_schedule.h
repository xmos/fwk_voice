#ifndef aec_schedule_h_
#define aec_schedule_h_

#define AEC_THREAD_COUNT   (2)
#define AEC_3_TASKS_AND_CHANNELS_PASSES   (3)
#define AEC_2_TASKS_AND_CHANNELS_PASSES   (2)
#define AEC_1_TASKS_AND_CHANNELS_PASSES   (1)
#define AEC_2_TASKS_PASSES   (1)
#define AEC_3_TASKS_PASSES   (2)
typedef struct {
    int task;
    int channel;
    int is_active;
}par_tasks_and_channels_t;

typedef struct {
    int task;
    int is_active;
}par_tasks_t;

typedef struct {
par_tasks_and_channels_t par_3_tasks_and_channels[AEC_THREAD_COUNT][AEC_3_TASKS_AND_CHANNELS_PASSES];
par_tasks_and_channels_t par_2_tasks_and_channels[AEC_THREAD_COUNT][AEC_2_TASKS_AND_CHANNELS_PASSES];
par_tasks_and_channels_t par_1_tasks_and_channels[AEC_THREAD_COUNT][AEC_1_TASKS_AND_CHANNELS_PASSES];
par_tasks_t par_2_tasks[AEC_THREAD_COUNT][AEC_2_TASKS_PASSES];
par_tasks_t par_3_tasks[AEC_THREAD_COUNT][AEC_3_TASKS_PASSES];
}schedule_t;

#endif /* aec_schedule_h_ */
