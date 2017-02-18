typedef struct rage_Barrier rage_Barrier;

rage_Barrier * rage_barrier_new(unsigned char tokens);
void rage_barrier_free(rage_Barrier * b);

void rage_barrier_release_token(rage_Barrier * b);
void rage_barrier_wait(rage_Barrier * b);
