#ifndef FSM_H_
#define FSM_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct fsm_t fsm_t;
typedef bool (*fsm_input_func_t)(const fsm_t * const);
typedef void (*fsm_output_func_t)(fsm_t * const);

typedef struct fsm_trans_t {
    int orig_state;
    fsm_input_func_t in;
    int dest_state;
    fsm_output_func_t out;
} fsm_trans_t;

struct fsm_t {
    int current_state;
    fsm_trans_t * p_tt;
};

fsm_t *fsm_new(fsm_trans_t * const p_tt);
void fsm_destroy(fsm_t * const p_fsm);
void fsm_init(fsm_t * const p_fsm, fsm_trans_t * const p_tt);
int fsm_get_state(const fsm_t * const p_fsm);
void fsm_set_state(fsm_t * const p_fsm, int state);
void fsm_fire(fsm_t * const p_fsm);

#ifdef __cplusplus
}
#endif

#endif