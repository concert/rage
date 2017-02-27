#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <jack/ringbuffer.h>
#include <sndfile.h>

// FIXME: these includes should be "systemy"
// This header should probably have a hash of the interface files or something
// dynamically embedded in it:
#include "element_impl.h"
#include "interpolation.h"
#include "error.h"

static rage_AtomDef const n_channels = {
    .type = RAGE_ATOM_INT,
    .name = "n_channels",
    .constraints = {.i = {.min = RAGE_JUST(1)}}
};

static rage_FieldDef const param_fields[] = {
    {.name = "channels", .type = &n_channels}
};

rage_TupleDef const init_params = {
    .name = "persistence",
    .description = "Store/Replay Audio Streams",
    .liberty = RAGE_LIBERTY_MUST_PROVIDE,
    .len = 1,
    .items = param_fields
};

static rage_AtomDef const path_atom = {
    .type = RAGE_ATOM_STRING, .name = "path",
    .constraints = {.s = RAGE_JUST(".*")}
};

static rage_AtomDef const time_atom = {
    .type = RAGE_ATOM_TIME, .name = "Time"
};

typedef enum {IDLE, PLAY, REC} PersistanceMode;

static rage_EnumOpt const mode_enum_opts[] = {
    {.value = IDLE, .name = "Idle"},
    {.value = PLAY, .name = "Play"},
    {.value = REC, .name = "Rec"}
};

static rage_AtomDef const mode_enum = {
    .type = RAGE_ATOM_ENUM, .name = "mode",
    .constraints = {.e = {.len = 3, .items = mode_enum_opts}}
};

static rage_FieldDef const tst_fields[] = {
    {.name = "mode", .type = &mode_enum},
    {.name = "file", .type = &path_atom},
    {.name = "offset", .type = &time_atom}
};

static rage_TupleDef const tst_def = {
    .name = "audio_chunk",
    .description = "A section of audio from/to a file",
    .liberty = RAGE_LIBERTY_MAY_PROVIDE,
    .len = 3,
    .items = tst_fields
};

rage_ProcessRequirements elem_describe_ports(rage_Atom * params) {
    rage_ProcessRequirements rval;
    rval.controls.len = 1;
    rval.controls.items = &tst_def;
    rage_StreamDef * stream_defs = calloc(params[0].i, sizeof(rage_StreamDef));
    for (unsigned i = 0; i < params[0].i; i++) {
        stream_defs[i] = RAGE_STREAM_AUDIO;
    }
    rval.inputs.len = params[0].i;
    rval.inputs.items = stream_defs;
    rval.outputs.len = params[0].i;
    rval.outputs.items = stream_defs;
    return rval;
}

void elem_free_port_description(rage_ProcessRequirements pr) {
    // FIXME: too const requiring cast?
    free((void *) pr.inputs.items);
}

typedef struct {
    unsigned n_channels;
    uint32_t frame_size; // Doesn't everyone HAVE to do this?
    jack_ringbuffer_t ** rec_buffs;
    jack_ringbuffer_t ** play_buffs;
    char ** rb_vec;
    SNDFILE * sf;
    SF_INFO sf_info;
    float * interleaved_buffer;
} persist_state;

rage_NewElementState elem_new(
        uint32_t sample_rate, uint32_t frame_size, rage_Atom * params) {
    persist_state * ad = malloc(sizeof(persist_state));
    // Not sure I like way the indices tie up here
    ad->n_channels = params[0].i;
    ad->frame_size = frame_size;
    ad->rec_buffs = calloc(ad->n_channels, sizeof(jack_ringbuffer_t *));
    ad->play_buffs = calloc(ad->n_channels, sizeof(jack_ringbuffer_t *));
    for (uint32_t c = 0; c < ad->n_channels; c++) {
        ad->rec_buffs[c] = jack_ringbuffer_create(2 * frame_size * sizeof(float));
        ad->play_buffs[c] = jack_ringbuffer_create(2 * frame_size * sizeof(float));
    }
    ad->sf = NULL;
    ad->rb_vec = calloc(2 * ad->n_channels, sizeof(float *));
    ad->interleaved_buffer = calloc(ad->n_channels * 2 * frame_size, sizeof(float));
    RAGE_SUCCEED(rage_NewElementState, (void *) ad);
}

void elem_free(void * state) {
    persist_state * ad = state;
    for (uint32_t c = 0; c < ad->n_channels; c++) {
        jack_ringbuffer_free(ad->rec_buffs[c]);
        jack_ringbuffer_free(ad->play_buffs[c]);
    }
    free(ad->rec_buffs);
    free(ad->play_buffs);
    if (ad->sf != NULL) {
        sf_close(ad->sf);
    }
    free(ad->rb_vec);
    free(ad);
}

rage_Error elem_process(void * state, rage_TransportState const transport_state, rage_Ports const * ports) {
    persist_state const * const data = (persist_state *) state;
    rage_InterpolatedValue const * chunk;
    uint32_t step_frames, remaining = data->frame_size;
    uint32_t c, frame_pos = 0;
    size_t chunk_size;
    if (transport_state == RAGE_TRANSPORT_STOPPED) {
       // FIXME: V. similar to idle! 
        for (c = 0; c < data->n_channels; c++) {
            memset(ports->outputs[c], 0x00, data->frame_size * sizeof(float));
        }
        RAGE_OK
    }
    while (remaining) {
        chunk = rage_interpolated_view_value(ports->controls[0]);
        frame_pos = data->frame_size - remaining;
        step_frames = (remaining > chunk->valid_for) ?
            chunk->valid_for : remaining;
        chunk_size = step_frames * sizeof(float);
        switch ((PersistanceMode) chunk->value[0].e) {
            case PLAY:
                for (c = 0; c < data->n_channels; c++) {
                    jack_ringbuffer_read(
                        data->play_buffs[c],
                        (char *) &ports->outputs[c][frame_pos],
                        chunk_size);
                }
                break;
            case REC:
                for (c = 0; c < data->n_channels; c++) {
                    jack_ringbuffer_write(
                        data->rec_buffs[c],
                        (char *) &ports->inputs[c][frame_pos],
                        chunk_size);
                }
            case IDLE:
                for (c = 0; c < data->n_channels; c++) {
                    memset(
                        &ports->outputs[c][frame_pos], 0x00, chunk_size);
                }
                break;
        }
        remaining -= step_frames;
        rage_interpolated_view_advance(ports->controls[0], step_frames);
    }
    RAGE_OK
}

// FIXME: use this or similar to improve efficiency
typedef struct {
    SF_INFO info;
    char * path;
} sndfile_status;

static SNDFILE * prep_sndfile(
        SNDFILE * sf, SF_INFO * info, char const * path, size_t pos) {
    if (sf != NULL) {
        sf_close(sf);
    }
    sf = sf_open(path, SFM_READ, info);
    // FIXME: may fail
    sf_seek(sf, pos, SEEK_SET);
    return sf;
}

static void deinterleave(
        float const * interleaved, float ** separate,
        uint32_t const n_channels, size_t const n_frames) {
    for (size_t f_no = 0; f_no < n_frames; f_no++) {
        for (uint32_t c = 0; c < n_channels; c++) {
            separate[c][f_no] = interleaved[(f_no * n_channels) + c];
        }
    }
}

// jack_ringbuffer_get_(read/write)_vector
typedef void (*rb_vec_getter)(const jack_ringbuffer_t *, jack_ringbuffer_data_t *);

static void populate_slabs(
        size_t * slabs, uint32_t n_channels,
        rb_vec_getter get_vec,
        char ** rb_vec, jack_ringbuffer_t ** rbs) {
    for (uint32_t c = 0; c < n_channels; c++) {
        jack_ringbuffer_data_t vec[2];
        jack_ringbuffer_get_read_vector(rbs[c], vec);
        rb_vec[c] = vec[0].buf;
        slabs[0] = vec[0].len / sizeof(float);
        rb_vec[c + n_channels] = vec[1].buf;
        slabs[1] = vec[1].len / sizeof(float);
    }
}

rage_PreparedFrames elem_prepare(void * state, rage_InterpolatedView ** controls) {
    persist_state * const data = (persist_state *) state;
    uint32_t n_prepared_frames = 0;
    rage_InterpolatedValue const * chunk = rage_interpolated_view_value(controls[0]);
    while (chunk->valid_for != UINT32_MAX) {
        switch ((PersistanceMode) chunk->value[0].e) {
            case PLAY:
                data->sf = prep_sndfile(
                    data->sf, &data->sf_info, chunk->value[1].s, chunk->value[2].frame_no);
                size_t slabs[2];
                populate_slabs(
                    slabs, data->n_channels, jack_ringbuffer_get_write_vector,
                    data->rb_vec, data->play_buffs);
                size_t const rb_space = slabs[0] + slabs[1];
                size_t const to_read = (rb_space < chunk->valid_for) ? rb_space : chunk->valid_for;
                sf_count_t const read = sf_readf_float(
                    data->sf, data->interleaved_buffer, to_read);
                if (read < to_read) {
                    RAGE_FAIL(rage_PreparedFrames, "Insufficient data in file")
                }
                deinterleave(data->interleaved_buffer, (float**) data->rb_vec, data->n_channels, slabs[0]);
                deinterleave(
                    &data->interleaved_buffer[slabs[0] * data->n_channels],
                    (float **) &data->rb_vec[data->n_channels],
                    data->n_channels, slabs[1]);
                for (uint32_t c = 0; c < data->n_channels; c++) {
                    jack_ringbuffer_write_advance(data->play_buffs[c], read * sizeof(float));
                }
                n_prepared_frames += read;
                if (rb_space <= chunk->valid_for) {
                    RAGE_SUCCEED(rage_PreparedFrames, n_prepared_frames);
                }
                break;
            case IDLE:
            case REC:
                // FIXME: worry about overflow?
                n_prepared_frames += chunk->valid_for;
                break;
        }
        rage_interpolated_view_advance(controls[0], chunk->valid_for);
        chunk = rage_interpolated_view_value(controls[0]);
    }
    assert(chunk->value[0].e == IDLE);
    RAGE_SUCCEED(rage_PreparedFrames, UINT32_MAX)
}

static void interleave(
        float * interleaved, float const ** separate,
        uint32_t const n_channels, size_t const n_frames) {
    for (size_t f_no = 0; f_no < n_frames; f_no++) {
        for (uint32_t c = 0; c < n_channels; c++) {
            interleaved[(f_no * n_channels) + c] = separate[c][f_no];
        }
    }
}

rage_PreparedFrames elem_cleanup(void * state, rage_InterpolatedView ** controls) {
    persist_state * const data = (persist_state *) state;
    size_t slabs[2];
    populate_slabs(
        slabs, data->n_channels, jack_ringbuffer_get_read_vector,
        data->rb_vec, data->rec_buffs);
    size_t const rb_space = slabs[0] + slabs[1];
    interleave(
        data->interleaved_buffer, (float const**) data->rb_vec,
        data->n_channels, slabs[0]);
    interleave(
        &data->interleaved_buffer[slabs[0] * data->n_channels],
        (float const **) &data->rb_vec[data->n_channels],
        data->n_channels, slabs[1]);
    size_t rb_left = rb_space;
    rage_InterpolatedValue const * chunk;
    while (rb_left) {
        chunk = rage_interpolated_view_value(controls[0]);
        switch ((PersistanceMode) chunk->value[0].e) {
            case REC:
                data->sf = prep_sndfile(
                    data->sf, &data->sf_info, chunk->value[1].s, chunk->value[2].frame_no);
                size_t const to_write = (rb_left < chunk->valid_for) ? rb_left : chunk->valid_for;
                sf_count_t const written = sf_writef_float(
                    data->sf, data->interleaved_buffer, to_write);
                if (written < to_write) {
                    RAGE_FAIL(rage_PreparedFrames, "Unable to write all data to file")
                }
                rb_left -= written;
                break;
            case IDLE:
            case PLAY:
                break;
        }
        rage_interpolated_view_advance(controls[0], chunk->valid_for);
    }
    for (uint32_t c = 0; c < data->n_channels; c++) {
        jack_ringbuffer_read_advance(data->rec_buffs[c], rb_space * sizeof(float));
    }
    uint32_t n_before_rec = 0;
    while (chunk->value[0].e != REC) {
        n_before_rec += chunk->valid_for;
        chunk = rage_interpolated_view_value(controls[0]);
        rage_interpolated_view_advance(controls[0], chunk->valid_for);
    }
    RAGE_SUCCEED(rage_PreparedFrames, rb_space + n_before_rec)
}

rage_Error elem_clear(void * state, rage_InterpolatedView ** controls, rage_FrameNo to_present) {
    // Controls are initialised at the start of the section to clear
    // to_present is the number of frames between that start point and the position after the last write
    persist_state * const data = (persist_state *) state;
    // ! Do NOT look at the read pointer, it will be moving during this.
    rage_InterpolatedValue const * chunk;
    // Work out the number of frames that have been inserted in the cleared
    // interval (which must include to the latest sample)
    rage_FrameNo consumed, to_erase = 0;
    while (to_present) {
        chunk = rage_interpolated_view_value(controls[0]);
        consumed = (consumed < chunk->valid_for) ? consumed : chunk->valid_for;
        switch ((PersistanceMode) chunk->value[0].e) {
            case PLAY:
                to_erase += consumed;
            case IDLE:
            case REC:
                to_present -= consumed;
        }
        rage_interpolated_view_advance(controls[0], consumed);
    }
    for (uint32_t i = 0; i < data->n_channels; i++) {
        jack_ringbuffer_t * const rb = data->play_buffs[i];
        // Roll back the write pointer
        jack_ringbuffer_write_advance(rb, -to_erase);
    }
    RAGE_OK
}

rage_ElementType const elem_info = {
    .parameters = &init_params,
    .state_new = elem_new,
    .state_free = elem_free,
    .get_ports = elem_describe_ports,
    .free_ports = elem_free_port_description,
    .process = elem_process,
    .prep = elem_prepare,
    .clear = elem_clear,
    .clean = elem_cleanup
};
