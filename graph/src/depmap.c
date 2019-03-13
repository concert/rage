#include "depmap.h"

static bool rage_conn_terminal_eq(
        rage_ConnTerminal const * a, rage_ConnTerminal const * b) {
    return a->harness == b->harness && a->idx == b->idx;
}

rage_DepMap * rage_depmap_new() {
    rage_DepMap * dm = malloc(sizeof(rage_DepMap));
    dm->len = 0;
    dm->items = NULL;
    return dm;
}

void rage_depmap_free(rage_DepMap * dm) {
    RAGE_ARRAY_FREE(dm);
}

rage_DepMap * rage_depmap_copy(rage_DepMap const * dm) {
    rage_DepMap * ddm = malloc(sizeof(rage_DepMap));
    ddm->len = dm->len;
    size_t const items_size = sizeof(rage_Connection) * dm->len;
    ddm->items = malloc(items_size);
    memcpy(ddm->items, dm->items, items_size);
    return ddm;
}

rage_ExtDepMap rage_depmap_connect(
        rage_DepMap const * dm, rage_ConnTerminal input, rage_ConnTerminal output) {
    rage_MaybeConnTerminal mct = rage_depmap_input_for(dm, output);
    if (RAGE_IS_JUST(mct)) {
        if (rage_conn_terminal_eq(&RAGE_FROM_JUST(mct), &input)) {
            return RAGE_FAILURE(rage_ExtDepMap, "Already connected");
        } else {
            return RAGE_FAILURE(
                rage_ExtDepMap,
                "Attempted to add second source to input");
        }
    } else {
        rage_DepMap * new_dm = malloc(sizeof(rage_DepMap));
        new_dm->len = dm->len + 1;
        new_dm->items = calloc(new_dm->len, sizeof(rage_Connection));
        memcpy(new_dm->items, dm->items, sizeof(rage_Connection) * dm->len);
        new_dm->items[dm->len].src = input;
        new_dm->items[dm->len].sink = output;
        return RAGE_SUCCESS(rage_ExtDepMap, new_dm);
    }
}

rage_ExtDepMap rage_depmap_disconnect(
        rage_DepMap const * dm, rage_ConnTerminal in, rage_ConnTerminal out) {
    for (uint32_t i = 0; i < dm->len; i++) {
        if (
                rage_conn_terminal_eq(&dm->items[i].src, &in) &&
                rage_conn_terminal_eq(&dm->items[i].sink, &out)) {
            rage_DepMap * new_dm = malloc(sizeof(rage_DepMap));
            new_dm->len = dm->len - 1;
            new_dm->items = calloc(new_dm->len, sizeof(rage_Connection));
            memcpy(new_dm->items, dm->items, sizeof(rage_Connection) * i);
            memcpy(new_dm->items, &dm->items[i], sizeof(rage_Connection) * (new_dm->len - i));
            return RAGE_SUCCESS(rage_ExtDepMap, new_dm);
        }
    }
    return RAGE_FAILURE(rage_ExtDepMap, "Were not connected");
}

rage_MaybeConnTerminal rage_depmap_input_for(
        rage_DepMap const * dm, rage_ConnTerminal const output) {
    for (uint32_t i = 0; i < dm->len; i++) {
        if (rage_conn_terminal_eq(&dm->items[i].sink, &output)) {
            return (rage_MaybeConnTerminal) RAGE_JUST(dm->items[i].src);
        }
    }
    return (rage_MaybeConnTerminal) RAGE_NOTHING;
}

typedef RAGE_MAYBE(rage_ConnTerminal const *) rage_MaybeTerminal;
typedef rage_MaybeTerminal (*rage_dm_match)(
    rage_Connection const * conn, rage_ConnTerminal * term);

static rage_ConnTerminals * rage_cons_matching(
        rage_DepMap const * dm, rage_dm_match match, rage_ConnTerminal term) {
    rage_ConnTerminals * rv = NULL;
    for (uint32_t i = 0; i < dm->len; i++) {
        rage_MaybeTerminal mt = match(&dm->items[i], &term);
        if (RAGE_IS_JUST(mt)) {
            rage_ConnTerminals * new_term = malloc(sizeof(rage_ConnTerminals));
            new_term->term = *RAGE_FROM_JUST(mt);
            new_term->next = rv;
            rv = new_term;
        }
    }
    return rv;
}

static rage_MaybeTerminal rage_outputs_to(rage_Connection const * conn, rage_ConnTerminal * term) {
    if (rage_conn_terminal_eq(&conn->src, term)) {
        return (rage_MaybeTerminal) RAGE_JUST(&conn->sink);
    }
    return (rage_MaybeTerminal) RAGE_NOTHING;
}

rage_ConnTerminals * rage_depmap_outputs_for(
        rage_DepMap const * dm, rage_ConnTerminal input) {
    return rage_cons_matching(dm, rage_outputs_to, input);
}

static rage_MaybeTerminal rage_output_harness(rage_Connection const * conn, rage_ConnTerminal * term) {
    if (conn->src.harness == term->harness) {
        return (rage_MaybeTerminal) RAGE_JUST(&conn->sink);
    }
    return (rage_MaybeTerminal) RAGE_NOTHING;
}

rage_ConnTerminals * rage_depmap_outputs(
        rage_DepMap const * dm, rage_Harness const * h) {
    return rage_cons_matching(
        dm, rage_output_harness,
        (rage_ConnTerminal) {.harness = (rage_Harness *) h});
}

static rage_MaybeTerminal rage_input_harness(rage_Connection const * conn, rage_ConnTerminal * term) {
    if (conn->sink.harness == term->harness) {
        return (rage_MaybeTerminal) RAGE_JUST(&conn->src);
    }
    return (rage_MaybeTerminal) RAGE_NOTHING;
}

rage_ConnTerminals * rage_depmap_inputs(
        rage_DepMap const * dm, rage_Harness const * h) {
    return rage_cons_matching(
        dm, rage_input_harness,
        (rage_ConnTerminal) {.harness = (rage_Harness *) h});
}

void rage_conn_terms_free(rage_ConnTerminals * ct) {
    while (ct != NULL) {
        rage_ConnTerminals * const nxt = ct->next;
        free(ct);
        ct = nxt;
    }
}

void rage_remove_connections_for(
        rage_DepMap * dm, rage_Harness const * const tgt,
        uint32_t const n_protected_ins, uint32_t const n_protected_outs) {
    uint32_t n_matched = 0;
    for (uint32_t i = 0; i < dm->len; i++) {
        rage_Connection const * const c = &dm->items[i];
        if (
                (c->src.harness == tgt && c->src.idx >= n_protected_ins) ||
                (c->sink.harness == tgt && c->sink.idx >= n_protected_outs)) {
            n_matched++;
        } else {
            dm->items[i - n_matched] = dm->items[i];
        }
    }
    dm->len -= n_matched;
}
