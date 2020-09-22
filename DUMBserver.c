#include <stdlib.h>
#include <stdio.h>
#include <string.h>    //strlen
#include <sys/socket.h>
#include <arpa/inet.h>    //inet_addr
#include <errno.h>
#include <pthread.h>
#include <ctype.h>
#include <time.h>


#if 0
 struct sockaddr_in
 {
    short   sin_family; /* must be AF_INET */  2 byts
    u_short sin_port;   // 2 bytes
    struct  in_addr sin_addr; // 4 bytes
    char    sin_zero[8]; /* Not used, must be zero */  8 BYTE_SIZE
};

struct sockaddr {
    unsigned short sa_family; // 2
    char sa_data[14];  // 14
};

typedef uint32_t in_addr_t;

struct in_addr {
    in_addr_t s_addr;  // 4 byts
};


#endif



typedef struct msgq_t{
    struct msgq_t *nextMsg;
    char *pMsg;
} MSG_QUEUE;

typedef struct mb_t {
    struct mb_t *nextMB;
    char mbName[26];
    MSG_QUEUE *pQHead;
    MSG_QUEUE *pQTail;
    int status; // 1=open, 0=closed
    int user_opened;
}MAIL_BOX;

typedef struct connect_t {
    int sock_desc; // socket
    struct sockaddr_in client_addr;
    struct connect_t *nextCon;
    MAIL_BOX* pMyMB;
}CONNECTION;

CONNECTION* socklist = NULL;
MAIL_BOX * pMBList =  NULL;
MSG_QUEUE * pMsgQ = NULL;
void logLine(CONNECTION *pClient, const char *pEv, FILE *pOutputDev)
{
    char month[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    time_t secs = time(NULL);
    struct tm *pNow = localtime(&secs);
    char zNow[32];
    memset(zNow , 0, sizeof(zNow));
    if(pNow){
        sprintf(zNow,"%d %s", pNow->tm_mday, month[pNow->tm_mon]);
    }

    fprintf(pOutputDev, "%d %s %s %s\n", pClient->sock_desc, zNow,
            inet_ntoa(pClient->client_addr.sin_addr), pEv);
    return;
}


int createMB(CONNECTION *pClient, const char *pMsg, char *pRsp)
{
    unsigned long len = strlen(pMsg);
    if(pMsg == NULL || pMsg[0] != ' ' || isalpha(pMsg[1]) == 0 || (len < 5 || len > 25)){
        strcpy(pRsp, "ER:WHAT?");
        logLine(pClient, pRsp, stderr);
        return -1;
    }
    MAIL_BOX* tempMB = pMBList;
    MAIL_BOX* prevMB = pMBList;
    // check if MB exist
    while(tempMB != NULL){
        if(strcmp(tempMB->mbName, pMsg+1)==0){
            strcpy(pRsp, "ER:EXIST");
            logLine(pClient, pRsp, stderr);
            return -1;
        }
        prevMB = tempMB;
        tempMB = tempMB->nextMB;
    }
    //created new mailbox
    tempMB = (MAIL_BOX*)malloc(sizeof(MAIL_BOX));
    // Set name
    strcpy(tempMB->mbName, pMsg+1);
    tempMB->nextMB = NULL;
    // set message queue head and tail
    tempMB->pQHead = NULL;
    tempMB->pQTail = NULL;
    tempMB->status = 0;
    if(pMBList == NULL){
        pMBList = tempMB;
    }
    else {
        prevMB->nextMB = tempMB;
    }
    logLine(pClient, "CREAT", stdout);
    strcpy(pRsp, "OK!");
    
    return 0;
}
int openMB(CONNECTION *pClient, const char *pMsg, char *pRsp)
{
    unsigned long len = strlen(pMsg);
    if(pMsg == NULL || pMsg[0] != ' ' || isalpha(pMsg[1]) == 0 || (len < 5 || len > 25)){
       // not valid mailbox name
        strcpy(pRsp, "ER:WHAT?");
        logLine(pClient, pRsp, stderr);
        return -1;
    }
    // find the mailbox to open
    MAIL_BOX* tempMB = pMBList;
    while(tempMB != NULL){
        if(strcmp(tempMB->mbName, pMsg+1)==0){
            if(tempMB->status == 1){
                // MB is open
                strcpy(pRsp, "ER:OPEND");
                logLine(pClient, pRsp, stderr);
                return -1;
            }
            else{
                // open mailbox
                tempMB->status = 1;
                tempMB->user_opened = pClient->sock_desc;
                pClient->pMyMB = tempMB;
                logLine(pClient, "OPNBX", stdout); //not opened
                strcpy(pRsp, "OK!");
                return 0;
            }
        }
        tempMB = tempMB->nextMB;
    }
    //if reaches here -> MB DNE
    strcpy(pRsp, "ER:NEXST");
    logLine(pClient, pRsp, stderr);
    return -1;
}


int closeMB(CONNECTION *pClient, const char *pMsg, char *pRsp){
    unsigned long len = strlen(pMsg);
    // validate mailbox name
    if(pMsg == NULL || pMsg[0] != ' ' || isalpha(pMsg[1]) == 0 || (len < 5 || len > 25)){
        strcpy(pRsp, "ER:WHAT?");
        logLine(pClient, pRsp, stderr);
        return -1;
    }
    // find mailbox
    MAIL_BOX* tempMB = pMBList;
    while(tempMB != NULL){
        if(strcmp(tempMB->mbName, pMsg+1)==0){
            if(tempMB->status == 0){
                strcpy(pRsp, "ER:NOOPN");
                logLine(pClient, pRsp, stderr);
                return -1;
            }
            else if(tempMB->user_opened ==pClient->sock_desc){
                tempMB->status=0;
                logLine(pClient, "CLSBX", stdout);
                strcpy(pRsp, "OK!");
                return 0;
            }
            else {
                strcpy(pRsp, "ER:NOOPN");
                logLine(pClient, pRsp, stderr);
                return -1;
            }
        }
        tempMB = tempMB->nextMB;
    }
    //if reaches here -> MB DNE
    strcpy(pRsp, "ER:NEXST");
    logLine(pClient, pRsp, stderr);
    return -1;
}

int putMG(CONNECTION *pClient, const char *pMsg, char *pRsp)
{
    // check if mailbox is open
    if(pClient->pMyMB->status == 0){
        strcpy(pRsp, "ER:NOOPN");
        logLine(pClient, pRsp, stderr);
        return -1;
    }
    // check if PUTMG is followed by !
    if(pMsg[0] != '!'){
        strcpy(pRsp, "ER:WHAT?");
        logLine(pClient, pRsp, stderr);
        return -1;
    }
    //
    pMsg = pMsg + 1;
    // get message length assuming it will not be longer than 9
    int i=0;
    char num[8];
    while(isdigit(pMsg[i]) != 0){
        num[i]=pMsg[i];
        i++;
        if(i>5){
            strcpy(pRsp, "ER:WHAT?");
            logLine(pClient, pRsp, stderr);
            return -1;
        }
    }
    num[i]='\0';
    int size = atoi(num);
    if(pMsg[i]!='!'){
        strcpy(pRsp, "ER:WHAT?");
        logLine(pClient, pRsp, stderr);
        return -1;
    }
    char* msg = (char*)malloc(size);
    pMsg = pMsg + i + 1;
    strcpy(msg, pMsg);
    MSG_QUEUE* pm = (MSG_QUEUE*)malloc(sizeof(MSG_QUEUE));
    pm->pMsg = msg;
    pm->nextMsg = NULL;
    
    if(pClient->pMyMB->pQHead == NULL){
        pClient->pMyMB->pQHead = pm;
        pClient->pMyMB->pQTail = pm;
    }
    else{
        pClient->pMyMB->pQTail->nextMsg = pm;
        pClient->pMyMB->pQTail = pm;
        pClient->pMyMB->pQTail->nextMsg = NULL;
    }
    logLine(pClient, "PUTMG", stdout);
    strcpy(pRsp, "OK!");
    
    return 0;
}

int nextMG(CONNECTION *pClient, const char *pMsg, char *pRsp){
    MAIL_BOX* currMB = pClient->pMyMB;
    if(currMB == NULL){ //no message box open
        strcpy(pRsp, "ER:NOOPN");
        logLine(pClient, pRsp, stderr);
        return -1;
    }
    if(currMB->pQHead == NULL){ //no messages in box
        strcpy(pRsp, "ER:EMPTY");
        logLine(pClient, pRsp, stderr);
        return -1;
    }
    unsigned long len = strlen(currMB->pQHead->pMsg);
    sprintf(pRsp, "OK!%lu!%s", len, currMB->pQHead->pMsg);
    logLine(pClient, "NXTMG", stdout);
    struct msgq_t *temp = currMB->pQHead;
    temp = currMB->pQHead->nextMsg;
    free(currMB->pQHead->pMsg);
    free(currMB->pQHead);
    currMB->pQHead = temp;
    return 0;
}

int deleteMB(CONNECTION *pClient, const char *pMsg, char *pRsp){
    // validate mailbox name
    unsigned long len = strlen(pMsg);
    if(pMsg == NULL || pMsg[0] != ' ' || isalpha(pMsg[1]) == 0 || (len < 5 || len > 25)){
        strcpy(pRsp, "ER:WHAT?");
        logLine(pClient, pRsp, stderr);
        return -1;
    }
    // find mailbox
    MAIL_BOX* tempMB = pMBList;
    MAIL_BOX* prevMB = pMBList;
    while(tempMB != NULL){
        if(strcmp(tempMB->mbName, pMsg+1)==0){
            // found mailbox
            if(tempMB->status == 1){
                strcpy(pRsp, "ER:OPEND");
                logLine(pClient, pRsp, stderr);
                return -1;
            }
            else{
                //check if it has any msgs
                if(tempMB->pQHead != NULL){
                    strcpy(pRsp, "ER:NOTMT");
                    logLine(pClient, pRsp, stderr);
                    return -1;
                }
                logLine(pClient, "DELBX", stdout);
                strcpy(pRsp, "OK!");
                prevMB->nextMB = tempMB->nextMB;
                free(tempMB);
                return 0;
            }
        }
        prevMB = tempMB;
        tempMB = tempMB->nextMB;
    }
    //if reaches here -> MB DNE
    strcpy(pRsp, "ER:NEXST");
    logLine(pClient, pRsp, stderr);
    return -1;
}

int closeOpenMB(CONNECTION *pClient)
{
    MAIL_BOX* tempMB = pMBList;
    while(tempMB != NULL){
        if((tempMB->status == 1) && (tempMB->user_opened ==pClient->sock_desc)){
            tempMB->status = 0;
            tempMB->user_opened = 0;
        }
        tempMB = tempMB->nextMB;
    }
    return 0;
}

int processMessage(CONNECTION *pClient, const char *pRxMsg, char *pTxMsg)
{
    if(pRxMsg){
       // printf("msg=%s\n", pRxMsg);
        if(strncmp(pRxMsg, "HELLO", 5) == 0){
            // print event HELLO
            logLine(pClient, "HELLO", stdout);
            strcpy(pTxMsg, "HELLO DUMBv0 ready!");
        }
        else if(strncmp(pRxMsg, "GDBYE", 5 ) == 0){
            closeOpenMB(pClient);
            logLine(pClient, "GDBYE", stdout);
            return 1;
        }
        else if(strncmp(pRxMsg, "CREAT", 5 ) == 0){
            strcpy(pTxMsg, "Got CREAT");
            createMB(pClient, pRxMsg + 5, pTxMsg);
            
        }
        else if(strncmp(pRxMsg, "OPNBX", 5 ) == 0){
            //strcpy(pTxMsg, "Got OPNBX");
            openMB(pClient, pRxMsg + 5, pTxMsg);
        }
        else if(strncmp(pRxMsg, "NXTMG", 5 ) == 0){
            strcpy(pTxMsg, "Got NXTMG");
            nextMG(pClient, pRxMsg, pTxMsg);
        }
        else if(strncmp(pRxMsg, "PUTMG", 5 ) == 0){
            strcpy(pTxMsg, "Got PUTMG");
            putMG(pClient, pRxMsg + 5, pTxMsg);
        }
        else if(strncmp(pRxMsg, "DELBX", 5 ) == 0){
            deleteMB(pClient, pRxMsg + 5, pTxMsg);
            //strcpy(pTxMsg, "Got DELBX");
        }
        else if(strncmp(pRxMsg, "CLSBX", 5 ) == 0){
            //strcpy(pTxMsg, "Got CLSBX");
            closeMB(pClient, pRxMsg + 5, pTxMsg);
        }
        else {
            strcpy(pTxMsg, "Got ERROR");
        }
    }
    return 0;
}

void *connectionHandler(void *pConn)
{
    //Get the socket descriptor
    CONNECTION *pClient = (CONNECTION *)pConn;
    int rx_size;
    size_t ret;
    char rx_msg[2000];
    char tx_msg[2000];
    //size_t len;
    //Receive a message from client
    errno = 0;
    logLine(pClient, "connected", stdout);
    while( (rx_size = (int)recv(pClient->sock_desc, rx_msg , 2000 , 0)) > -1 )
    {
        if(rx_size == 0){
            continue;
        }
        rx_msg[rx_size] = '\0';
        if(processMessage(pClient, rx_msg, tx_msg) == 0){
            ret = send(pClient->sock_desc , tx_msg , (size_t)strlen(tx_msg), 0);
            if(ret == -1){
                break;
            }
           // printf("sent msg \"%s\" to client\n", tx_msg);
        }
        else {
            // remove socket from link list
            CONNECTION *pPrev = socklist;
            CONNECTION *pCur = socklist;
            while(pCur != NULL){
                if(pCur->sock_desc == pClient->sock_desc){
                    // remove socket
                    pPrev->nextCon = pCur->nextCon;
                    free(pCur);
                    break;
                }
                pPrev = pCur;
                pCur = pCur->nextCon;
            }
            return NULL;
        }
    }
    printf("socket io failed errno=%d\n", errno);
    return NULL;
}

//#define MAX_CLIENTS 1024

int main(int argc , char *argv[])
{
    if(argc < 2 ){
        printf("Usage: ./DUMBServer <listening port>\n");
        return -1;
    }
    int port = atoi(argv[1]);
    if((port < 0x1000) || (port > 0xffff)) {
        printf("port %d is not in the valid range, valid range is 4097 to 65535\n", port);
        return -2;
    }
    int len;
    int socket_desc;
    struct sockaddr_in server , client;
    errno = 0;
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket, errno=%d", errno);
        return -3;
    }
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port); // listening port
    
    //Bind
    errno = 0;
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)))
    {
        printf("bind to port %d failed, errno=%d\n", port, errno);
        return -4;
    }
    
    //Listen
    errno = 0;
    if(listen(socket_desc , 3)) {
        printf("failed to listen on port %d, errno=%d\n", port, errno);
        return -5;
    }
    
    //Accept and incoming connection
  //  printf("Waiting on port %d for incoming connections\n", port);
    len = sizeof(struct sockaddr_in);
    //int client_count = 0;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_t clientTid;
    //int client_sok[MAX_CLIENTS];
    int cSock;
    
    //pthread_t *pTid = (pthread_t *)malloc((unsigned long)MAX_CLIENTS * sizeof(pthread_t));
    while(1)
    {
        errno = 0;
        cSock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&len);
        
        if(cSock < 0) {
            printf("Failed to accept connection errno=%d\n", errno);
            continue;
        }
        
        CONNECTION* pSock = (CONNECTION *)malloc(sizeof(CONNECTION));
        pSock->sock_desc = cSock;
        pSock->client_addr = client;
        pSock->nextCon = NULL;
        // find last connection
        CONNECTION *pSockTemp = socklist;
        while(pSockTemp!= NULL){
            pSockTemp = pSockTemp->nextCon;
        }
        pSockTemp = pSock;
        
        if( pthread_create(&clientTid, &attr ,  (void *)connectionHandler, (void *)pSock) < 0)
        {
            printf("could not create client handling thread\n");
            continue;
        }
    }
    return 0;
}



