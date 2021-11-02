#include <stdlib.h>
#include <pthread.h>

#include "centipede.h"
#include "threadwrappers.h"
#include "console.h"
#include "gameglobals.h"
#include "player.h"
#include "keyboard.h"

pthread_mutex_t screenMutex;
pthread_mutex_t gameOverMutex;
pthread_cond_t gameOverCond;
pthread_t screenThread;
pthread_t keyboardThread;

void centipedeRun() {

	if(consoleInit(GAME_ROWS, GAME_COLS, GAME_BOARD)) {
		wrappedMutexInit(&screenMutex, NULL);	
		wrappedMutexInit(&gameOverMutex, NULL);
		wrappedPthreadCreate(&screenThread, NULL, screenRefresh, NULL);
		player* p = spawnPlayer(22, 38, 3);
		wrappedPthreadCreate(&keyboardThread, NULL, runKeyboard, p);

		wrappedMutexLock(&gameOverMutex);	
		wrappedCondWait(&gameOverCond, &gameOverMutex);
		wrappedMutexUnlock(&gameOverMutex);
		wrappedMutexLock(&screenMutex);
		finalKeypress();
		consoleFinish();
		wrappedMutexUnlock(&screenMutex);
		wrappedPthreadJoin(screenThread, NULL);
		wrappedPthreadJoin(p->thread, NULL);
	}
}

void* screenRefresh(void* dummy) {
	while(true) {
		wrappedMutexLock(&gameOverMutex);
		if(gameOver) {
			wrappedMutexUnlock(&gameOverMutex);
			pthread_exit(NULL);
		}
		wrappedMutexUnlock(&gameOverMutex);
		wrappedMutexLock(&screenMutex);
		consoleRefresh();
		wrappedMutexUnlock(&screenMutex);
		sleepTicks(1);
	}
}
