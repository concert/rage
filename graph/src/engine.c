#include "engine.h"
#include <stdlib.h>
#include <jack/jack.h>

struct rage_Engine {
    jack_client_t * jack_client;
};

rage_NewEngine rage_engine_new() {
    rage_Engine * e = malloc(sizeof(rage_Engine));
    // FIXME: Error handling etc.
    e->jack_client = jack_client_open("rage", JackNoStartServer, NULL);
    RAGE_SUCCEED(rage_NewEngine, e);
}

void rage_engine_free(rage_Engine * engine) {
    // FIXME: this is probably a hint that starting and stopping should be
    // elsewhere as the close has a return code
    jack_client_close(engine->jack_client);
    free(engine);
}
