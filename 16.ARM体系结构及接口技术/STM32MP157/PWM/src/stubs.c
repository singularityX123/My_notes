// src/stubs.c - 裸机 libgcc 存根（替代缺少的 libc 符号）
int raise(int sig) {
    (void)sig;
    return 0;   // 除零等异常 → 静默返回
}
