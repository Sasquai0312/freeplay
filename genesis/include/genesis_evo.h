/* Native Evolution Lab session. Caller-owned. No allocation, no globals.
 * Portable generators still accept 128 steps. This session is the native
 * path and rejects lengths outside 1..64. */
#ifndef GENESIS_EVO_H
#define GENESIS_EVO_H

#include "genesis.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GEN_EVO_NATIVE_MAX 64

enum {
    GEN_EVO_TURING = 0,
    GEN_EVO_MARKOV = 1,
    GEN_EVO_MUTATE = 2
};

enum {
    GEN_EVO_EMPTY = 0,
    GEN_EVO_VALID = 1,
    GEN_EVO_STALE = 2,
    GEN_EVO_ERROR = 3,
    GEN_EVO_APPLIED = 4
};

typedef struct gen_evo {
    gen_turing committed;
    gen_turing proposed;
    gen_markov_model model;   /* published Markov model */
    gen_markov_model pending; /* learned model waiting for a successful Apply */
    gen_markov_model scratch; /* learn scratch; not a published model */
    uint32_t preview_bind;
    uint32_t live_bind;
    uint32_t turing_sig;
    int16_t last_status;
    uint8_t mode;
    uint8_t phase;
    uint8_t proposed_ok;
    uint8_t pending_ok;
    uint8_t model_ready;
    uint8_t _pad[1];
} gen_evo;

void gen_evo_init(gen_evo *e);
void gen_evo_set_mode(gen_evo *e, uint8_t mode);
void gen_evo_set_live(gen_evo *e, uint32_t bind);
void gen_evo_discard(gen_evo *e);

/* FNV-1a over raw bytes. h is the running hash (start with 2166136261). */
uint32_t gen_evo_hash(uint32_t h, const void *bytes, uint32_t n);

/* 1..64 is native. 0, 65, and 128 are rejected. */
gen_status gen_evo_native_steps(unsigned steps);

/* Rebuild the Turing register from cfg. Writes no events. */
gen_status gen_evo_turing_reinit(gen_evo *e, const gen_turing_cfg *cfg);

/* Preview reads committed state. A register-defining cfg change reinits first.
 * Proposed state is stored separately. Committed bytes stay put. */
gen_status gen_evo_turing_preview(gen_evo *e, const gen_turing_cfg *cfg,
                                  gen_buf *out, uint32_t bind);

/* Preset styles 0..3 fill a temporary chain and do not replace a learned model.
 * Style LEARNED generates from `pending` after a successful learn, or from the
 * published model when pending is empty. It does not substitute a preset. */
gen_status gen_evo_markov_preview(gen_evo *e, const gen_markov_cfg *cfg, uint8_t style,
                                  gen_buf *out, uint32_t bind);

/* Learn into `pending`. Failure leaves `model` and the previous `pending` bytes
 * unchanged and does not mark a new pending publication. */
gen_status gen_evo_markov_learn(gen_evo *e, uint8_t scale, int root,
                                const gen_buf *src, uint32_t gap_ticks);

gen_status gen_evo_mutate_preview(gen_evo *e, const gen_buf *src,
                                  const gen_mutate_req *req, gen_buf *out,
                                  uint32_t bind);

/* native_ok nonzero commits Turing proposed state and a pending Markov model
 * exactly once. Zero leaves committed state unchanged and keeps a VALID preview
 * so the caller can retry. A second success returns PARAM and does not advance. */
gen_status gen_evo_apply_result(gen_evo *e, int native_ok);

/* Native event rules: grid, range, duration, monophonic, step count <= 64. */
gen_status gen_evo_audit(const gen_buf *buf, unsigned steps, uint32_t ticks_per_step);

#ifdef __cplusplus
}
#endif
#endif
