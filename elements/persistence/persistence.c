#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <jack/ringbuffer.h>
#include <sndfile.h>

// This header should probably have a hash of the interface files or something
// dynamically embedded in it:
#include <element_impl.h>
#include <interpolation.h>
#include <error.h>

typedef enum {PLAY, REC} PersistanceMode;

static rage_EnumOpt const mode_enum_opts[] = {
    {.value = PLAY, .name = "Play"},
    {.value = REC, .name = "Rec"}
};

static rage_AtomDef const mode_enum = {
    .type = RAGE_ATOM_ENUM, .name = "mode",
    .constraints = {.e = {.len = 2, .items = mode_enum_opts}}
};

static rage_FieldDef const mode_fields[] = {
    {.name = "mode", .type = &mode_enum}
};

static rage_AtomDef const n_channels = {
    .type = RAGE_ATOM_INT,
    .name = "n_channels",
    .constraints = {.i = {.min = RAGE_JUST(1)}}
};

static rage_FieldDef const channels_fields[] = {
    {.name = "channels", .type = &n_channels}
};

rage_TupleDef const init_tups[] = {
    {
        .name = "n_channels",
        .description = "Number of channels",
        .default_value = NULL,
        .len = 1,
        .items = channels_fields
    },
    {
        .name = "mode",
        .description = "Source or sink",
        .default_value = NULL,
        .len = 1,
        .items = mode_fields
    }
};

rage_ParamDefList const init_params = {
    .len = 2,
    .items = init_tups
};

static rage_AtomDef const path_atom = {
    .type = RAGE_ATOM_STRING, .name = "path",
    .constraints = {.s = RAGE_NOTHING}
};

static rage_AtomDef const time_atom = {
    .type = RAGE_ATOM_TIME, .name = "time"
};

static rage_FieldDef const tst_fields[] = {
    {.name = "file", .type = &path_atom},
    {.name = "offset", .type = &time_atom}
};

static rage_TupleDef const tst_def = {
    .name = "audio_chunk",
    .description = "A section of audio from/to a file",
    .default_value = NULL,
    .len = 2,
    .items = tst_fields
};

#define RAGE_PERSISTANCE_MAX_FRAMES 4095

typedef struct {
    SNDFILE * sf;
    SF_INFO sf_info;
    char * open_path;
} sndfile_status;

struct rage_ElementState {
    unsigned n_channels;
    uint32_t sample_rate;
    jack_ringbuffer_t ** buffs;
    char ** rb_vec;
    sndfile_status sndfile;
    float * interleaved_buffer;
};

static void sndfile_status_init(sndfile_status * const s) {
    s->sf = NULL;
    s->open_path = NULL;
}

static void sndfile_status_destroy(sndfile_status * const s) {
    if (s->sf != NULL) {
        sf_close(s->sf);
    }
    if (s->open_path != NULL) {
        free(s->open_path);
    }
}

struct rage_ElementTypeState {
    int n_channels;
};

rage_NewElementState elem_new(
        rage_ElementTypeState * type_state, uint32_t sample_rate) {
    rage_ElementState * ad = malloc(sizeof(rage_ElementState));
    ad->n_channels = type_state->n_channels;
    ad->sample_rate = sample_rate;
    ad->buffs = calloc(ad->n_channels, sizeof(jack_ringbuffer_t *));
    for (uint32_t c = 0; c < ad->n_channels; c++) {
        // Snaps to the next power of 2 up - 1 byte in size
        ad->buffs[c] = jack_ringbuffer_create(RAGE_PERSISTANCE_MAX_FRAMES * sizeof(float));
    }
    ad->rb_vec = calloc(2 * ad->n_channels, sizeof(float *));
    ad->interleaved_buffer = calloc(ad->n_channels * RAGE_PERSISTANCE_MAX_FRAMES, sizeof(float));
    sndfile_status_init(&ad->sndfile);
    return RAGE_SUCCESS(rage_NewElementState, ad);
}

void elem_free(rage_ElementState * ad) {
    for (uint32_t c = 0; c < ad->n_channels; c++) {
        jack_ringbuffer_free(ad->buffs[c]);
    }
    free(ad->buffs);
    free(ad->interleaved_buffer);
    free(ad->rb_vec);
    sndfile_status_destroy(&ad->sndfile);
    free(ad);
}

static inline void zero_fill_outputs(
        rage_ElementState * data, rage_Ports const * ports, uint32_t frame_pos,
        uint32_t chunk_size) {
    for (uint32_t c = 0; c < data->n_channels; c++) {
        memset(
            &ports->outputs[c][frame_pos], 0x0, chunk_size);
    }
}

static void play_process(
        rage_ElementState * data, rage_TransportState const transport_state,
        uint32_t period_size, rage_Ports const * ports) {
    rage_InterpolatedValue const * chunk;
    uint32_t step_frames, remaining = period_size;
    uint32_t c, frame_pos = 0;
    size_t chunk_size;
    if (transport_state == RAGE_TRANSPORT_STOPPED) {
        zero_fill_outputs(data, ports, 0, period_size * sizeof(float));
        return;
    }
    while (remaining) {
        chunk = rage_interpolated_view_value(ports->controls[0]);
        frame_pos = period_size - remaining;
        step_frames = (remaining > chunk->valid_for) ?
            chunk->valid_for : remaining;
        chunk_size = step_frames * sizeof(float);
        if (strcmp(chunk->value[0].s, "")) {
            for (c = 0; c < data->n_channels; c++) {
                jack_ringbuffer_read(
                    data->buffs[c],
                    (char *) &ports->outputs[c][frame_pos],
                    chunk_size);
            }
        } else {
            zero_fill_outputs(
                data, ports, frame_pos, chunk_size);
        }
        remaining -= step_frames;
        rage_interpolated_view_advance(ports->controls[0], step_frames);
    }
}

static void rec_process(
        rage_ElementState * data, rage_TransportState const transport_state,
        uint32_t period_size, rage_Ports const * ports) {
    rage_InterpolatedValue const * chunk;
    uint32_t step_frames, remaining = period_size;
    uint32_t c, frame_pos = 0;
    size_t chunk_size;
    while (remaining) {
        chunk = rage_interpolated_view_value(ports->controls[0]);
        frame_pos = period_size - remaining;
        step_frames = (remaining > chunk->valid_for) ?
            chunk->valid_for : remaining;
        chunk_size = step_frames * sizeof(float);
        if (strcmp(chunk->value[0].s, "")) {
            for (c = 0; c < data->n_channels; c++) {
                jack_ringbuffer_write(
                    data->buffs[c],
                    (char *) &ports->inputs[c][frame_pos],
                    chunk_size);
            }
        }
        remaining -= step_frames;
        rage_interpolated_view_advance(ports->controls[0], step_frames);
    }
}

static bool file_path_changed(sndfile_status * const s, char const * const path, int mode) {
    if (s->open_path == NULL || strcmp(path, s->open_path)) {
        if (s->sf != NULL) {
            sf_close(s->sf);
        }
        free(s->open_path);
        s->open_path = strdup(path);
        return true;
    }
    return false;
}

typedef RAGE_OR_ERROR(uint32_t) rage_PreparedFrames;

// Should this return rage_PreparedFrames to reduce casting?
static rage_Error check_sfinfo(
        SF_INFO * const info, uint32_t n_channels, uint32_t sample_rate) {
    if (info->channels != n_channels) {
        return RAGE_ERROR("Channel count mismatch");
    }
    if (info->samplerate != sample_rate) {
        return RAGE_ERROR("Sample rate mismatch");
    }
    return RAGE_OK;
}

static rage_PreparedFrames read_prep_sndfile(
        rage_ElementState * const data, char const * const path, size_t pos,
        uint32_t to_read, float * interleaved_buffer) {
    if (file_path_changed(&data->sndfile, path, SFM_READ)) {
        data->sndfile.sf = sf_open(path, SFM_READ, &data->sndfile.sf_info);
        const rage_Error info_check = check_sfinfo(
            &data->sndfile.sf_info, data->n_channels, data->sample_rate);
        if (RAGE_FAILED(info_check)) {
            return RAGE_FAILURE_CAST(rage_PreparedFrames, info_check);
        }
    }
    if (sf_seek(data->sndfile.sf, pos, SEEK_SET) == -1) {
        return RAGE_FAILURE(rage_PreparedFrames, "Seek failed");
    }
    uint32_t read = sf_readf_float(
        data->sndfile.sf, data->interleaved_buffer, to_read);
    if (read < to_read) {
        return RAGE_FAILURE(rage_PreparedFrames, "Insufficient data in file");
    }
    return RAGE_SUCCESS(rage_PreparedFrames, read);
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
        get_vec(rbs[c], vec);
        rb_vec[c] = vec[0].buf;
        slabs[0] = vec[0].len / sizeof(float);
        rb_vec[c + n_channels] = vec[1].buf;
        slabs[1] = vec[1].len / sizeof(float);
    }
}

static rage_Error play_prepare(rage_ElementState * data, rage_InterpolatedView ** controls) {
    size_t slabs[2];
    bool future_values = true;
    do {
        rage_InterpolatedValue const * chunk = rage_interpolated_view_value(controls[0]);
        if (strcmp(chunk->value[0].s, "")) {
                populate_slabs(
                    slabs, data->n_channels, jack_ringbuffer_get_write_vector,
                    data->rb_vec, data->buffs);
                size_t const rb_space = (slabs[0] + slabs[1]) / sizeof(float);
                size_t const to_read = (rb_space < chunk->valid_for) ? rb_space : chunk->valid_for;
                rage_PreparedFrames const read_or_fail = read_prep_sndfile(
                    data, chunk->value[0].s, chunk->value[1].frame_no, to_read, data->interleaved_buffer);
                if (RAGE_FAILED(read_or_fail)) {
                    return RAGE_FAILURE_CAST(rage_Error, read_or_fail);
                }
                uint32_t const read = RAGE_SUCCESS_VALUE(read_or_fail);
                deinterleave(data->interleaved_buffer, (float**) data->rb_vec, data->n_channels, slabs[0]);
                deinterleave(
                    &data->interleaved_buffer[slabs[0] * data->n_channels],
                    (float **) &data->rb_vec[data->n_channels],
                    data->n_channels, slabs[1]);
                for (uint32_t c = 0; c < data->n_channels; c++) {
                    jack_ringbuffer_write_advance(data->buffs[c], read * sizeof(float));
                }
                rage_interpolated_view_advance(controls[0], read);
                if (rb_space <= chunk->valid_for) {
                    return RAGE_OK;
                }
        } else {
            future_values = chunk->valid_for != UINT32_MAX;
            rage_interpolated_view_advance(controls[0], chunk->valid_for);
        }
    } while (future_values);
    return RAGE_OK;
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

static rage_PreparedFrames write_buffer_to_file(
        sndfile_status * const s, char const * const path, uint32_t pos,
        uint32_t to_write, float * interleaved_buffer, uint32_t sample_rate,
        uint32_t n_channels) {
    if (file_path_changed(s, path, SFM_WRITE)) {
        s->sf_info.samplerate = sample_rate;
        s->sf_info.channels = n_channels;
        s->sf_info.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
        s->sf = sf_open(path, SFM_RDWR, &s->sf_info);
        const rage_Error info_check = check_sfinfo(
            &s->sf_info, n_channels, sample_rate);
        if (RAGE_FAILED(info_check)) {
            return RAGE_FAILURE_CAST(rage_PreparedFrames, info_check);
        }
    }
    if (sf_seek(s->sf, pos, SEEK_SET) == -1) {
        return RAGE_FAILURE(rage_PreparedFrames, "Seek failed");
    }
    return RAGE_SUCCESS(rage_PreparedFrames, sf_writef_float(
        s->sf, interleaved_buffer, to_write));
}

static rage_Error play_clear(rage_ElementState * data, rage_InterpolatedView ** controls, rage_FrameNo to_present) {
    // Controls are initialised at the start of the section to clear
    // to_present is the number of frames between that start point and the position after the last write
    // ! Do NOT look at the read pointer, it will be moving during this.
    rage_InterpolatedValue const * chunk;
    // Work out the number of frames that have been inserted in the cleared
    // interval (which must include to the latest sample)
    rage_FrameNo consumed, to_erase = 0;
    while (to_present) {
        chunk = rage_interpolated_view_value(controls[0]);
        consumed = (to_present < chunk->valid_for) ? to_present : chunk->valid_for;
        if (strcmp(chunk->value[0].s, "")) {
            to_erase += consumed;
        }
        to_present -= consumed;
        rage_interpolated_view_advance(controls[0], consumed);
    }
    for (uint32_t i = 0; i < data->n_channels; i++) {
        jack_ringbuffer_t * const rb = data->buffs[i];
        // Roll back the write pointer
        jack_ringbuffer_write_advance(rb, -(to_erase * sizeof(float)));
    }
    return RAGE_OK;
}

static rage_Error rec_cleanup(rage_ElementState * data, rage_InterpolatedView ** controls) {
    size_t slabs[2] = {};
    bool future_values = true;
    populate_slabs(
        slabs, data->n_channels, jack_ringbuffer_get_read_vector,
        data->rb_vec, data->buffs);
    rage_FrameNo interleaved_remaining = slabs[0] + slabs[1];
    interleave(
        data->interleaved_buffer, (float const**) data->rb_vec,
        data->n_channels, slabs[0]);
    interleave(
        &data->interleaved_buffer[slabs[0] * data->n_channels],
        (float const **) &data->rb_vec[data->n_channels],
        data->n_channels, slabs[1]);
    for (uint32_t c = 0; c < data->n_channels; c++) {
        jack_ringbuffer_read_advance(data->buffs[c], interleaved_remaining * sizeof(float));
    }
    do {
        rage_InterpolatedValue const * chunk = rage_interpolated_view_value(controls[0]);
        if (strcmp(chunk->value[0].s, "")) {
            uint32_t const to_write = (interleaved_remaining < chunk->valid_for) ? interleaved_remaining : chunk->valid_for;
            rage_PreparedFrames const written_frames = write_buffer_to_file(
                &data->sndfile, chunk->value[0].s,
                chunk->value[1].frame_no, to_write,
                data->interleaved_buffer, data->sample_rate,
                data->n_channels);
            if (RAGE_FAILED(written_frames)) {
                return RAGE_FAILURE_CAST(rage_Error, written_frames);
            }
            uint32_t const written = RAGE_SUCCESS_VALUE(written_frames);
            if (written < to_write) {
                return RAGE_ERROR("Unable to write all data to file");
            }
            interleaved_remaining -= written;
            rage_interpolated_view_advance(controls[0], written);
            if (!interleaved_remaining) {
                return RAGE_OK;
            }
        } else {
            future_values = chunk->valid_for != UINT32_MAX;
            rage_interpolated_view_advance(controls[0], chunk->valid_for);
        }
    } while (future_values);
    return RAGE_OK;
}

void type_destroy(rage_ElementType * type) {
    free(type->type_state);
    // FIXME: too const requiring cast?
    free((void *) type->spec.inputs.items);
}

rage_Error kind_specialise(rage_ElementType * type, rage_Atom ** params) {
    int const n_channels = params[0][0].i;
    PersistanceMode const mode = params[1][0].e;
    type->type_destroy = type_destroy;
    type->spec.max_uncleaned_frames = RAGE_PERSISTANCE_MAX_FRAMES;
    type->spec.max_period_size = RAGE_PERSISTANCE_MAX_FRAMES / 2;
    type->spec.controls.len = 1;
    type->spec.controls.items = &tst_def;
    rage_StreamDef * stream_defs = calloc(n_channels, sizeof(rage_StreamDef));
    for (unsigned i = 0; i < n_channels; i++) {
        stream_defs[i] = RAGE_STREAM_AUDIO;
    }
    type->state_new = elem_new;
    type->state_free = elem_free;
    type->type_state = malloc(sizeof(rage_ElementTypeState));
    type->type_state->n_channels = n_channels;
    switch (mode) {
        case PLAY:
            type->spec.outputs.len = n_channels;
            type->spec.outputs.items = stream_defs;
            type->process = play_process;
            type->prep = play_prepare;
            type->clear = play_clear;
            break;
        case REC:
            type->spec.inputs.len = n_channels;
            type->spec.inputs.items = stream_defs;
            type->process = rec_process;
            type->clean = rec_cleanup;
            break;
    }
    return RAGE_OK;
}

rage_ElementKind const kind = {
    .parameters = &init_params,
    .specialise = kind_specialise
};
