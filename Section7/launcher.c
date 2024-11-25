#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define NUM_CLIENTS 5 // Adjust the number of clients to spawn
#define CLIENT_PROGRAM "./client" // Path to the compiled client program

int main() {
    pid_t pids[NUM_CLIENTS];

    printf("Launcher: Starting %d clients...\n", NUM_CLIENTS);

    for (int i = 0; i < NUM_CLIENTS; i++) {
        pids[i] = fork();

        if (pids[i] == 0) {
            // Child process: Run the client program
            execl(CLIENT_PROGRAM, CLIENT_PROGRAM, NULL);

            // If execl fails, print an error and exit
            perror("execl failed");
            exit(EXIT_FAILURE);
        } else if (pids[i] < 0) {
            // Fork failed
            perror("fork failed");
            exit(EXIT_FAILURE);
        } else {
            // Parent process: Continue spawning other clients
            printf("Launcher: Started client %d (PID: %d)\n", i + 1, pids[i]);
        }
    }

    // Parent process: Wait for all child processes to finish
    for (int i = 0; i < NUM_CLIENTS; i++) {
        waitpid(pids[i], NULL, 0);
        printf("Launcher: Client %d (PID: %d) has finished.\n", i + 1, pids[i]);
    }

    printf("Launcher: All clients have finished.\n");

    return 0;
}
