#ifndef _COMMON_H
#define _COMMON_H

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


#endif /* _COMMON_H */