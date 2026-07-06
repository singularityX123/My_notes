// main.c - PWM 音乐播放（裸机寄存器版本，无 HAL / 无 U-Boot）
#include "stm32mp157_gpio.h"
#include "stm32mp157_rcc.h"
#include "stm32mp157_tim.h"
#include "src/music_engine.h"

// ========== 曲库 ==========
#include "MIDI_lib/song1_mid.h"

typedef struct {
    const uint8_t *data;
    uint32_t       size;
} MidiSong;

static const MidiSong g_playlist[] = {
    { song1_mid, song1_mid_len },
};
// ====================== 曲库 ======================

// ===================== 看门狗寄存器 =====================
#define IWDG1_KR  (*(volatile uint32_t *)0x5A002000)
#define IWDG2_KR  (*(volatile uint32_t *)0x5A004000)

static inline void iwdg_refresh(void) {
    IWDG1_KR = 0xAAAA;
    IWDG2_KR = 0xAAAA;
}

// ===================== 全局对象 =====================
static EventQueue g_event_queue __attribute__((section(".data"))) = {0};
static MidiParserState g_midi_state __attribute__((section(".data"))) = {0};
static volatile uint32_t g_sys_tick __attribute__((section(".data"))) = {0};

// ===================== 延时函数 ======================
void delay_us(uint32_t us) { // TODO 依赖具体平台
    while (us--) {
        asm volatile(
            "ldr r4, =1500\n"
            "1: subs r4, r4, #1\n"
            "bne 1b\n"
            : : : "r4", "cc"
        );
    }
}

void delay_ms(uint32_t ms) { // TODO 依赖具体平台
    while (ms--) {
        delay_us(1000);
        g_sys_tick++;
        iwdg_refresh();
    }
}

// ===================== 系统 tick =====================
uint32_t get_tick_ms(void) {
    return g_sys_tick;
}

// ===================== MIDI 解析器 =====================
void init_midi_parser(MidiParserState *state) {
    state->running_status = 0;
    state->abs_time = 0;
    state->tempo = 500000;       // 默认 120 BPM
    state->ticks_per_qn = 480;   // 标准 MIDI
}

// ===================== MIDI 文件播放 =====================
// 从字节数组安全读取大端 uint32 / uint16（避免非对齐访问崩溃）
static inline uint32_t read_u32be(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16)
         | ((uint32_t)p[2] << 8)  |  (uint32_t)p[3];
}
static inline uint16_t read_u16be(const uint8_t *p) {
    return ((uint16_t)p[0] << 8) | (uint16_t)p[1];
}

void play_midi_from_memory(const uint8_t *midi_data, uint32_t size) {
    // 重置状态
    init_midi_parser(&g_midi_state);
    g_event_queue.head = 0;
    g_event_queue.tail = 0;
    g_event_queue.count = 0;

    // MIDI 文件头: 14 字节 (chunk_type=4 + length=4 + format=2 + tracks=2 + division=2)
    if (size < 14) return;
    if (read_u32be(midi_data) != 0x4D546864) return;   // "MThd"

    uint16_t track_count = read_u16be(midi_data + 10);
    if (track_count > 128) return;

    uint32_t offset = 14;
    for (int track = 0; track < track_count; track++) {
        // MTrk 头: 8 字节 (chunk_type=4 + length=4)
        if (offset + 8 > size) return;
        if (read_u32be(midi_data + offset) != 0x4D54726B) return; // "MTrk"
        uint32_t track_size = read_u32be(midi_data + offset + 4);
        if (offset + 8 + track_size > size) return;

        parse_midi_track(midi_data + offset + 8,
                         track_size, &g_midi_state, &g_event_queue);
        offset += 8 + track_size;
    }

    // 和试音完全一样的播放模式
    if (g_event_queue.count == 0) {
        play_test_melody();   // 解析失败，回退试音
        return;
    }
    while (g_event_queue.count > 0) {
        NoteEvent *note = &g_event_queue.buffer[g_event_queue.tail];
        audio_play_note(note);
        delay_ms(note->duration);
        beep_pwm_off();
        delay_ms(10);
        g_event_queue.tail = (g_event_queue.tail + 1) % EVENT_QUEUE_SIZE;
        g_event_queue.count--;
    }
}

// ===================== 测试旋律（硬编码）=====================
void play_test_melody(void) {
    const NoteEvent test_melody[] = {
        {262, 400, 100, 0, 0},  // C4
        {294, 400, 100, 0, 0},  // D4
        {330, 400, 100, 0, 0},  // E4
        {349, 400, 100, 0, 0},  // F4
        {392, 400, 100, 0, 0},  // G4
        {440, 400, 100, 0, 0},  // A4
        {494, 400, 100, 0, 0},  // B4
        {523, 800, 120, 0, 0},  // C5
    };

    for (int i = 0; i < (int)(sizeof(test_melody) / sizeof(NoteEvent)); i++) {
        audio_play_note(&test_melody[i]);
        delay_ms(test_melody[i].duration);
        beep_pwm_off();
        delay_ms(10);
    }
}

// ===================== 主函数入口 =====================
int main(void) {
    // 1. 初始化 PWM 蜂鸣器（TIM4_CH1 → PB6）
    beep_pwm_init();

    // 2. 初始化音频引擎
    audio_engine_init(TIM4, 1);

    // 3. 初始化 MIDI 解析器
    init_midi_parser(&g_midi_state);

    // 4. 播放测试旋律
    play_test_melody();

    // 5. 循环播放曲库
    int idx = 0;
    while (1) {
        play_midi_from_memory(g_playlist[idx].data, g_playlist[idx].size);
        idx = (idx + 1) % (sizeof(g_playlist) / sizeof(g_playlist[0]));
        delay_ms(1000);
    }

    return 0;
}