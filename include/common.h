#ifndef _COMMON_H
#define _COMMON_H

#define     PORT        8080
#define     BUFF_SIZE   4096

typedef enum {
    STATUS_SUCCESS = 0,
    STATUS_ERROR = -1
} Status_e;

typedef enum {
    MSG_HELLO_REQ
} Msg_e;

typedef struct {
    Msg_e type;
    unsigned short len;
} MsgHdr_t;

typedef enum {
    STATE_NEW,
    STATE_DISCONNECTED,
    STATE_HELLO,
    STATE_MSG
} State_e;

typedef struct {
    int fd;
    State_e state;
    char buffer[BUFF_SIZE];
} ClientState_t;


#endif /* _COMMON_H */