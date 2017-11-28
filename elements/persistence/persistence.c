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

rage_TupleDef const init_param = {
    .name = "persistence",
    .description = "Store/Replay Audio Streams",
    .default_value = NULL,
    .len = 1,
    .items = param_fields
};

rage_ParamDefList const init_params = {
    .len = 1,
    .items = &init_param
};

static rage_AtomDef const path_atom = {
    .type = RAGE_ATOM_STRING, .name = "path",
    .constraints = {.s = RAGE_NOTHING}
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
    .default_value = NULL,
    .len = 3,
    .items = tst_fields
};

rage_NewInstanceSpec elem_describe_ports(rage_Atom ** params) {
    int const n_channels = params[0][0].i;
    rage_InstanceSpec rval;
    rval.controls.len = 1;
    rval.controls.items = &tst_def;
    rage_StreamDef * stream_defs = calloc(n_channels, sizeof(rage_StreamDef));
    for (unsigned i = 0; i < n_channels; i++) {
        stream_defs[i] = RAGE_STREAM_AUDIO;
    }
    rval.inputs.len = n_channels;
    rval.inputs.items = stream_defs;
    rval.outputs.len = n_channels;
    rval.outputs.items = stream_defs;
    return RAGE_SUCCESS(rage_NewInstanceSpec, rval);
}

void elem_free_port_description(rage_InstanceSpec pr) {
    // FIXME: too const requiring cast?
    free((void *) pr.inputs.items);
}

typedef struct {
    SNDFILE * sf;
    SF_INFO sf_info;
    char * open_path;
} sndfile_status;

struct rage_ElementState {
    unsigned n_channels;
    uint32_t frame_size; // Doesn't everyone HAVE to do this?
    uint32_t sample_rate;
    jack_ringbuffer_t ** rec_buffs;
    jack_ringbuffer_t ** play_buffs;
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

rage_NewElementState elem_new(
        uint32_t sample_rate, uint32_t frame_size, rage_Atom ** params) {
    rage_ElementState * ad = malloc(sizeof(rage_ElementState));
    // Not sure I like way the indices tie up here
    ad->n_channels = params[0][0].i;
    ad->frame_size = frame_size;
    ad->sample_rate = sample_rate;
    ad->rec_buffs = calloc(ad->n_channels, sizeof(jack_ringbuffer_t *));
    ad->play_buffs = calloc(ad->n_channels, sizeof(jack_ringbuffer_t *));
    for (uint32_t c = 0; c < ad->n_channels; c++) {
        ad->rec_buffs[c] = jack_ringbuffer_create(2 * frame_size * sizeof(float));
        ad->play_buffs[c] = jack_ringbuffer_create(2 * frame_size * sizeof(float));
    }
    ad->rb_vec = calloc(2 * ad->n_channels, sizeof(float *));
    ad->interleaved_buffer = calloc(ad->n_channels * 2 * frame_size, sizeof(float));
    sndfile_status_init(&ad->sndfile);
    return RAGE_SUCCESS(rage_NewElementState, ad);
}

void elem_free(rage_ElementState * ad) {
    for (uint32_t c = 0; c < ad->n_channels; c++) {
        jack_ringbuffer_free(ad->rec_buffs[c]);
        jack_ringbuffer_free(ad->play_buffs[c]);
    }
    free(ad->rec_buffs);
    free(ad->play_buffs);
    sndfile_status_destroy(&ad->sndfile);
    free(ad->rb_vec);
    free(ad);
}

static inline void zero_fill_outputs(
        rage_ElementState * data, rage_Ports const * ports, uint32_t frame_pos,
        uint32_t chunk_size) {
    for (uint32_t c = 0; c < data->n_channels; c++) {
        memset(
            &ports->outputs[c][frame_pos], 0x00, chunk_size);
    }
}

void elem_process(rage_ElementState * data, rage_TransportState const transport_state, rage_Ports const * ports) {
    rage_InterpolatedValue const * chunk;
    uint32_t step_frames, remaining = data->frame_size;
    uint32_t c, frame_pos = 0;
    size_t chunk_size;
    if (transport_state == RAGE_TRANSPORT_STOPPED) {
        zero_fill_outputs(data, ports, 0, data->frame_size * sizeof(float));
        return;
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
                zero_fill_outputs(
                    data, ports, frame_pos, data->frame_size * sizeof(float));
                break;
        }
        remaining -= step_frames;
        rage_interpolated_view_advance(ports->controls[0], step_frames);
    }
}

// FIXME: doesn't handle mode switching with same path, also could do more
static bool file_path_changed(sndfile_status * const s, char const * const path) {
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
    if (file_path_changed(&data->sndfile, path)) {
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

static inline uint32_t add_chunk_time(uint32_t current_total, uint32_t addition) {
    return (current_total == UINT32_MAX) ? UINT32_MAX : (addition == UINT32_MAX) ? UINT32_MAX : current_total + addition;
}

rage_PreparedFrames elem_prepare(rage_ElementState * data, rage_InterpolatedView ** controls) {
    uint32_t n_prepared_frames = 0;
    size_t slabs[2];
    rage_InterpolatedValue const * chunk = rage_interpolated_view_value(controls[0]);
    while (chunk->valid_for != UINT32_MAX) {
        switch ((PersistanceMode) chunk->value[0].e) {
            case PLAY:
                populate_slabs(
                    slabs, data->n_channels, jack_ringbuffer_get_write_vector,
                    data->rb_vec, data->play_buffs);
                size_t const rb_space = slabs[0] + slabs[1];
                size_t const to_read = (rb_space < chunk->valid_for) ? rb_space : chunk->valid_for;
                rage_PreparedFrames const read_or_fail = read_prep_sndfile(
                    data, chunk->value[1].s, chunk->value[2].frame_no, to_read, data->interleaved_buffer);
                if (RAGE_FAILED(read_or_fail)) {
                    return read_or_fail;
                }
                uint32_t const read = RAGE_SUCCESS_VALUE(read_or_fail);
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
                    rage_interpolated_view_advance(controls[0], n_prepared_frames);
                    return RAGE_SUCCESS(rage_PreparedFrames, n_prepared_frames);
                }
                break;
            case IDLE:
            case REC:
                n_prepared_frames = add_chunk_time(n_prepared_frames, chunk->valid_for);
                break;
        }
        rage_interpolated_view_advance(controls[0], chunk->valid_for);
        chunk = rage_interpolated_view_value(controls[0]);
    }
    assert(chunk->value[0].e == IDLE);
    return RAGE_SUCCESS(rage_PreparedFrames, UINT32_MAX);
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
    if (file_path_changed(s, path)) {
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

rage_PreparedFrames elem_cleanup(rage_ElementState * data, rage_InterpolatedView ** controls) {
    uint32_t frames_until_buffer_full = 0;
    size_t slabs[2] = {};
    size_t rb_left = 0;
    rage_InterpolatedValue const * chunk;
    chunk = rage_interpolated_view_value(controls[0]);
    while (chunk->valid_for != UINT32_MAX) {
        switch ((PersistanceMode) chunk->value[0].e) {
            case IDLE:
            case PLAY:
                frames_until_buffer_full = add_chunk_time(frames_until_buffer_full, chunk->valid_for);
                rage_interpolated_view_advance(controls[0], chunk->valid_for);
                break;
            case REC:
                if (!rb_left) {
                    populate_slabs(
                        slabs, data->n_channels, jack_ringbuffer_get_read_vector,
                        data->rb_vec, data->rec_buffs);
                    rb_left = slabs[0] + slabs[1];
                    interleave(
                        data->interleaved_buffer, (float const**) data->rb_vec,
                        data->n_channels, slabs[0]);
                    interleave(
                        &data->interleaved_buffer[slabs[0] * data->n_channels],
                        (float const **) &data->rb_vec[data->n_channels],
                        data->n_channels, slabs[1]);
                }
                size_t const to_write = (rb_left < chunk->valid_for) ? rb_left : chunk->valid_for;
                rage_PreparedFrames const written_frames = write_buffer_to_file(
                    &data->sndfile, chunk->value[1].s,
                    chunk->value[2].frame_no, to_write,
                    data->interleaved_buffer, data->sample_rate,
                    data->n_channels);
                if (RAGE_FAILED(written_frames)) {
                    return written_frames;
                }
                uint32_t const written = RAGE_SUCCESS_VALUE(written_frames);
                if (written < to_write) {
                    return RAGE_FAILURE(rage_PreparedFrames, "Unable to write all data to file");
                }
                for (uint32_t c = 0; c < data->n_channels; c++) {
                    jack_ringbuffer_read_advance(data->rec_buffs[c], written * sizeof(float));
                }
                rb_left -= written;
                frames_until_buffer_full += written;
                rage_interpolated_view_advance(controls[0], written);
                break;
        }
        chunk = rage_interpolated_view_value(controls[0]);
    }
    if (chunk->valid_for == UINT32_MAX) {
        frames_until_buffer_full = UINT32_MAX;
    }
    assert(chunk->value[0].e == IDLE);
    return RAGE_SUCCESS(rage_PreparedFrames, frames_until_buffer_full);
}

rage_Error elem_clear(rage_ElementState * data, rage_InterpolatedView ** controls, rage_FrameNo to_present) {
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
    return RAGE_OK;
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
