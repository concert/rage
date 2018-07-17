#include "depmap.h"

struct rage_DepMap {
    rage_ConnTerminal src;
    rage_ConnTerminal sink;
    struct rage_DepMap * next;
};

static bool rage_conn_terminal_eq(
        rage_ConnTerminal const * a, rage_ConnTerminal const * b) {
    return a->harness == b->harness && a->idx == b->idx;
}

rage_DepMap * rage_depmap_new() {
    return NULL;
}

void rage_depmap_free(rage_DepMap * dm) {
    while (dm != NULL) {
        rage_DepMap * const nxt = dm->next;
        free(dm);
        dm = nxt;
    }
}

rage_ExtDepMap rage_depmap_connect(
        rage_DepMap * dm, rage_ConnTerminal input, rage_ConnTerminal output) {
    rage_MaybeConnTerminal mct = rage_depmap_input_for(dm, output);
    if (RAGE_IS_JUST(mct)) {
        if (rage_conn_terminal_eq(&RAGE_FROM_JUST(mct), &input)) {
            return RAGE_SUCCESS(rage_ExtDepMap, dm);
        } else {
            return RAGE_FAILURE(
                rage_ExtDepMap,
                "Attempted to add second source to input");
        }
    } else {
        rage_DepMap * new_dm = malloc(sizeof(rage_DepMap));
        new_dm->next = dm;
        new_dm->src = input;
        new_dm->sink = output;
        return RAGE_SUCCESS(rage_ExtDepMap, new_dm);
    }
}

rage_DepMap * rage_depmap_disconnect(
        rage_DepMap * initial, rage_ConnTerminal in, rage_ConnTerminal out) {
    rage_DepMap * c, * prev_con = NULL;
    for (c = initial; c != NULL; c = c->next) {
        if (
                rage_conn_terminal_eq(&c->src, &in) &&
                rage_conn_terminal_eq(&c->sink, &out)) {
            if (prev_con != NULL) {
                prev_con->next = c->next;
            } else {
                initial = c->next;
            }
            free(c);
            break;
        }
        prev_con = c;
    }
    return initial;
}

rage_MaybeConnTerminal rage_depmap_input_for(
        rage_DepMap const * dm, rage_ConnTerminal const output) {
    for (; dm != NULL; dm = dm->next) {
        if (rage_conn_terminal_eq(&dm->sink, &output)) {
            return (rage_MaybeConnTerminal) RAGE_JUST(dm->src);
        }
    }
    return (rage_MaybeConnTerminal) RAGE_NOTHING;
}

typedef RAGE_MAYBE(rage_ConnTerminal const *) rage_MaybeTerminal;
typedef rage_MaybeTerminal (*rage_dm_match)(rage_DepMap const * dm, rage_ConnTerminal * term);

static rage_ConnTerminals * rage_cons_matching(rage_DepMap const * dm, rage_dm_match match, rage_ConnTerminal term) {
    rage_ConnTerminals * rv = NULL;
    for (; dm != NULL; dm = dm->next) {
        rage_MaybeTerminal mt = match(dm, &term);
        if (RAGE_IS_JUST(mt)) {
            rage_ConnTerminals * new_term = malloc(sizeof(rage_ConnTerminals));
            new_term->term = *RAGE_FROM_JUST(mt);
            new_term->next = rv;
            rv = new_term;
        }
    }
    return rv;
}

static rage_MaybeTerminal rage_outputs_to(rage_DepMap const * dm, rage_ConnTerminal * term) {
    if (rage_conn_terminal_eq(&dm->src, term)) {
        return (rage_MaybeTerminal) RAGE_JUST(&dm->sink);
    }
    return (rage_MaybeTerminal) RAGE_NOTHING;
}

rage_ConnTerminals * rage_depmap_outputs_for(
        rage_DepMap const * dm, rage_ConnTerminal input) {
    return rage_cons_matching(dm, rage_outputs_to, input);
}

static rage_MaybeTerminal rage_output_harness(rage_DepMap const * dm, rage_ConnTerminal * term) {
    if (dm->src.harness == term->harness) {
        return (rage_MaybeTerminal) RAGE_JUST(&dm->sink);
    }
    return (rage_MaybeTerminal) RAGE_NOTHING;
}

rage_ConnTerminals * rage_depmap_outputs(
        rage_DepMap const * dm, rage_Harness const * h) {
    return rage_cons_matching(
        dm, rage_output_harness,
        (rage_ConnTerminal) {.harness = (rage_Harness *) h});
}

static rage_MaybeTerminal rage_input_harness(rage_DepMap const * dm, rage_ConnTerminal * term) {
    if (dm->sink.harness == term->harness) {
        return (rage_MaybeTerminal) RAGE_JUST(&dm->src);
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

rage_DepMap * rage_remove_connections_for(
        rage_DepMap * initial, rage_Harness * tgt) {
    rage_DepMap * c, * prev_con = NULL;
    for (c = initial; c != NULL; c = c->next) {
        if (c->src.harness == tgt || c->sink.harness == tgt) {
            if (prev_con) {
                prev_con->next = c->next;
            } else {
                initial = c->next;
            }
            free(c);
        }
        prev_con = c;
    }
    return initial;
}

rage_DepMapIter * rage_depmap_iter(rage_DepMap const * dm) {
    return dm;
}

rage_DepMapIter * rage_depmap_iter_item(
        rage_DepMapIter * dmi,
        rage_ConnTerminal * src, rage_ConnTerminal * sink) {
    *src = dmi->src;
    *sink = dmi->sink;
    return dmi->next;
}
