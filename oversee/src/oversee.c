#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include <unistd.h>
#include <signal.h>

#include <sys/inotify.h>

#define MAX_FILE_NAME_LEN         50

#define EXT_ERR_TOO_FEW_ARGS       2
#define EXT_ERR_UNKNOWN_FILE_TYPE  3
#define EXT_ERR_INIT_INOTIFY       4
#define EXT_ERR_ADD_WATCH          5
#define EXT_ERR_READ_INOTIFY       6

int events      = -1;
int eventStatus = -1;

void signalHandler(int signal) 
{
	int closeStatus;

	printf("[Interrupt] Cleaning up!\n");
	
	closeStatus = inotify_rm_watch(events, eventStatus);
	if (closeStatus == -1) {
		perror("inotify_rm_watch");
	}

	close(events);
	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) 
{
	
	char*          cmd;
	char*          ext;
	char*          token;
	char*          prefix;
	char*          tmpPath;
	char*          filePath;
	int            readLength;
	char*          outputPath;
	char           buffer[4096];

	const uint32_t watchMask  = IN_MODIFY;

	const struct inotify_event* watchEvent;

	/* verify cli arguments */
	if (argc < 3) {
		fprintf(stderr, "USAGE: oversee <TYPE> <PATH>\n");
		exit(EXT_ERR_TOO_FEW_ARGS);
	}

	/* verification of file type */

	if ((strcmp(argv[1], "-c") != 0) && (strcmp(argv[1], "-cpp") != 0)) {
		fprintf(stderr, "[ERROR] Invalid File type!\n");
		exit(EXT_ERR_UNKNOWN_FILE_TYPE);
	}

	if (strcmp(argv[1], "-c") == 0) {
		prefix = "gcc ";
		ext = ".c";
	} else {
		prefix = "g++ ";
		ext = ".cpp";
	}

	filePath = (char *)malloc(sizeof(char) * strlen(argv[2]) + strlen(ext) + 1);
	strcat(filePath, argv[2]);
	strcat(filePath, ext);

	/* monitoring w/ inotify */
	events = inotify_init();		

	if (events == -1) {
		perror("inotify_init");
		exit(EXT_ERR_INIT_INOTIFY);
	}

	eventStatus = inotify_add_watch(events, filePath, watchMask);

	if (eventStatus == -1) {
		perror("inotify_add_watch");
		exit(EXT_ERR_ADD_WATCH);
	}

	signal(SIGINT,  signalHandler);
	signal(SIGABRT, signalHandler);
	signal(SIGTERM, signalHandler);

	while (true) {
		readLength = read(events, buffer, sizeof(buffer));
		
		if (readLength == -1) {
			fprintf(stderr, "[ERROR] Couldn't read from inotify instance!\n");
			exit(EXT_ERR_READ_INOTIFY);
		}

		for (char *bufferPointer = buffer; 
			 bufferPointer < buffer + readLength;
			 bufferPointer += sizeof(struct inotify_event) + watchEvent->len
		) {
			watchEvent = (const struct inotify_event *)bufferPointer;

			
			/* run gcc or g++ command on file update */
			outputPath = (char *)malloc(sizeof(char) * strlen(argv[2]));
			
			cmd = (char *)malloc(sizeof(char) * (strlen(argv[2]) * 2) + strlen(prefix) + 6); 
		
			/* make the cmd: gcc file.ext -o file */
			strcat(cmd, prefix);
			strcat(cmd, argv[2]);
			strcat(cmd, ext);
			strcat(cmd, " -o ");
			strcat(cmd, argv[2]);

			strcat(outputPath, "./");
			strcat(outputPath, argv[2]);
		
			if (watchEvent->mask & IN_MODIFY) {
				system("clear");
				if (system(cmd) == 0) {
					system(outputPath);
					remove(outputPath);
				}
			}
		}
	}	
	
	free(cmd);
	free(outputPath);
	return 0;
}
