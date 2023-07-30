# CSCI-442-Project-2-UNIX-Shell
#### using C

## Implemented a UNIX-like bash shell that supports an arbitrary number of commands associated with files on the filesystem and pipes, enabling pipelines with file redirection.
## For example: arg1 arg2 arg3 < input_file arg4 | cmd2_arg1 >> append_file.


#### 1. Utilized any of the exec* family of library functions (such as execvp(3)) to find the command in the PATH to execute the command using fork(2) and execve(2).

#### 2. Used the pipe(2) system call to handle an arbitrary number of commands piped together.
##### For an example of a real piped command:
##### command1 < inputfile | command2 | command3 > outputfile

#### 3. Handled file redirection using > (overwrite to a file), >> (append to a file), or < (input from a file).
##### For example file redirection:
##### command > file-to-write-or-overwrite.txt
