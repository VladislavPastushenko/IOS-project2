// ********************************************* //
// Created by Vladislav Pastushenko, xpastu04
// ********************************************* //
// 02.05.2020


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>

// Sleep random time from 0 to maxTime interval
void sleep_random_time(int maxTime)
{
	maxTime *= 1000;
	if (maxTime > 0)
		usleep((useconds_t)(rand() % maxTime));
}



// Set value "val" in the semaphore "sem"
void sem_set(sem_t* sem, int val) {
	int value;
	sem_getvalue(sem, &value);
	while (value != val)
	{
		if (value > val) {
			sem_wait(sem);
			value--;
		}
		else {
			sem_post(sem);
			value++;
		}
	}
}



// The main function of the program
int main(int argc, char* argv[])
{

	// Check quantity of the arguments
	if (argc != 6) {
		fprintf(stderr, "Quantity of arguments has to be equal to 5\n");
		exit(1);
	}

	// Check if arguments contain any incorrect symbol
	for (int i = 1; i < argc; i++)
	{
		for (unsigned j = 0; j < strlen(argv[i]); j++) {
			if (argv[i][j] > '9' || argv[i][j] < '0') {
				fprintf(stderr, "Arguments have to be integers\n");
				exit(1);
			}
		}
	}

	// Quantity of the immigrants
	unsigned PI = atoi(argv[1]);
	if (PI < 1) {
		fprintf(stderr, "Quantity of immigrants has to be more than 0\n");
		exit(1);
	}

	// Max length of time for generation of new Immigrant process
	unsigned IG = atoi(argv[2]);
	if (IG > 2000) {
		fprintf(stderr, "Max length of time for generation of new Immigrant has to be in <0; 2000> interval\n");
		exit(1);
	}

	// Max lenght of time when judge return to the building 
	unsigned JG = atoi(argv[3]);
	if (JG > 2000) {
		fprintf(stderr, "Max lenght of time when judge return to the building has to be in <0; 2000> interval\n");
		exit(1);
	}

	// Max lenght of obtaining a certificate time
	unsigned IT = atoi(argv[4]);
	if (IT > 2000) {
		fprintf(stderr, "Max lenght of obtaining a certificate time has to be in <0; 2000> interval\n");
		exit(1);
	}

	// Max lenght of judge deciding time
	unsigned JT = atoi(argv[5]);
	if (JT > 2000) {
		fprintf(stderr, "Max lenght of judge deciding time has to be in <0; 2000> interval\n");
		exit(1);
	}

	// Find or open place for memory sharing
	long sharedMemKey;
	if ((sharedMemKey = (shm_open("/sharedMem", O_CREAT, S_IRWXU))) == -1) {
		fprintf(stderr, "shm_open ERROR");
		exit(1);
	}

	

	// Counter, wich show actual process number
	int* processCounter;
	if ((processCounter = mmap((void*)sharedMemKey, 10, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0)) == MAP_FAILED)
	{
		shm_unlink("/sharedMem");
		fprintf(stderr, "mmap ERROR\n");
		exit(1);
	}
	*processCounter = 0;

	// Actual quantity of the immigrants, which are in building and haven't got decision about them
	int* NE;
	if ((NE = mmap((void*)sharedMemKey, sizeof(int*), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0)) == MAP_FAILED)
	{
		munmap(processCounter, sizeof(int*));
		shm_unlink("/sharedMem");
		fprintf(stderr, "mmap ERROR\n");
		exit(1);
	}
	*NE = 0;

	// Actual quantity of the immigrants, which are registered and haven't got registration decision about them
	int* NC;
	if ((NC = mmap((void*)sharedMemKey, sizeof(int*), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0)) == MAP_FAILED)
	{
		munmap(processCounter, sizeof(int*));
		munmap(NE, sizeof(int*));
		shm_unlink("/sharedMem");
		fprintf(stderr, "mmap ERROR\n");
		exit(1);
	}
	*NC = 0;

	// Actual quantity of the immigrants, which are in building
	int* NB;
	if ((NB = mmap((void*)sharedMemKey, sizeof(int*), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0)) == MAP_FAILED)
	{
		munmap(processCounter, sizeof(int*));
		munmap(NE, sizeof(int*));
		munmap(NC, sizeof(int*));
		shm_unlink("/sharedMem");
		fprintf(stderr, "mmap ERROR\n");
		exit(1);
	}

	// Actual quantity of the immigrants, which haven't got certificate
	int* IM;
	if ((IM = mmap((void*)sharedMemKey, sizeof(int*), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0)) == MAP_FAILED)
	{
		munmap(processCounter, sizeof(int*));
		munmap(NE, sizeof(int*));
		munmap(NC, sizeof(int*));
		munmap(NB, sizeof(int*));
		shm_unlink("/sharedMem");
		fprintf(stderr, "mmap ERROR\n");
		exit(1);
	}
	*IM = PI;


	// If isNotJudgeInBuilding == 0 then judge is in the bulding, if 1 then not
	sem_t* isNotJudgeInBuilding;
	if ((isNotJudgeInBuilding = sem_open("/isNotJudgeInBuilding", O_CREAT, S_IRWXU, 1)) == SEM_FAILED) {
		munmap(processCounter, sizeof(int*));
		munmap(NE, sizeof(int*));
		munmap(NC, sizeof(int*));
		munmap(NB, sizeof(int*));
		munmap(IM, sizeof(int*));
		shm_unlink("/sharedMem");
		fprintf(stderr, "sem_open ERROR\n");
		exit(1);
	}
	sem_set(isNotJudgeInBuilding, 1);


	// Registration semaphore
	sem_t* registration;
	if ((registration = sem_open("/registration", O_CREAT, S_IRWXU, 0)) == SEM_FAILED) {
		munmap(processCounter, sizeof(int*));
		munmap(NE, sizeof(int*));
		munmap(NC, sizeof(int*));
		munmap(NB, sizeof(int*));
		munmap(IM, sizeof(int*));
		sem_close(isNotJudgeInBuilding);
		shm_unlink("/sharedMem");
		fprintf(stderr, "sem_open ERROR\n");
		exit(1);
	}
	sem_set(registration, 0);

	// Main semaphore of the program
	sem_t* mainSem;
	if ((mainSem = sem_open("/main", O_CREAT, S_IRWXU, 0)) == SEM_FAILED) {
		munmap(processCounter, sizeof(int*));
		munmap(NE, sizeof(int*));
		munmap(NC, sizeof(int*));
		munmap(NB, sizeof(int*));
		munmap(IM, sizeof(int*));
		sem_close(isNotJudgeInBuilding);
		sem_close(registration);
		shm_unlink("/sharedMem");
		fprintf(stderr, "sem_open ERROR\n");
		exit(1);
	}
	sem_set(mainSem, 0);

	// Open semaphore
	sem_t* openSem;
	if ((openSem = sem_open("/openSem", O_CREAT, S_IRWXU, 0)) == SEM_FAILED) {
		munmap(processCounter, sizeof(int*));
		munmap(NE, sizeof(int*));
		munmap(NC, sizeof(int*));
		munmap(NB, sizeof(int*));
		munmap(IM, sizeof(int*));
		sem_close(isNotJudgeInBuilding);
		sem_close(registration);
		sem_close(mainSem);
		shm_unlink("/sharedMem");
		fprintf(stderr, "sem_open ERROR\n");
		exit(1);
	}
	sem_set(openSem, 0);

	// Semaphore for printing
	sem_t* printSem;
	if ((printSem = sem_open("/printSem", O_CREAT, S_IRWXU, 1)) == SEM_FAILED) {
		munmap(processCounter, sizeof(int*));
		munmap(NE, sizeof(int*));
		munmap(NC, sizeof(int*));
		munmap(NB, sizeof(int*));
		munmap(IM, sizeof(int*));
		sem_close(isNotJudgeInBuilding);
		sem_close(registration);
		sem_close(mainSem);
		sem_close(openSem);
		shm_unlink("/sharedMem");
		fprintf(stderr, "sem_open ERROR\n");
		exit(1);
	}
	sem_set(printSem, 1);

	// Semaphore for judge waiting for immigrants
 	sem_t* judgeWaitingSem;
	if ((judgeWaitingSem = sem_open("/judgeWaitingSem", O_CREAT, S_IRWXU, 1)) == SEM_FAILED) {
		munmap(processCounter, sizeof(int*));
		munmap(NE, sizeof(int*));
		munmap(NC, sizeof(int*));
		munmap(NB, sizeof(int*));
		munmap(IM, sizeof(int*));
		sem_close(isNotJudgeInBuilding);
		sem_close(registration);
		sem_close(mainSem);
		sem_close(openSem);
		sem_close(printSem);
		shm_unlink("/sharedMem");
		fprintf(stderr, "sem_open ERROR\n");
		exit(1);
	}
	sem_set(judgeWaitingSem, 1);

	// If isImmInBuilding == 1 then any imm is in the bulding, if 0 then not
	sem_t* isImmInBuilding;
	if ((isImmInBuilding = sem_open("/isImmInBuilding", O_CREAT, S_IRWXU, 1)) == SEM_FAILED) {
		munmap(processCounter, sizeof(int*));
		munmap(NE, sizeof(int*));
		munmap(NC, sizeof(int*));
		munmap(NB, sizeof(int*));
		munmap(IM, sizeof(int*));
		sem_close(isNotJudgeInBuilding);
		sem_close(registration);
		sem_close(mainSem);
		sem_close(openSem);
		sem_close(printSem);
		sem_close(judgeWaitingSem);
		shm_unlink("/sharedMem");
		fprintf(stderr, "sem_open ERROR\n");
		exit(1);
	}
	sem_set(isImmInBuilding, 0);


	// Open file
	FILE *fl = fopen("proj2.out", "w");
	setbuf(fl, NULL);

	pid_t pid = fork();
	
	// Fork error
	if (pid == -1) {
		munmap(processCounter, sizeof(int*));
		munmap(NE, sizeof(int*));
		munmap(NC, sizeof(int*));
		munmap(NB, sizeof(int*));
		munmap(IM, sizeof(int*));
		sem_close(isNotJudgeInBuilding);
		sem_close(registration);
		sem_close(mainSem); sem_close(openSem);
		sem_close(printSem);
		sem_close(judgeWaitingSem);
		sem_close(isImmInBuilding);
		shm_unlink("/sharedMem");
		fclose(fl);
		fprintf(stderr, "fork ERROR\n");
		exit(-1);
	}

	// Judge process
	if (pid != 0) {

		sem_wait(openSem);
		while (true) {


			// Judge wants to enter
			sleep_random_time(JG);
			sem_wait(printSem);
			*processCounter += 1;
			fprintf(fl, "%d   : JUDGE   : wants to enter\n", *processCounter);
			sem_post(printSem);

			sem_wait(isNotJudgeInBuilding);

			// Judge enters
			sem_wait(printSem);
			*processCounter += 1;
			fprintf(fl, "%d   : JUDGE   : enters              : %d   : %d   : %d\n", *processCounter, *NE, *NC, *NB);
			sem_post(printSem);

			sem_wait(printSem);
			if (*NC != *NE) {
				*processCounter += 1;
				fprintf(fl, "%d   : JUDGE   : waits for imm       : %d   : %d   : %d\n", *processCounter, *NE, *NC, *NB);
				sem_post(printSem);

				sem_wait(judgeWaitingSem);
			}
			else
			{
				sem_set(judgeWaitingSem, 0);
				sem_post(printSem);
			}

			sem_wait(printSem);
			*processCounter += 1;
			fprintf(fl, "%d   : JUDGE   : starts confirmation : %d   : %d   : %d\n", *processCounter, *NE, *NC, *NB);
			sem_post(printSem);

			sleep_random_time(JT);

			

			sem_wait(printSem);
			sem_set(isImmInBuilding, 0);
			int nc = *NC;
			*NE = 0;
			*NC = 0;
			*processCounter += 1;
			fprintf(fl, "%d   : JUDGE   : ends confirmation   : %d   : %d   : %d\n", *processCounter, *NE, *NC, *NB);
			sem_post(printSem);


			for (int i = 0; i < nc; i++) {
					sem_post(registration);
				}

				

			sleep_random_time(JT);

			sem_wait(printSem);
			*processCounter += 1;
			fprintf(fl, "%d   : JUDGE   : leaves              : %d   : %d   : %d\n", *processCounter, *NE, *NC, *NB);
			sem_post(isNotJudgeInBuilding);
			sem_post(printSem);

			if (*NE == 0) {

				if (*IM == 0)
					break;

				sem_wait(isImmInBuilding);
				sem_post(isImmInBuilding);
			}

			if (*IM == 0)
				break;
			
		}
		sem_wait(printSem);
		*processCounter += 1;
		fprintf(fl, "%d   : JUDGE   : finishes\n", *processCounter);
		sem_post(printSem);

		sem_post(mainSem);
		munmap(processCounter, sizeof(int*));
		munmap(NE, sizeof(int*));
		munmap(NC, sizeof(int*));
		munmap(NB, sizeof(int*));
		munmap(IM, sizeof(int*));
		sem_close(isNotJudgeInBuilding);
		sem_close(registration);
		sem_close(mainSem); 
		sem_close(openSem);
		sem_close(printSem);
		sem_close(isImmInBuilding);
		sem_close(judgeWaitingSem);
		sem_close(isImmInBuilding);
		shm_unlink("/sharedMem");
		fclose(fl);
		return 0;
	}


	// Generator of immigrants process
	else
	{
		for (unsigned i = 0; i < PI; i++) {
			sleep_random_time(IG);

			pid_t I = fork();
			 
			// Fork error
			if (I == -1) {
				munmap(processCounter, sizeof(int*));
				munmap(NE, sizeof(int*));
				munmap(NC, sizeof(int*));
				munmap(NB, sizeof(int*));
				munmap(IM, sizeof(int*));
				sem_close(isNotJudgeInBuilding);
				sem_close(registration);
				sem_close(mainSem);
				sem_close(openSem);
				sem_close(judgeWaitingSem);
				sem_close(isImmInBuilding);
				shm_unlink("/sharedMem");
				sem_close(printSem);
				fclose(fl);
				fprintf(stderr, "fork ERROR");
				exit(-1);
			}


			// New immigrant
			else if (I == 0) {
				sem_wait(printSem);
				*processCounter += 1;
				fprintf(fl, "%d   : IMM %d   : starts\n", *processCounter, i + 1);
				sem_post(openSem);
				sem_post(printSem);


				// Waits until judge leaves building
				sem_wait(isNotJudgeInBuilding);

				// Enter to the building
				sem_wait(printSem);
				sem_post(isNotJudgeInBuilding);
				sem_set(judgeWaitingSem, 0);
				*NE += 1;
				*NB += 1;
				*processCounter += 1;
				fprintf(fl, "%d   : IMM %d   : enters              : %d   : %d   : %d\n", *processCounter, i + 1, *NE, *NC, *NB);
				sem_post(printSem);


				// Registering
				sem_wait(printSem);
				sem_post(isImmInBuilding);
				*NC += 1;
				*processCounter += 1;
				fprintf(fl, "%d   : IMM %d   : checks              : %d   : %d   : %d\n", *processCounter, i + 1, *NE, *NC, *NB);
				if (*NC == *NE) 
					sem_post(judgeWaitingSem);
 
					
				sem_post(printSem);
				
				// Wait until the judge approve registration
				sem_wait(registration);


				// Getting of a sertificate
				sem_wait(printSem);
				*processCounter += 1;
				fprintf(fl, "%d   : IMM %d   : wants certificate   : %d   : %d   : %d\n", *processCounter, i + 1, *NE, *NC, *NB);
				sem_post(printSem);


				sleep_random_time(IT);

				sem_wait(printSem);
				*processCounter += 1;
				fprintf(fl, "%d   : IMM %d   : got certificate     : %d   : %d   : %d\n", *processCounter, i + 1, *NE, *NC, *NB);
				*IM -= 1;
				if (*IM == 0) {

					sem_post(isImmInBuilding);
				}
				sem_post(printSem);

				// Waits until judge leaves building

				sem_wait(isNotJudgeInBuilding);

				// Leaving the building 
				sem_wait(printSem);
				*NB -= 1;
				*processCounter += 1;
				fprintf(fl, "%d   : IMM %d   : leaves              : %d   : %d   : %d\n", *processCounter, i + 1, *NE, *NC, *NB);
				sem_post(isNotJudgeInBuilding);

				sem_post(printSem);

				sem_wait(mainSem);
				sem_post(mainSem);
				return 0;
			}
		}
	}
	return 0;
}
