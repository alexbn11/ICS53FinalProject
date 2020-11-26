#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

// These are the message types for the PETR protocol
enum msg_types {
    OK,
    LOGIN = 0x10,
    LOGOUT,
    EUSREXISTS = 0x1a,
    RMCREATE = 0x20,
    RMDELETE,
    RMCLOSED,
    RMLIST,
    RMJOIN,
    RMLEAVE,
    RMSEND,
    RMRECV,
    ERMEXISTS = 0x2a,
    ERMFULL,
    ERMNOTFOUND,
    ERMDENIED,
    USRSEND = 0x30,
    USRRECV,
    USRLIST,
    EUSRNOTFOUND = 0x3a,
    ESERV = 0xff
};

// This is the struct describes the header of the PETR protocol messages
typedef struct {
    uint32_t msg_len; // this should include the null terminator
    uint8_t msg_type;
} petr_header;

int rd_msgheader(int socket_fd, petr_header *h);
int wr_msg(int socket_fd, petr_header *h, char *msgbuf);

#endif