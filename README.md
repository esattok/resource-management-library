# Resource Management Library

- Application that manages same or differnet kinds of resources among the clients
- The clients are simulated by seperate threads
- If the `avoid-flag` is set to 1, deadlock avoidance is applied to manage the resources
- If the `avoid-flag` is set to 0, deadlock detection is applied and the occuring deadlocks can be seen
- Banker's Algorithm is used as deadlock avoidance algorithm
- The application is developed on Linux operating system using C programming language

## Contents

- Project3.pdf (Project Description)
- rm.c (Source File)
- rm.h (Source File)
- myapp.c (Source File)
- Makefile (Makefile to Compile the Project)

## How to Run

- cd to the project directory

##### Compilation and linking

```
$ make
```

##### Recompile

```
$ make clean
$ make
```

##### Running the program

```
$ ./myapp <avoid-flag>
```

##### Example proctopk run

```
$ make
$ ./myapp 1
```
