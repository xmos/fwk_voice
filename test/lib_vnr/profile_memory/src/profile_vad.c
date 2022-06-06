
#include "vad_api.h"

vad_state_t state;

int main(int argc, char** argv) {    
    vad_init(&state);
    int32_t input[VAD_FRAME_ADVANCE] = {0}; 
    uint8_t vad = vad_probability_voice(input, &state);
    return (int)vad;
}
