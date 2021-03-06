//centipede.c
//holds the main executing function of the program
//as well as the screenRefresh and cleanup threads

#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>

#include "centipede.h"
#include "bullet.h"
#include "threadwrappers.h"
#include "console.h"
#include "gameglobals.h"
#include "player.h"
#include "keyboard.h"
#include "enemy.h"

//player starting position
#define START_COL 38
#define START_ROW 20

//position of score on screen
#define SCORE_COL 7
#define SCORE_ROW 0
#define SCORE_LEN 10

// # of lives started with
#define LIVES 3
//position of lives on screen
#define LIVES_COL 23
#define LIVES_ROW 0
#define LIVES_LEN 2

void centipedeRun() {
	if(consoleInit(LOWER_GAME_BOUND, RIGHT_GAME_BOUND, GAME_BOARD)) {
		//set everything up
		wrappedMutexInit(&screenMutex, NULL);
		wrappedMutexInit(&gameOverMutex, NULL);
		wrappedMutexInit(&randomMutex, NULL);
		wrappedMutexInit(&bulletListMutex, NULL);
		wrappedMutexInit(&enemyListMutex, NULL);
		wrappedMutexInit(&scoreMutex, NULL);
	
		score = 0;

		bList = mallocBulletList();
		eList = mallocEnemyList();

		wrappedMutexLock(&randomMutex);
		srand(time(0));
		wrappedMutexUnlock(&randomMutex);
		
		p = spawnPlayer(START_ROW, START_COL, LIVES);
		wrappedMutexLock(&p->mutex);
		wrappedPthreadCreate(&screenThread, NULL, screenRefresh, NULL);
		wrappedPthreadCreate(&keyboardThread, NULL, runKeyboard, p);
		wrappedMutexUnlock(&p->mutex);
		wrappedPthreadCreate(&spawnerThread, NULL, runEnemySpawner, eList);
		wrappedPthreadCreate(&cleanupThread, NULL, runCleanup, NULL);

		//wait until gameOver signal is sent
		wrappedMutexLock(&gameOverMutex);	
		wrappedCondWait(&gameOverCond, &gameOverMutex);
		wrappedMutexUnlock(&gameOverMutex);
	
		//wait for final keypress
		wrappedMutexLock(&screenMutex);
		finalKeypress();
		consoleFinish();
		wrappedMutexUnlock(&screenMutex);
		
		//join threads and free memory
		wrappedPthreadJoin(screenThread, NULL);
		wrappedPthreadJoin(p->thread, NULL);
		free(p);
		wrappedPthreadJoin(keyboardThread, NULL);
		wrappedPthreadJoin(cleanupThread, NULL);
		
		joinEnemyList(eList);
		freeEnemyList(eList);
		joinBulletList(bList);
		freeBulletList(bList);
		
		//this thread takes a while to clean up because it has a very long sleep timer
		//that is why the program hangs for a bit before the done message is shown
		wrappedPthreadJoin(spawnerThread, NULL);
	}
}

void* screenRefresh(void* dummy) {
	while(true) {
		//if gameover, exit thread
		if(isGameOver()) {
			pthread_exit(NULL);
		}
		wrappedMutexLock(&screenMutex);
		//refresh screen
		consoleRefresh();
		wrappedMutexUnlock(&screenMutex);
		sleepTicks(1);
	}
}

void* runCleanup(void* dummy) {
	while(true) {
		//if gameover exit thread
		if(isGameOver()) {
			pthread_exit(NULL);
		}
		wrappedMutexLock(&enemyListMutex);
		wrappedMutexLock(&scoreMutex);
		//if no more enemies and player has shot a bullet, gameover and exit
		if(eList->liveEnemies == 0 && score > 0) {
			wrappedMutexUnlock(&enemyListMutex);
			wrappedMutexUnlock(&scoreMutex);
			endGame();
			pthread_exit(NULL);
		}
		wrappedMutexUnlock(&enemyListMutex);
		wrappedMutexUnlock(&scoreMutex);
		
		//cleanup dead bullets/enemies
		joinEnemyList(eList);
		joinBulletList(bList);
		
		char score_str[SCORE_LEN];
		char lives_str[LIVES_LEN];

		//get score
		wrappedMutexLock(&scoreMutex);
		sprintf(score_str, "%d", score);
		wrappedMutexUnlock(&scoreMutex);

		//get lives
		wrappedMutexLock(&p->mutex);
		sprintf(lives_str, "%d", p->lives);
		wrappedMutexUnlock(&p->mutex);
		
		//put them on screen
		wrappedMutexLock(&screenMutex);
		putString(score_str, SCORE_ROW, SCORE_COL, SCORE_LEN);
		putString(lives_str, LIVES_ROW, LIVES_COL, LIVES_LEN);
		wrappedMutexUnlock(&screenMutex);

		sleepTicks(5);
	}
}
