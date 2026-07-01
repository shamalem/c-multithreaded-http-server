# c-multithreaded-http-server

A multithreaded HTTP server written in C17 using POSIX sockets and pthreads,
built incrementally as a learning project covering TCP/IP, the HTTP protocol,
Linux system calls, and concurrent programming with a thread pool.

This project is under active, milestone-by-milestone development. The design
notes and rationale for each milestone are being discussed and recorded as the
project grows; this README will be expanded into full documentation
(architecture diagram, performance notes, future work) once the server is
feature-complete.

## Roadmap

- [ ] M1: Minimal TCP listening socket (socket/bind/listen/accept/close)
- [ ] M2: Accept loop + first HTTP response
- [ ] M3: HTTP request line/header parsing
- [ ] M4: Serving static files from disk
- [ ] M5: Robust error handling (400/404/405/500) + logging module
- [ ] M6: Single-threaded server's concurrency limits (motivation for threads)
- [ ] M7: Thread-per-connection model with pthreads
- [ ] M8: Thread pool (producer-consumer, mutex + condition variable)
- [ ] M9: Graceful shutdown & signal handling
- [ ] M10: Testing (unit tests), gdb & Valgrind debugging passes
- [ ] M11: Polish - architecture diagram, CI (GitHub Actions), Docker, docs

## Building

```sh
make          # build ./server
make run      # build and run
make asan     # build with AddressSanitizer + UBSan for local debugging
make clean
```

## Project layout

```
src/      implementation files
include/  public headers
tests/    unit tests
docs/     architecture notes, diagrams
```
