// src/audio_engine.c - 寄存器级音频引擎（无 HAL）
#include "music_engine.h"
#include "stm32mp157_tim.h"

// 人耳可听范围：20Hz ~ 20kHz，PWM 载波频率限制
#define MIN_FREQ  20
#define MAX_FREQ  20000

AudioEngine g_audio __attribute__((section(".data"))) = {0};

// 初始化PWM音频引擎（TIM4_CH1）
void audio_engine_init(tim2_3_4_5_t *htim, uint32_t channel) {
    g_audio.htim = htim;
    g_audio.pwm_channel = channel;
    g_audio.is_playing = false;

    // 调用参考.c 的寄存器级 PWM 初始化（一次性）
    beep_pwm_init();
}

// 播放单个音符（非阻塞，纯寄存器操作）
bool audio_play_note(const NoteEvent *note) {
    if (note->freq == 0 || note->freq < MIN_FREQ || note->freq > MAX_FREQ) {
        // 休止符或无效频率：停止计数器
        g_audio.htim->CR1 &= ~(1 << 0);
        g_audio.is_playing = false;
        return (note->freq == 0);  // freq=0 的休止符视为成功
    }

    // 计算 PWM 周期（TIM4 时钟 = 209MHz/209 = 1MHz = 1us）
    // ARR = 1e6 / freq - 1，确保 ARR >= 1
    uint32_t period = 1000000 / note->freq;
    if (period < 2) period = 2;  // ARR 至少为 1，防止下溢
    g_audio.htim->ARR = period - 1;

    // 根据力度设置占空比（velocity: 0-127 → 占空比约 50% 处映射）
    // duty = period * velocity / 254
    uint32_t duty = (period * note->velocity) / 254;
    if (duty > period - 1)
        duty = period - 1;

    // 写 CCRx 寄存器（根据通道号选择）
    switch (g_audio.pwm_channel) {
        case 1: g_audio.htim->CCR1 = duty; break;
        case 2: g_audio.htim->CCR2 = duty; break;
        case 3: g_audio.htim->CCR3 = duty; break;
        case 4: g_audio.htim->CCR4 = duty; break;
        default: return false;
    }

    // 清零计数器 + 生成更新事件（强制 ARR/CCR 影子寄存器立即生效）
    g_audio.htim->CNT = 0;
    g_audio.htim->EGR = 1;       // UG=1 → 更新事件

    // 重新使能输出 + 启动计数器
    g_audio.htim->CCER |= (1 << 0);      // CC1E = 1
    g_audio.htim->CR1  |= (1 << 0);      // CEN = 1

    // MIDI 队列播放需要的状态记录
    g_audio.current_note = *note;
    g_audio.note_start_time = get_tick_ms();
    g_audio.is_playing = true;

    return true;
}

// 从事件队列取下一个音符播放（主循环调用）
void audio_process_queue(EventQueue *queue) {
    if (!g_audio.is_playing && queue->count > 0) {
        NoteEvent next_note = queue->buffer[queue->tail];
        audio_play_note(&next_note);
        queue->tail = (queue->tail + 1) % EVENT_QUEUE_SIZE;
        queue->count--;
    }
}