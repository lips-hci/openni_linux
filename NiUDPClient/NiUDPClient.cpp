#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#if defined(COMPRESS)
#include "zlib.h"
#endif

#if defined(RES_VGA)
#define IMG_WIDTH 640
#define IMG_HEIGHT 480
#else
#define IMG_WIDTH 320
#define IMG_HEIGHT 240
#endif
#define SERVER_PORT 5566
#define CLIENT_PORT 5567
#define MAX_DATA 1024
#define PKT_SIZE 51200
#define MAX_COUNT 20
#define FID_SIZE 11
#if defined(COMPRESS)
#define PAYLOAD_SIZE 6
#endif

using namespace std;
using namespace cv;

int main( int argc, char* argv[] )
{
    int socket_fd;
    int size;
    char data[MAX_DATA] = {0};
    char *data1;
#if !defined(COMPRESS)
    data1 = (char*)malloc((PKT_SIZE+FID_SIZE)*sizeof(char));
#else
    data1 = (char*)malloc((PKT_SIZE+FID_SIZE+PAYLOAD_SIZE)*sizeof(char));
#endif
    char *data_all;
    data_all = (char*)malloc(IMG_WIDTH*IMG_HEIGHT*2*sizeof(char));
    struct sockaddr_in myaddr;
    struct sockaddr_in server_addr;
    struct hostent *hp;

    if ( argc < 2 ) {
        cout << "No parameter" << endl;
        return 1;
    }

    if ((socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 ) {
        cout << "socket failed" << endl;
        return 1;
    }

    memset(&myaddr, 0, sizeof(myaddr));
    memset(&server_addr, 0, sizeof(server_addr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(CLIENT_PORT);
    if (bind(socket_fd, (struct sockaddr*)&myaddr, sizeof(myaddr)) < 0) {
        cout << "bind socket failed" << endl;
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    hp = gethostbyname(argv[1]);
    if (hp == 0) {
        cout << "could not obtain address" << endl;
        return 1;
    }
    memcpy(hp->h_addr_list[0], (caddr_t)&server_addr.sin_addr, hp->h_length); 
    strncpy(data, "done", MAX_DATA);

    size = sizeof(server_addr);

    char serial[11] = {0};
    char now_serial[11] = {0};
#if defined(COMPRESS)
    char payload_size[PAYLOAD_SIZE] = {0};
    uLong len = 0;
#endif
    int counter = 0;
    while ( true )
    {
#if !defined(COMPRESS)
        recvfrom(socket_fd, data1, PKT_SIZE + FID_SIZE, 0, (struct sockaddr*)&server_addr, (socklen_t *)&size);
#else
        recvfrom(socket_fd, data1, PKT_SIZE + FID_SIZE + PAYLOAD_SIZE, 0, (struct sockaddr*)&server_addr, (socklen_t *)&size);
#endif
        unsigned int offset = *data1 - 'A';
        memcpy(serial, data1 + 1, FID_SIZE-1);
        counter++;
        //cout << "offset : " << offset << ", serial : " << serial << ", now_serial : " << now_serial << ", counter : " << counter << endl;
        if (!strcmp(now_serial, "")) {
            strcpy(now_serial, serial);
        } else if (strcmp(now_serial, serial)) {
            strcpy(now_serial, serial);
#if !defined(COMPRESS)
#if defined(RES_VGA)
            if (counter > 9) {
#else
            if (counter > 2) {
#endif
#else
            {
                int err;
                Byte *uzData;
                uzData = (Byte*)calloc(IMG_WIDTH*IMG_HEIGHT*2, 1);
                uLong uzDataLen = (uLong)(IMG_WIDTH*IMG_HEIGHT*2);
                err = uncompress(uzData, &uzDataLen, (Bytef*)(data_all), len);
                if (err != Z_OK) {
                    switch (err) {
                        case Z_ERRNO:
                            cout << "uncompress error: Z_ERROR" << endl;
                            break;
                        case Z_STREAM_ERROR:
                            cout << "uncompress error: Z_STREAM_ERROR" << endl;
                            break;
                        case Z_DATA_ERROR:
                            cout << "uncompress error: Z_DATA_ERROR" << endl;
                            break;
                        case Z_MEM_ERROR:
                            cout << "uncompress error: Z_MEM_ERROR" << endl;
                            break;
                        case Z_BUF_ERROR:
                            cout << "uncompress error: Z_BUF_ERROR" << endl;
                            break;
                        case Z_VERSION_ERROR:
                            cout << "uncompress error: Z_VERSION_ERROR" << endl;
                            break;
                        default:
                            cout << "uncompress error:" << err << endl;
                            break;
                    }
                    cout << "uzDataLen: " << uzDataLen << ", len: " << len << endl;
                }
#endif
#if !defined(COMPRESS)
                Mat imgDepth( IMG_HEIGHT, IMG_WIDTH, CV_16UC1, ( void* )data_all );
#else
                Mat imgDepth( IMG_HEIGHT, IMG_WIDTH, CV_16UC1, ( void* )uzData );
#endif
                Mat img8bitDepth;
                imgDepth.convertTo( img8bitDepth, CV_8U, 255.0 / 5000 );
                imshow( "Depth view", img8bitDepth );
                waitKey( 1 );
#if defined(COMPRESS)
                free(uzData);
#endif
            }
            counter = 0;
#if defined(COMPRESS)
            len = 0;
#endif
        }
#if !defined(COMPRESS)
        memcpy(data_all + offset * PKT_SIZE , data1 + FID_SIZE, PKT_SIZE);
#else
        memcpy(payload_size, data1 + FID_SIZE, PAYLOAD_SIZE - 1);
        cout << "payload_size : " << payload_size << "(" << atoi(payload_size) << ")" << endl;
        memcpy(data_all + offset * PKT_SIZE, data1 + FID_SIZE + PAYLOAD_SIZE, atoi(payload_size));
        len += atoi(payload_size);
#endif
    }

    free(data1);
    free(data_all);
    return 0;
}
