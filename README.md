### Project Description:
The program implements a customized shell that supports Linux shell commands in addition to the internal commands. The program also supports several I/O of redirection.

### Compiling Instruction:
To make, type: make <br>
To run, type: ./shell <br>
To clean, type: make clean <br>

### A Sample Test Run:
$ path + /bin <br>
$ path + /sbin <br>
$ path + /usr/bin <br>
$ path <br>
/bin:/sbin:/usr/bin <br> <br>
$ pwd <br>
/Users/yuseffperry/downloads/yuperry <br> <br>
$ cd .. <br>
$ pwd <br>
/Users/yuseffperry/Downloads <br> <br>
$ cd yuperry <br>
$ pwd <br>
/Users/yuseffperry/downloads/yuperry <br> <br>
$ who <br>
yuseffperry console  Jul  5 16:53 <br>
yuseffperry ttys000  Jul 11 09:35 <br> <br>
$ who | sort | wc -l <br>
       2 <br> <br>
$ wc < shell.c <br>
     318    1033    7650 <br> <br>
$ grep <br>
usage: grep [-abcDEFGHhIiJLlmnOoqRSsUVvwxZ] [-A num] [-B num] [-C[num]] <br>
	[-e pattern] [-f file] [--binary-files=value] [--color=when] <br>
	[--context[=num]] [--directories=action] [--label] [--line-buffered] <br>
	[--null] [pattern] [file ...] <br> <br>
$ ls <br>
Makefile	README.txt	shell.c <br> <br>
$ cat Makefile <br>
CC=gcc <br>
CFLAGS=-Wall -g -D_GNU_SOURCE <br>
LDFLAGS=-lreadline <br>

shell: shell.c <br>
	$(CC) $(CFLAGS) shell.c -o shell $(LDFLAGS) <br>

clean: <br>
	rm -rf ./shell <br> <br>
$ cat Makefile | wc -l <br>
       9 <br> <br>
$ sort < Makefile <br>
$(CC) $(CFLAGS) shell.c -o shell $(LDFLAGS) <br>
rm -rf ./shell <br>
CC=gcc <br>
CFLAGS=-Wall -g -D_GNU_SOURCE <br>
LDFLAGS=-lreadline <br>
clean: <br>
shell: shell.c <br> <br>
$ sort < Makefile > Makefile.sorted <br>
$ wc -l Makefile.sorted <br>
       9 Makefile.sorted <br> <br>
$ path - /usr/bin <br>
$ path - /sbin <br>
$ path - /bin <br>
$ path <br>
empty
