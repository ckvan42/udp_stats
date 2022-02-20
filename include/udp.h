////
//// Created by Giwoun Bae on 2022-02-18.
////
//
//#ifndef DC_UDP_UDP_H
//#define DC_UDP_UDP_H
//
//#include "udp_structures.h"
//
//void udp_init(const struct dc_posix_env *env, struct dc_error *err, void * setting, void * state)
//{
//    int       sockfd;
//    struct    sockaddr_in  servaddr;
//    size_t     n;
//
//    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
//    {
//        perror("socket creation failed");
//        exit(EXIT_FAILURE);
//    }
//
//    memset(&servaddr, 0, sizeof(servaddr));
//
//    servaddr.sin_family = AF_INET;
//    servaddr.sin_port = htons(PORT);
//    servaddr.sin_addr.s_addr = INADDR_ANY;
//}
//
//
//
//
//
//
//#endif //DC_UDP_UDP_H
