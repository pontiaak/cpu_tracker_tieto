//Task for Reader thread
//The first thread (Reader) reads /proc/stat and sends the string (as raw data) or a structure (fields from file needed for calculation) to the second thread (Analyzer)

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>	//lib for threads
#include <string.h>
#include "tieto_cpu_tracker.h"	//header file for this project
#include <unistd.h>	//lib for sleep function


void* ReaderTask(void*)
{

	char fileLine[maximumLineLength];

	while(!terminationRequest)	//while loop to keep thread functioning
	{
		usleep(250000); //250 milliseconds - sleep function to being able to gather sets of cpu stats with interval
		cpuNumber = 0;	//temporary loop until better way is implimented
		FILE* file = fopen("/proc/stat", "r");	//opening /proc/stat with atribute for only reading ("r")
		if (file == NULL)	//catching an issue
		{
			perror("Failed to open /proc/stat");
			sem_post(&loggerSemaphore);	//waking up logger
			loggerMessage = 9;	//logger message 9 means "Reader thread failed to open /proc/stat"
			terminationRequest = 1;
		}

		sem_wait(&producerSemaphore);	//waiting until analizer thread finishes with last packet of data and incriments this semaphore
		pthread_mutex_lock(&mutexBuffer);	//locking thread into mutex to solve pcp
			while (fgets(fileLine, sizeof(fileLine), file) && cpuNumber < maximumCpuNumber)	//iterating by line whith "cpu" and adding all the statistics to the global structure
			{
				if (strncmp(fileLine, "cpu", 3) == 0)	//getting all the lines with cpu statistics - producing data into structure buffer
				{			
					sscanf(fileLine, "%*s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
					&cpuStatistics[cpuNumber].user,
					&cpuStatistics[cpuNumber].nice,
					&cpuStatistics[cpuNumber].system,
					&cpuStatistics[cpuNumber].idle,
					&cpuStatistics[cpuNumber].iowait,
					&cpuStatistics[cpuNumber].irq,
					&cpuStatistics[cpuNumber].softirq,
					&cpuStatistics[cpuNumber].steal,
					&cpuStatistics[cpuNumber].guest,
					&cpuStatistics[cpuNumber].guest_nice);
						
					cpuNumber++;	//incrimenting to continue iterating
				}
			}
		watchdogFlags[0]=1;	//flag for watchdog to know this thread is active, its inside mutex lock b.c. it's shared memory with watchdog
		pthread_mutex_unlock(&mutexBuffer);	//unlocking thread from mutex
		sem_post(&consumerSemaphore);	//incrimenting semaphore, thus telling the wait() in analyzer thread that reading of this data packege is done and it can start analyzing it
		fclose(file);
	}
	pthread_exit(NULL);
}
