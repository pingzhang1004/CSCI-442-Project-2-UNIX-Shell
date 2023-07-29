#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>

#include "dispatcher.h"
#include "shell_builtins.h"
#include "parser.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

/**
 * dispatch_external_command() - run a pipeline of commands
 *
 * @pipeline:   A "struct command" pointer representing one or more
 *              commands chained together in a pipeline.  See the
 *              documentation in parser.h for the layout of this data
 *              structure.  It is also recommended that you use the
 *              "parseview" demo program included in this project to
 *              observe the layout of this structure for a variety of
 *              inputs.
 *
 * Note: this function should not return until all commands in the
 * pipeline have completed their execution.
 *
 * Return: The return status of the last command executed in the
 * pipeline.
 */
static int dispatch_external_command(struct command *pipeline)
{
	/*
	 * Note: this is where you'll start implementing the project.
	 *
	 * It's the only function with a "TODO".  However, if you try
	 * and squeeze your entire external command logic into a
	 * single routine with no helper functions, you'll quickly
	 * find your code becomes sloppy and unmaintainable.
	 *
	 * It's up to *you* to structure your software cleanly.  Write
	 * plenty of helper functions, and even start making yourself
	 * new files if you need.
	 *
	 * For D1: you only need to support running a single command
	 * (not a chain of commands in a pipeline), with no input or
	 * output files (output to stdout only).  In other words, you
	 * may live with the assumption that the "input_file" field in
	 * the pipeline struct you are given is NULL, and that
	 * "output_type" will always be COMMAND_OUTPUT_STDOUT.
	 *
	 * For D2: you'll extend this function to support input and
	 * output files, as well as pipeline functionality.
	 *
	 * Good luck!
	 */

	int prev_pipe[2];
	int current_pipe[2];
	int output_fd;
	int input_fd;
	// the process ID of child after invoke fork
	pid_t child_pid;
	// the process ID of child that was wait by the parent
	pid_t wait_pid;
	// the process child status got by parent
	int wait_status;

	int order_command = 0;

	while (pipeline != NULL) {
		order_command = order_command + 1;
		if (pipeline->output_type == COMMAND_OUTPUT_PIPE) {
			// On success, zero is returned.  On error, -1 is returned.
			int pipe_success = pipe(current_pipe);
			if (pipe_success == -1) {
				fprintf(stderr, "%s\n",
					"Error: The pipe did not create successfully.\n");
			} else {
				output_fd = current_pipe[1];
			}
		}
		//Output to the terminal.
		else if (pipeline->output_type == COMMAND_OUTPUT_STDOUT) {
			output_fd = STDOUT_FILENO;
		}
		// Output to the file specified by output_filename. Overwrite any existing contents.
		else if (pipeline->output_type ==
			 COMMAND_OUTPUT_FILE_TRUNCATE) {
			output_fd = open(pipeline->output_filename,
					 O_CREAT | O_WRONLY | O_TRUNC, 0664);
		}
		// Output to the file specified by output_filename.Append to any existing contents.
		else if (pipeline->output_type == COMMAND_OUTPUT_FILE_APPEND) {
			output_fd = open(pipeline->output_filename,
					 O_CREAT | O_WRONLY | O_APPEND, 0664);
		}
		child_pid = fork();
		if (child_pid == -1) {
			fprintf(stderr, "%s\n",
				"Error: The child did not fork successfully.\n");
			return (1); // true == 1 , false == 0
		}
		// 0 is returned from frok() in the child, indicates the child process has successfully forked from the parent.
		else if (child_pid == 0) {
			//check the input is a file or a pipe， command ==1 means is a file
			if (order_command == 1) {
				if (pipeline->input_filename != NULL) {
					input_fd =
						open(pipeline->input_filename,
						     O_RDONLY);
					if (input_fd < 0) {
						fprintf(stderr, "%s\n",
							"Error: No such file or directory.");
						exit(EXIT_FAILURE);
					}
				} else {
					input_fd = STDIN_FILENO;
				}
			} else {
				input_fd = prev_pipe[0];
			}
			if (pipeline->output_type == COMMAND_OUTPUT_PIPE) {
				close(current_pipe[0]);
			}
			dup2(input_fd, STDIN_FILENO);
			dup2(output_fd, STDOUT_FILENO);
			// On success, execve() does not return, on error -1 is returned,
			if (execvp(pipeline->argv[0], pipeline->argv) == -1) {
				fprintf(stderr,
					"Error: A child process did not execute successfully.\n");
				exit(EXIT_FAILURE);
			} else {
				exit(EXIT_SUCCESS);
			}

		}
		// this is the parent process
		else {
			if (pipeline->output_type == COMMAND_OUTPUT_PIPE) {
				close(current_pipe[1]);
			}
			if (order_command == 1) {
				close(prev_pipe[0]);
			}
			wait_pid = waitpid(child_pid, &wait_status, 0);
		}
		if (pipeline->output_type != COMMAND_OUTPUT_PIPE) {
			break;
		}
		prev_pipe[0] = current_pipe[0];
		prev_pipe[1] = current_pipe[1];
		pipeline = pipeline->pipe_to;
	}

	// on success, waitpid() returns the process ID of the child whose state has changed;
	// if WNOHANG was specified and one or more child(ren) specified by pid exist, but have not yet changed state, then 0 is returned.
	// On error, -1 is returned.
	if (wait_pid == -1) {
		fprintf(stderr, "Error: Wait unsuccessfully.\n");
		return (1);
	} else {
		// WIFEXITED(wstatus) returns true if the child terminated normally
		if (WIFEXITED(wait_status) == false) {
			fprintf(stderr,
				"Error: A child process terminate abnormal.\n");
			return (1);
		} else {
			// WEXITSTATUS() is called only if WIFEXITED returned true.
			// WEXITSTATUS() returns the exit status of the child.
			return WEXITSTATUS(
				wait_status); // reutrn exit( successful), it means retur(1)
		}
	}
	return 0;
}

/**
 * dispatch_parsed_command() - run a command after it has been parsed
 *
 * @cmd:                The parsed command.
 * @last_rv:            The return code of the previously executed
 *                      command.
 * @shell_should_exit:  Output parameter which is set to true when the
 *                      shell is intended to exit.
 *
 * Return: the return status of the command.
 */
static int dispatch_parsed_command(struct command *cmd, int last_rv,
				   bool *shell_should_exit)
{
	/* First, try to see if it's a builtin. */
	for (size_t i = 0; builtin_commands[i].name; i++) {
		if (!strcmp(builtin_commands[i].name, cmd->argv[0])) {
			/* We found a match!  Run it. */
			return builtin_commands[i].handler(
				(const char *const *)cmd->argv, last_rv,
				shell_should_exit);
		}
	}

	/* Otherwise, it's an external command. */
	return dispatch_external_command(cmd);
}

int shell_command_dispatcher(const char *input, int last_rv,
			     bool *shell_should_exit)
{
	int rv;
	struct command *parse_result;
	enum parse_error parse_error = parse_input(input, &parse_result);

	if (parse_error) {
		fprintf(stderr, "Input parse error: %s\n",
			parse_error_str[parse_error]);
		return -1;
	}

	/* Empty line */
	if (!parse_result)
		return last_rv;
	rv = dispatch_parsed_command(parse_result, last_rv, shell_should_exit);
	free_parse_result(parse_result);
	return rv;
}
