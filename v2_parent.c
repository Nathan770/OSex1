

#include "v2_parent.h"
#include "v1_parent.h"

#pragma once

int main(int argc, char *argv[]) {     /* Pipe file descriptors */
    int gcd1 = 0, gcd2 = 0;
    int fileLines = 0;
    int *nums = getPairs(argv[1], &fileLines);
    int child1, child2;

    int child1_in_pipe[2]; //writing to child1
    int child1_out_pipe[2]; //reading from child1
    if (0 != pipe(child1_in_pipe) || 0 != pipe(child1_out_pipe)) {
        perror("cannot create pipes for child1");
        exit(1);
    }

    int child2_in_pipe[2];
    int child2_out_pipe[2];
    if (0 != pipe(child2_in_pipe) || 0 != pipe(child2_out_pipe)) {
        perror("cannot create pipes for child2");
        exit(1);
    }

    child1 = fork();
    switch (child1) {
        case -1:
            perror("fork1 failed");
        case 0:
            if (dup2(child1_in_pipe[0], STDIN_FILENO) == -1) {
                perror("child1 dup2 reader failed");
            }
            if (close(child1_in_pipe[0]) == -1 || close(child1_in_pipe[1]) == -1) {
                perror("child1 closing dup failed");
            }

            if (dup2(child1_out_pipe[1], STDOUT_FILENO) == -1) {
                perror("child1 dup2 writer failed");
            }
            if (close(child1_out_pipe[0]) == -1 || close(child1_out_pipe[1]) == -1) {
                perror("child1 closing dup failed");
            }

            if (close(child2_in_pipe[0]) == -1 || close(child2_in_pipe[1]) == -1 || close(child2_out_pipe[0]) == -1 ||
                close(child2_out_pipe[1]) == -1) {
                perror("child1 closing unnecessary fd failed");
            }
            char *args[] = {"./child2", NULL};
            execv("child2", args);
            perror("execv error child1");
        default:
            break;
    }

    child2 = fork();
    switch (child2) {
        case -1:
            perror("fork2 failed");
        case 0:
            if (dup2(child2_in_pipe[0], STDIN_FILENO) == -1) {
                perror("child2 dup2 reader failed");
            }
            if (close(child2_in_pipe[0]) == -1 || close(child2_in_pipe[1])) {
                perror("child2 closing dup failed");
            }

            if (dup2(child2_out_pipe[1], STDOUT_FILENO) == -1) {
                perror("child2 dup2 sriter failed");
            }
            if (close(child2_out_pipe[0]) == -1 || close(child2_out_pipe[1])) {
                perror("child2 closing dup failed");
            }

            if (close(child1_in_pipe[0]) == -1 || close(child1_in_pipe[1]) || close(child1_out_pipe[0]) ||
                close(child1_out_pipe[1])) {
                perror("child2 closing unnecessary fd failed");
            }

            char *args[] = {"./child2", NULL};
            execv("child2", args);
            perror("execv error child2");
        default:
            break;
    }

    //parent close unnecessary fd
    if (close(child1_in_pipe[0]) == -1 || close(child2_in_pipe[0]) == -1 || close(child1_out_pipe[1]) == -1 ||
        close(child2_out_pipe[1]) == -1) {
        perror("parent close unnecessary fd failed");
    }

    //parent writing to childern's pipe (2 by2)
    for (int i = 0; i <= fileLines; i += 2) {
        if (write(child1_in_pipe[1], &nums[i], sizeof(int) * 2) < 0) {
            perror("child1 - partial/failed write");
        }
        i += 2;
        if (i >= fileLines) break;
        if (write(child2_in_pipe[1], &nums[i], sizeof(int) * 2) < 0) {
            perror("child2 - partial/failed write");
        }
    }

    //closing parent's writing fd
    if (close(child1_in_pipe[1]) == -1 || close(child2_in_pipe[1]) == -1) {
        perror("parent closing writers failed");
    }
    //waiting for the children to finish
    if (waitpid(child1, NULL, 0) == -1) {
        perror("child1 waiting failed");
    }
    if (waitpid(child2, NULL, 0) == -1) {
        perror("child2 waiting failed");
    }

    //reading from children and printing result
    for (int i = 0; i < fileLines; i += 2) {
        if (read(child1_out_pipe[0], &gcd1, sizeof(int)) < 0) {
            perror("read from child1");
        }
        printf("%d %d gcd: %d", nums[i], nums[i + 1], gcd1);
        i += 2;
        if (i >= fileLines) break;
        if (read(child2_out_pipe[0], &gcd2, sizeof(int)) < 0) {
            perror("read from child2");
        }
        printf("%d %d gcd: %d", nums[i], nums[i + 1], gcd2);
    }

    //closing parent's reading fd
    if (close(child1_out_pipe[0]) == -1 || close(child2_out_pipe[0]) == -1) {
        perror("parent close readers failed");
    }

    exit(EXIT_SUCCESS);
}

int *getPairs(char *fileName, int *fileLines) {
    char buf[BUF_SIZE];
    int lineCounter = 0, numsCounter = 0;
    int *nums = (int *) malloc(sizeof(int) * 2);
    FILE *f = fopen(fileName, "r");
    while (!feof(f)) {
        fgets(buf, sizeof(buf), f);
        if (checkbuf(buf) == 0) continue;
        lineCounter++;
        extractPairs(&nums[numsCounter], &nums[numsCounter + 1], buf);
        numsCounter += 2;
        if (feof(f)) break;
        nums = (int *) realloc(nums, sizeof(int) * 2 * (lineCounter + 1));
    }
    *fileLines = lineCounter;
    return nums;
}