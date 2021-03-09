//==============================================================================================
// project1.c - Collatz conjecture program
//
// This program that utilizes the Collatz conjecture to take a positive integer
// from user input and follow the sequence until we reach the expected result of
// 1. It outputs the steps taken to reach 1, as well as the number of steps and
// the highest step.
//
// Author:     Nicholas Nassar, University of Toledo
// Class:      EECS 3540-001 Operating Systems and Systems Programming, Spring 2021
// Instructor: Dr.Thomas
// Date:       Feb 24, 2021
// Copyright:  Copyright 2021 by Nicholas Nassar. All rights reserved.

#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

// Define constants for the
// READ_END and WRITE_END of
// the pipe.
#define READ_END 0
#define WRITE_END 1

// We define the BUFFER_SIZE as 25
// because it is a couple characters
// longer than the max number of digits
// an unsigned long long can contain.
#define BUFFER_SIZE 25

int main(int argc, char *argv[])
{
	// We first check that the user provided only a single
	// argument, and tell them what it should be if they
	// do not.
	if (argc != 2)
	{
		fprintf(stderr, "Please enter a positive integer as the one and only argument.\n");
		return 1; // exit, since we can't do anything without a valid integer.
	}

	// Set a variable for the first argument
	// (technically the second since you
	// include the program path as argv[0])
	char *firstArgument = argv[1];

	// If the first letter of the argument is -, then we have a negative number,
	// which will not work. If the first letter is a 0 and the first argument only
	// has a length of 1, we have a zero, which won't work either. Technically,
	// this doesn't account for the case where the user puts in multiple 0's (ex: 000)
	// but to handle that properly, it would have involved looping through each character
	// in the string and checking that it was zero, and that seemed like a lot,
	// especially since the error handling below would catch the issue, but just
	// provide a strange message.
	if (*firstArgument == '-' || (*firstArgument == '0' && strlen(firstArgument) == 1))
	{
		fprintf(stderr, "Sorry, you must enter a positive integer.\n");
		return 1; // exit, since we need a positive number.
	}

	// We set errno = 0 incase something else has changed the result.
	// errno is brought in by errno.h, and gets set by various methods
	// in the standard library to get the error after a method call.
	errno = 0;

	// We also declare a char pointer which will be set to the
	// next character in the string after the integer value.
	char *endptr;

	// Finally, we do the conversion from our firstArgument string
	// and place the result into n, passing in a pointer to our endptr,
	// as well as a base of 10.
	unsigned long long n = strtoull(firstArgument, &endptr, 10);

	// If n == 0 then we have an entire string of
	// characters, or if *endptr is not \0, we have
	// letters after our number, so we do not have
	// a proper number.
	if (n == 0 || *endptr != '\0')
	{
		fprintf(stderr, "Sorry, you must enter a positive integer.\n");
		return 1; // exit, since we don't have a number to work with.
	}

	// Finally, we ensure that our number n is not
	// out of the range of values an unsigned long long
	// can represent. Using documentation, we know that
	// if n == ULLONG_MAX and we received an error of the
	// code ERANGE, then we have a number too big.
	if (n == ULLONG_MAX && errno == ERANGE)
	{
		fprintf(stderr, "That number is too large.\n");
		return 1; // exit, since we don't have a number to work with.
	}

	// Create an array that will hold
	// the file descriptors for our
	// read and write ends of the pipe.
	int fd[2];

	if (pipe(fd) == -1) // try to create the pipe
	{
		fprintf(stderr, "Pipe failed");
		return 1; // We failed to create our pipe, so we need to exit.
	}

	pid_t pid = fork(); // Attempt to fork the current process.

	if (pid < 0) // Check if we failed
	{
		fprintf(stderr, "Fork Failed\n");
		return 1; // We failed forking, so we exit again.
	}

	// Create the buffer we will be using to send and receive
	// data in the pipe between the parent and child process.
	char buffer[BUFFER_SIZE];

	if (pid == 0) // if we are the child process
	{
		close(fd[READ_END]);				  // close unused end of the pipe, since we won't be reading.
		int steps = 0;						  // We keep track of the number of steps to get to 1.
		unsigned long long highestNumber = 1; // We also keep track of the highest number we've seen.
		while (n != 1)						  // While we haven't reached 1,
		{
			steps++;			   // increment the number of steps.
			if (n > highestNumber) // Update our highestNumber if n > highestNumber
			{
				highestNumber = n;
			}
			snprintf(buffer, BUFFER_SIZE, "%llu", n);	// Place our number into the buffer as a string
			write(fd[WRITE_END], &buffer, BUFFER_SIZE); // Write our buffer to the pipe

			// Now we use the Collatz conjecture
			if (n % 2 == 0) // If n is even,
			{
				n /= 2; // set n to itself divided by 2
			}
			else // otherwise,
			{
				n = 3 * n + 1; // set n to 3n + 1
			}
		}
		// Since we have exited the while loop,
		// we are left with n = 1, so let's send it:
		snprintf(buffer, BUFFER_SIZE, "%llu", n);	// Place our number into the buffer as a string
		write(fd[WRITE_END], &buffer, BUFFER_SIZE); // Write our buffer to the pipe

		// // The parent is expecting us to send a step
		// // and highest value now that we have sent 1.
		snprintf(buffer, BUFFER_SIZE, "%d", steps);			  // Place the number of steps into the buffer as a string.
		write(fd[WRITE_END], &buffer, BUFFER_SIZE);			  // Write our buffer to the pipe.
		snprintf(buffer, BUFFER_SIZE, "%llu", highestNumber); // Place the highest number into the buffer as a string.
		write(fd[WRITE_END], &buffer, BUFFER_SIZE);			  // Write our buffer to the pipe.

		close(fd[WRITE_END]); // close the write end of the pipe
	}
	else // otherwise we are the parent process, so we need to print out our numbers.
	{
		close(fd[WRITE_END]); // close unused end of the pipe, since we won't be writing.

		while (read(fd[READ_END], &buffer, BUFFER_SIZE) > 0) // While we can still read data from the pipe,
		{
			printf("%s", buffer); // Print what we just read.

			// If we have reached 1, we now have to
			// break so we can properly format the
			// steps and highest number we will print
			// out.
			if (strcmp(buffer, "1") == 0)
			{
				break;
			}

			// Since we have another number coming,
			// print out a comma and space to seperate
			// this one from it.
			printf(", ");
		}

		// At this point, we printed our final number (1)
		// so now we need to print out the steps, followed by
		// the highest number.
		read(fd[READ_END], &buffer, BUFFER_SIZE);
		printf(" (%s", buffer);

		read(fd[READ_END], &buffer, BUFFER_SIZE);
		printf(", %s)\n", buffer);

		close(fd[READ_END]); // close the read end of the pipe
		wait(NULL);			 // Wait for the child process to end
	}
	return 0; // Return 0, we succeeded, goodbye!
}
