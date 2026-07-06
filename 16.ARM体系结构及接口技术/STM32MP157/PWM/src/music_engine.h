// src/music_engine.h
#ifndef _MUSIC_ENGINE_H_
#define _MUSIC_ENGINE_H_

#include <stdint.h>
#include <stdbool.h>
#include "stm32mp157_tim.h"

// ===================== MIDI 消息类型 =====================
#define MIDI_NOTE_OFF       0x80
#define MIDI_NOTE_ON        0x90

// ===================== 音符事件结构 =====================
typedef struct {
    uint16_t freq;      // 频率(Hz)
    uint16_t duration;  // 持续时间(ms)
    uint8_t  velocity;  // 力度(0-127)
    uint8_t  __pad1;
    uint16_t __pad2;    // 总 8 字节，数组元素 4 字节对齐
} NoteEvent;

// ===================== 事件队列（循环缓冲区）=====================
#define EVENT_QUEUE_SIZE  256

typedef struct {
    NoteEvent buffer[EVENT_QUEUE_SIZE];
    volatile int head;
    volatile int tail;
    volatile uint32_t count;
} EventQueue;

// ===================== MIDI 文件头结构 =====================
typedef struct {
    uint32_t chunk_type;    // 'MThd'
    uint32_t length;        // =6
    uint16_t format;
    uint16_t tracks;
    uint16_t division;      // ticks per quarter note
} __attribute__((packed)) MThdChunk;

typedef struct {
    uint32_t chunk_type;    // 'MTrk'
    uint32_t length;
} __attribute__((packed)) MTrkChunk;

// ===================== MIDI 解析器状态 =====================
typedef struct {
    uint8_t  running_status;
    uint32_t abs_time;       // 绝对时间(tick)
    uint32_t tempo;          // 微秒/四分音符
    uint32_t ticks_per_qn;   // ticks per quarter note
} MidiParserState;

// ===================== PWM 音频引擎状态 =====================
typedef struct {
    volatile uint32_t is_playing;   // 用 uint32_t 强制 4 字节对齐（MMU 关后非对齐 str 会 data abort）
    volatile NoteEvent current_note;
    volatile uint32_t note_start_time;
    tim2_3_4_5_t *htim;
    uint32_t pwm_channel;
} AudioEngine;

// 全局音频引擎实例（audio_engine.c 定义，main.c 使用）
extern AudioEngine g_audio;

// ===================== PWM 蜂鸣器（寄存器级）=====================
void beep_pwm_init(void);    // 一次性初始化 GPIO + TIM4 基础配置
void beep_pwm_on(void);      // 使能计数器，开始输出
void beep_pwm_off(void);     // 停止计数器

// ===================== 音频引擎 =====================
void audio_engine_init(tim2_3_4_5_t *htim, uint32_t channel);
bool audio_play_note(const NoteEvent *note);
void audio_process_queue(EventQueue *queue);

// ===================== MIDI 解析 =====================
uint32_t read_variable_length(const uint8_t *data, int *offset, uint32_t data_len);
uint16_t midi_note_to_freq(uint8_t note);
void parse_midi_track(const uint8_t *track_data, uint32_t track_len,
                      MidiParserState *state, EventQueue *queue);

// ===================== 工具函数 =====================
uint32_t get_tick_ms(void);
void delay_ms(uint32_t ms);
void delay_us(uint32_t us);
void init_midi_parser(MidiParserState *state);

#endif