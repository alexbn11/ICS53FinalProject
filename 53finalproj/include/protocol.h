#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

// These are the message types for the PETR protocol
enum msg_types {
    OK = 0x00,
    LOGIN = 0x10,
    LOGOUT = 0x11,
    EUSREXISTS = 0x1a,
    RMCREATE = 0x20,
    RMDELETE = 0x21,
    RMCLOSED = 0x22,
    RMLIST = 0x23,
    RMJOIN = 0x24,
    RMLEAVE = 0x25,
    RMSEND = 0x26,
    RMRECV = 0x27,
    ERMEXISTS = 0x2a,
    ERMFULL = 0x2b,
    ERMNOTFOUND = 0x2c,
    ERMDENIED = 0x2d,
    USRSEND = 0x30,
    USRRECV = 0x31,
    USRLIST = 0x32,
    EUSRNOTFOUND = 0x3a,
    ESERV = 0xff
};

// This is the struct describes the header of the PETR protocol messages
typedef struct {
    uint32_t msg_len; // this should include the null terminator
    uint8_t msg_type;
} petr_header;

// Read a petr_header from the socket_fd. Places content into memory.
// Referenced by h. Returns 0 upon success, -1 on error.
int rd_msgheader(int socket_fd, petr_header *h);

// Write petr_header reference by h and the msgbuf to the socket_fd
// If the msg_len is 0, msg_buf is ignored
// Returns 0 upon success, -1 on error.
int wr_msg(int socket_fd, petr_header *h, char *msgbuf);

#endif