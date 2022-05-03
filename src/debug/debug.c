#include <shared/api.h>
#include <shared/types.h>
#include <shared/input.h>

void pre_update(f32 t, DebugMemory *memory, Input *input, RenderCommands *frame) {
    /* Recording input */
    if (input->active[DEBUG_INPUT_RECORD_START]) {
        if (memory->record_state == IDLE) {
            memory->platform.log(LOG_INFO, "Starting recording");
            memory->record_state = RECORDING;
        }
    }
    if (input->active[DEBUG_INPUT_RECORD_STOP]) {
        if (memory->record_state == RECORDING) {
            memory->platform.log(LOG_INFO, "Stopping recording");
            memory->record_state = IDLE;
        }
    }
    if (input->active[DEBUG_INPUT_REPLAY_START]) {
        if (memory->record_state == IDLE) {
            memory->platform.log(LOG_INFO, "Starting replay");
            memory->record_state = REPLAYING;
        }
    }
    if (input->active[DEBUG_INPUT_REPLAY_STOP]) {
        if (memory->record_state == REPLAYING) {
            memory->platform.log(LOG_INFO, "Stopping replay");
            memory->record_state = IDLE;
            memcpy(memory->active_game_memory, &memory->replay_old_memory, sizeof(GameMemory));
            memcpy(memory->active_game_input,  &memory->replay_old_input,  sizeof(Input));
        }
    }

    /* Handle recording of input */
    if (!memory->is_record_file_open && (memory->record_state == RECORDING ||
                                         memory->record_state == REPLAYING)) {
        const char *mode = (memory->record_state == RECORDING) ? "w" : "r";
        memory->record_file = memory->platform.file_open("input_recording", mode);
        memory->is_record_file_open = true;
        memory->platform.log(LOG_INFO, "opening fd");
    }
    if (memory->record_state == RECORDING) {
        RecordData r;
        memcpy(&r.input, memory->active_game_input, sizeof(Input));
        memcpy(&r.game_memory, memory->active_game_memory, sizeof(GameMemory));
        memory->platform.file_write(memory->record_file, &r, sizeof(RecordData), 1);
    } else if (memory->record_state == REPLAYING) {
        if (!memory->replay_memory) {
            u64 file_size = memory->platform.file_size(memory->record_file);
            u64 item_size = sizeof(RecordData);
            memory->replay_count = file_size/item_size;
            memory->replay_memory = memory->platform.allocate_memory(file_size);
            memory->platform.file_read(memory->record_file, memory->replay_memory, file_size, 1);

            memory->replay_old_memory = *memory->active_game_memory;
            memory->replay_old_input = *memory->active_game_input;
            memcpy(&memory->replay_old_memory, memory->active_game_memory, sizeof(GameMemory));
            memcpy(&memory->replay_old_input,  memory->active_game_input,  sizeof(Input));
        }

        memcpy(memory->active_game_memory, &memory->replay_memory[memory->replay_index].game_memory, sizeof(GameMemory));
        memcpy(memory->active_game_input,  &memory->replay_memory[memory->replay_index].input,       sizeof(Input));

        memory->replay_index = (memory->replay_index + 1) % memory->replay_count;
        memory->platform.log(LOG_INFO, "Replay frame: %llu/%llu", memory->replay_index, memory->replay_count);
    } else if (memory->record_state == IDLE && memory->is_record_file_open) {
        memory->platform.log(LOG_INFO, "closing fd");
        memory->platform.file_close(memory->record_file);
        memory->is_record_file_open = false;
        if (memory->replay_memory) {
            memory->platform.free_memory(memory->replay_memory);
        }
    }
}

void post_update(f32 t, DebugMemory *memory, Input *input, RenderCommands *frame) {
    pushText(frame, VEC2(0,0), RGB(0,0,0), "wow!!!! :)");
    pushText(frame, VEC2(-0.9f,-0.8f), RGB(0,0,0), "wow!!!! :)");
}
