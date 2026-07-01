#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

// Fully handles one accepted connection: reads the request, routes it,
// writes the response, and closes conn_fd before returning.
void handle_client(int conn_fd);

#endif
