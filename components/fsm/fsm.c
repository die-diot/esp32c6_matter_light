#include <stdlib.h>

#include "fsm.h"

fsm_t *fsm_new(fsm_trans_t * const p_tt)
{
    if (p_tt == NULL) {
        return NULL;
    }
    if ((p_tt->orig_state == -1) || (p_tt->in == NULL) || (p_tt->dest_state == -1)) {
        return NULL;
    }

    fsm_t *p_fsm = (fsm_t *)malloc(sizeof(fsm_t));
    if (p_fsm != NULL) {
        fsm_init(p_fsm, p_tt);
    }
    return p_fsm;
}

void fsm_destroy(fsm_t * const p_fsm)
{
    free(p_fsm);
}

void fsm_init(fsm_t * const p_fsm, fsm_trans_t * const p_tt)
{
    if (p_tt != NULL) {
        p_fsm->p_tt = p_tt;
        p_fsm->current_state = p_tt->orig_state;
    }
}

int fsm_get_state(const fsm_t * const p_fsm)
{
    return p_fsm->current_state;
}

void fsm_set_state(fsm_t * const p_fsm, int state)
{
    p_fsm->current_state = state;
}

void fsm_fire(fsm_t * const p_fsm)
{
    fsm_trans_t *p_t;
    for (p_t = p_fsm->p_tt; p_t->orig_state >= 0; ++p_t) {
        if ((p_fsm->current_state == p_t->orig_state) && p_t->in(p_fsm)) {
            p_fsm->current_state = p_t->dest_state;
            if (p_t->out) {
                p_t->out(p_fsm);
            }
            break;
        }
    }
}
