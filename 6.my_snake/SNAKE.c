#include <ncurses.h>
#include <stdlib.h>
#include <time.h>

#define WIDTH 60
#define HEIGHT 20

//#define HardMode // 困难模式

typedef struct {
	int x, y;
}Point;

int main(int argc, const char *argv[])
{
	srand(time(NULL));
	
	int lenght = 1;
	Point snake[HEIGHT*WIDTH] = {
		{WIDTH/2, HEIGHT/2}
	};
	Point food = {rand()%(WIDTH-2) + 1, rand()%(HEIGHT-2) + 1};

	//第一步：初始化环境,初始化stdsdc窗口
	initscr();
	printw("Snake Game!!! Press any key to start...\n");
		getch();//等待按键输入，达到阻塞目的

	//开启特殊按键功能
	keypad(stdscr, TRUE);
	//开启非阻塞功能
	nodelay(stdscr, TRUE);
	curs_set(0);//隐藏光标
	//刷新数据
	refresh();
	//创建窗口
	WINDOW *win = newwin(HEIGHT, WIDTH, 0, 0);
	//绘制窗口边界
	box(win, 0, 0);
	//刷新显示
	wrefresh(win);
	int ch = 0;
	int direction = KEY_RIGHT;
	while('q' != ch) {
		mvwprintw(win, 1, 1, "         ");

		//等待按键输入，达到阻塞目的
		ch = getch();

		// 处理方向键和wasd键
		switch(ch) {
			case KEY_DOWN:
			case 's':  // 新增：s 键对应下方向
			case 'S':
				direction = direction==KEY_UP ? KEY_UP : KEY_DOWN;
				break;
				
			case KEY_UP:
			case 'w':  // 新增：w 键对应上方向
			case 'W':
				direction = direction==KEY_DOWN ? KEY_DOWN : KEY_UP;
				break;
				
			case KEY_LEFT:
			case 'a':  // 新增：a 键对应左方向
			case 'A':
				direction = direction==KEY_RIGHT ? KEY_RIGHT : KEY_LEFT;
				break;
				
			case KEY_RIGHT:
			case 'd':  // 新增：d 键对应右方向
			case 'D':
				direction = direction==KEY_LEFT ? KEY_LEFT : KEY_RIGHT;
				break;
        }

		// 移动蛇头
		switch(direction) {
			case KEY_DOWN:
				snake[0].y++;
				break;
			case KEY_UP:
				snake[0].y--;
				break;
			case KEY_LEFT:
				snake[0].x--;
				break;
			case KEY_RIGHT:
				snake[0].x++;
				break;
		}

		// 清屏并重新绘制窗口
		werase(win);
		box(win, 0, 0);
		mvwaddch(win, snake[0].y, snake[0].x, 'o');
		mvwaddch(win, food.y, food.x, '*');

		//碰撞检测
		if(snake[0].x < 0 || snake[0].x >= WIDTH || snake[0].y < 0 || snake[0].y >= HEIGHT){
		    // todo 检查是否撞到自己


			// 创建游戏结束窗口
			WINDOW *gameover_win = newwin(5, 30, HEIGHT/2 - 2, WIDTH/2 - 15);
			box(gameover_win, 0, 0);
			mvwprintw(gameover_win, 1, 1, "          You Loss!    ");
			wrefresh(gameover_win);
			// 设置标准窗口为阻塞模式
			nodelay(stdscr, FALSE);
			getch();  // 等待按键
			
			delwin(gameover_win);
			delwin(win);

			break;
		}

		//检查是否吃到食物
		if(snake[0].x == food.x && snake[0].y == food.y) {
			lenght++;
			food.y = rand()% (HEIGHT-2) + 1;
			food.x = rand()% (WIDTH-2) + 1;
		}
		//显示蛇身
		for(int i = 0; i < lenght; i++) {
			if(i == 0) {
				mvwaddch(win,snake[i].y, snake[i].x, '@');
			} else {
				mvwaddch(win,snake[i].y, snake[i].x, 'o');
			}
		}
		//移动蛇身
		for(int i = lenght - 1; i > 0; i--)
			snake[i] = snake[i-1];

			
		// 控制游戏速度
		napms(140);
		//刷新显示
		wrefresh(win);
	}

	//销毁不使用的窗口
	delwin(win);
	//结束环境
	endwin();
	return 0;
}
