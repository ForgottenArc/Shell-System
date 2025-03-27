A small shell built in C for an Operating Systems course. It handles basic Unix commands, runs them in child processes, and supports I/O redirection and simple piping.

Features
Runs built-in and external commands

Supports input (<) and output (>) redirection

Simple piping with |

Uses fork(), execvp(), and wait() under the hood

Error handling for unknown commands or bad syntax

Compile it with:
gcc -o simpleshell shell.c

Then run: 
./simpleshell  

You’ll get a custom prompt. Try running commands like:
ls -l
cat file.txt > out.txt
sort < data.txt | uniq


Notes
This was mainly for learning how the shell works under the hood.

Doesn’t support advanced features like background jobs or complex syntax (yet).

Tested on Linux (Ubuntu), should work on most Unix-like systems.


