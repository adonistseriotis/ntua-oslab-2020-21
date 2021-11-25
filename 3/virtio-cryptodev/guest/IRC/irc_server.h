#ifndef _IRC_SERVER_H
#define _IRC_SERVER_H
#define TCP_PORT    35001
#define BACKLOG     10
#define MAX_USERS   10

#define KEY "kwstas1312geias"
#define IV "elanasouporebro"
#define KEY_SIZE        16
#define BLOCK_SIZE      16
#define DATA_SIZE       256
#define HELLO "Hello, please input username!\n"
#define USER_INERR "Please input a username with maximum 100 characters.\n"
#define ONLINE_USER "Please select user to speak to (0-N):\n"
#define NO_ONLINE_USERS "No users available"
#define TRY_IT_AGAIN "This user seems to be offline try again\n"

struct
{
    char username[20];
    int sockfd;
    int speaks_with;
    int active;
} user[10];

struct {
    unsigned char in[DATA_SIZE],
    encrypted[DATA_SIZE],
    decrypted[DATA_SIZE],
    iv[BLOCK_SIZE],
    key[KEY_SIZE];
}data;


#endif

/* Insist until all of the data has been read */
ssize_t insist_read(int fd, void *buf, size_t cnt)
{
        ssize_t ret;
        size_t orig_cnt = cnt;

        while (cnt > 0) {
                ret = read(fd, buf, cnt);
                if (ret < 0)
                        return ret;
                buf += ret;
                cnt -= ret;
        }

        return orig_cnt;
}

/* Insist until all of the data has been written */
ssize_t insist_write(int fd, const void *buf, size_t cnt)
{
	ssize_t ret;
	size_t orig_cnt = cnt;

	while (cnt > 0) {
	        ret = write(fd, buf, cnt);
	        if (ret < 0)
	                return ret;
	        buf += ret;
	        cnt -= ret;
	}

	return orig_cnt;
}

/* Common functions */ 
void clear_buff(char *buff,int n)
{
    int i;
    for(i=n; i<sizeof(buff); i++)
    {
        buff[i] = '\0';
    }
}

int encrypt (char *buff, int cfd) 
{
    //int i = 0;
    struct session_op sess;
    struct crypt_op cryp;

    memset(&data, 0, sizeof(data));
    memset(&sess, 0, sizeof(sess));
	memset(&cryp, 0, sizeof(cryp));
    //memset(&data, 0, sizeof(data));
    
    memcpy(data.key, KEY, sizeof(data.key));
    memcpy(data.iv, IV, sizeof(data.iv));
    strncpy((char *) data.in, buff, sizeof(data.in));
    
    
    #ifdef CLIEN
        printf("Data input\n");
        for(i = 0; i < DATA_SIZE; i++)
        {
            printf("%c",data.in[i]);
        }
        printf("\n");
    #endif

    sess.cipher = CRYPTO_AES_CBC;
	sess.keylen = KEY_SIZE;
	sess.key = data.key;

    if (ioctl(cfd, CIOCGSESSION, &sess)) {
		perror("ioctl(CIOCGSESSION)");
		return 1;
	}

    cryp.ses = sess.ses;
	cryp.len = sizeof(data.in);
	cryp.src = data.in;
	cryp.dst = data.encrypted;
	cryp.iv = data.iv;
	cryp.op = COP_ENCRYPT;

    #ifdef CLIEN
        printf("\nCryp.key:\n");
        for (i = 0; i < sizeof(IV); i++)
            printf("%c", cryp.iv[i]);
        printf("\n");
    #endif
    
    if (ioctl(cfd, CIOCCRYPT, &cryp)) {
		perror("ioctl(CIOCCRYPT) u bitch");
		return 1;
	}
    
    
    #ifdef CLIEN
        printf("\nEncrypted data:\n");
        for (i = 0; i < DATA_SIZE; i++) {
            printf("%c", data.encrypted[i]);
        }
        printf("\n");
    #endif

    /* Finish sesh */
    if (ioctl(cfd, CIOCFSESSION, &sess.ses)) {
		perror("ioctl(CIOCFSESSION)");
		return 1;
	}

	return 0;
}

int decrypt (char *buff, int cfd)
{
    //int i;
    struct session_op sess;
    struct crypt_op cryp;

    memset(&data, 0, sizeof(data));
    memset(&sess, 0, sizeof(sess));
	memset(&cryp, 0, sizeof(cryp));
    //memset(&data, 0, sizeof(data));
    
    memcpy(data.key, KEY, sizeof(data.key));
    memcpy(data.iv, IV, sizeof(data.iv));
    memcpy(data.encrypted, buff, sizeof(data.encrypted));

    sess.cipher = CRYPTO_AES_CBC;
	sess.keylen = KEY_SIZE;
	sess.key = data.key;

    /* Start session with driver */
    if (ioctl(cfd, CIOCGSESSION, &sess)) {
		perror("ioctl(CIOCGSESSION)");
		return 1;
	}

    cryp.ses = sess.ses;
	cryp.len = sizeof(data.encrypted);
    cryp.src = data.encrypted;
	cryp.dst = data.decrypted;
    cryp.iv = data.iv;
	cryp.op = COP_DECRYPT;

    /* Decrypt data */
	if (ioctl(cfd, CIOCCRYPT, &cryp)) {
		perror("ioctl(CIOCCRYPT)");
		return 1;
	}

    /* End session with driver 
     * Decrypted data are in data.decrypted array 
     */
    if (ioctl(cfd, CIOCFSESSION, &sess.ses)) {
		perror("ioctl(CIOCFSESSION)");
		return 1;
	}

    /* strcpy(buff, (char *) data.decrypted);
    buff[sizeof(data.decrypted)] = '\0';
    for(i=0; i<sizeof(data.decrypted); i++)
    {
        printf("This is the decrypted message:%c\n",buff[i]);
    } */
    #ifdef SERVER
        memcpy(buff,data.decrypted,sizeof(data.decrypted));
    #endif

    #ifdef CLIENT
        if (insist_write(STDOUT_FILENO, data.decrypted, sizeof(data.decrypted)) != sizeof(data.decrypted)) 
            {
                perror("write");
                exit(1);
            }
    #endif

	return 0;

}

int isNumber (char *c) {
    if (c[0] >= '0' && c[0] <= '9' && ((c[1] >= '0' && c[1] <= '9') || c[1] == '\n'))
        return 1;
    return 0;
}