// src/midi_parser.c
#include "music_engine.h"

// 读取变长长度（MIDI特有格式，最多4字节）
uint32_t read_variable_length(const uint8_t *data, int *offset, uint32_t data_len) {
    uint32_t value = 0;
    uint8_t byte;
    int count = 0;
    
    do {
        if (*offset >= (int)data_len || ++count > 4) {
            return 0;  // 边界保护：防止越界读取和恶意数据死循环
        }
        byte = data[(*offset)++];
        value = (value << 7) | (byte & 0x7F);
    } while (byte & 0x80);
    
    return value;
}

// 将MIDI音符转换为频率（Hz）
uint16_t midi_note_to_freq(uint8_t note) {
    // A4 = 440Hz = MIDI 69
    // 公式: f = 440 * 2^((note-69)/12)
    if (note < 21 || note > 108) return 0;
    
    // MIDI 21(A0) ~ 108(C8)，标准88键钢琴音域
    static const uint16_t note_freq[] = {
        28,  29,  31,  33,  35,  37,  39,  41,  44,  46,  49,  52,  // A0 ~ B1
        55,  58,  62,  65,  69,  73,  78,  82,  87,  92,  98,  104, // C2 ~ B2
        110, 117, 123, 131, 139, 147, 156, 165, 175, 185, 196, 208, // C3 ~ B3
        220, 233, 247, 262, 277, 294, 311, 330, 349, 370, 392, 415, // C4 ~ B4 (A4=440)
        440, 466, 494, 523, 554, 587, 622, 659, 698, 740, 784, 831, // C5 ~ B5
        880, 932, 988, 1047,1109,1175,1245,1319,1397,1480,1568,1661, // C6 ~ B6
        1760,1865,1976,2093,2217,2349,2489,2637,2794,2960,3136,3322, // C7 ~ B7
        3520,3729,3951,4186                                      // A7 ~ C8
    };
    
    return note_freq[note - 21];
}

// 将音符事件加入队列（由 parse_midi_track 内部调用）
static void add_note_to_queue(uint8_t note, uint8_t velocity,
                              uint32_t start_tick, uint32_t end_tick,
                              MidiParserState *state, EventQueue *queue) {
    uint16_t freq = midi_note_to_freq(note);
    if (freq == 0) return;

    // ticks → ms:  duration_ms = ticks * tempo / (ticks_per_qn * 1000)
    uint32_t tick_diff = end_tick - start_tick;
    uint32_t duration_ms = (uint32_t)((uint64_t)tick_diff * state->tempo
                                      / (state->ticks_per_qn * 1000));
    if (duration_ms < 10)  duration_ms = 10;
    if (duration_ms > 5000) duration_ms = 5000;

    NoteEvent evt = {
        .freq     = freq,
        .duration = (uint16_t)duration_ms,
        .velocity = velocity ? velocity : 64,
        .__pad1   = 0,
        .__pad2   = 0
    };

    int next_head = (queue->head + 1) % EVENT_QUEUE_SIZE;
    if (next_head != queue->tail) {
        queue->buffer[queue->head] = evt;
        queue->head = next_head;
        queue->count++;
    }
}

// 解析MIDI轨道，生成音符事件（配对 Note-On/Off 计算真实时值）
void parse_midi_track(const uint8_t *track_data, uint32_t track_len,
                      MidiParserState *state, EventQueue *queue) {
    int      offset   = 0;
    uint32_t abs_tick = 0;

    // 活跃音符追踪：note → start_tick
    uint32_t note_start[128];
    bool     note_active[128];
    for (int n = 0; n < 128; n++) {
        note_start[n] = 0;
        note_active[n] = false;
    }

    while (offset < (int)track_len) {
        uint32_t delta = read_variable_length(track_data, &offset, track_len);
        abs_tick += delta;

        if (offset >= (int)track_len) break;

        uint8_t status = track_data[offset++];

        // Running status
        if (status < 0x80) {
            offset--;
            if (state->running_status == 0) return;
            status = state->running_status;
        } else {
            state->running_status = status;
        }

        // ===== Meta Event (0xFF) =====
        if (status == 0xFF) {
            if (offset >= (int)track_len) return;
            uint8_t  meta_type = track_data[offset++];
            uint32_t meta_len  = read_variable_length(track_data, &offset, track_len);
            if (offset + meta_len > track_len) return;

            if (meta_type == 0x51 && meta_len == 3) {          // Set Tempo
                state->tempo = ((uint32_t)track_data[offset] << 16)
                             | ((uint32_t)track_data[offset+1] << 8)
                             |  (uint32_t)track_data[offset+2];
            }
            // 0x2F = End of Track — 立即关闭所有活跃音符
            if (meta_type == 0x2F) {
                offset += meta_len;
                break;
            }
            offset += meta_len;
            continue;
        }

        // ===== SysEx (0xF0 / 0xF7) =====
        if (status == 0xF0 || status == 0xF7) {
            uint32_t sys_len = read_variable_length(track_data, &offset, track_len);
            if (offset + sys_len > track_len) return;
            offset += sys_len;
            continue;
        }

        uint8_t cmd = status & 0xF0;

        switch (cmd) {
        case MIDI_NOTE_ON: {
            if (offset + 1 >= (int)track_len) return;
            uint8_t note     = track_data[offset++];
            uint8_t velocity = track_data[offset++];

            if (velocity > 0) {
                // ★ 诊断：直接入队，跳过 Note-On/Off 配对
                uint16_t freq = midi_note_to_freq(note);
                if (freq > 0) {
                    NoteEvent evt = {freq, 300, velocity, 0, 0};
                    int nh = (queue->head + 1) % EVENT_QUEUE_SIZE;
                    if (nh != queue->tail) {
                        queue->buffer[queue->head] = evt;
                        queue->head = nh;
                        queue->count++;
                    }
                }
            }
            break;
        }
        case MIDI_NOTE_OFF: {
            if (offset + 1 >= (int)track_len) return;
            uint8_t note     = track_data[offset++];
            uint8_t velocity = track_data[offset++];  // release velocity, 忽略

            if (note < 128 && note_active[note]) {
                add_note_to_queue(note, 0, note_start[note], abs_tick, state, queue);
                note_active[note] = false;
            }
            break;
        }
        default:
            if (cmd == 0xC0 || cmd == 0xD0)      offset += 1;
            else if (cmd == 0xA0 || cmd == 0xB0 || cmd == 0xE0) offset += 2;
            else return;   // 未知事件，安全退出
        }
    }

    // 关闭所有残留的活跃音符
    for (int n = 0; n < 128; n++) {
        if (note_active[n]) {
            add_note_to_queue(n, 0, note_start[n], abs_tick, state, queue);
            note_active[n] = false;
        }
    }
}