
//# g++ -pthread -std=c++17 -o server.out server_linux.cpp

#define _CRT_SECURE_NO_WARNINGS

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <iostream>
#include <string>
#include <cstring>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>

void memcpy_s(void *dst, unsigned int sz1, const void *src, unsigned int sz2)
{
    memcpy(dst, src, sz1);
}

#include "data_structure.hpp"

const int waiting_connect_max = 20;
const int service_connect_max = 50;
int connect_cnt = 0;

DataTime GetDataTime()
{
    DataTime time = {};
    struct timeval tv; 
    struct timezone tz; 
    struct tm *p; 
    gettimeofday(&tv, &tz); 
    p = localtime(&tv.tv_sec);

    time.year = 1900 + p->tm_year;
    time.month = 1 + p->tm_mon;
    time.day = p->tm_mday;
    time.hour = p->tm_hour;
    time.minute = p->tm_min;
    time.second = p->tm_sec;
    time.millisecond = tv.tv_usec / 1000;

    return time;
}

bool OnceRead(int sk, char *buf, int expect_len)
{
    int ret;
    int read_len = 0;
    int cnt = 0;

    while (read_len < expect_len)
    {
        ret = recv(sk, buf + read_len, expect_len - read_len, 0);
        if (ret < 0)
        {
            return false;
        }
        read_len += ret;
        if (++cnt >= 200)
        {
            break;
        }
        usleep(10 * 1000);
    }
    return read_len == expect_len;
}

Data *GetData(int sk)
{
    bool ret;
    unsigned int header = 0;
    int len = 0;
    DataType dt = {};
    char *buf = nullptr;

    ret = OnceRead(sk, (char*)&header, sizeof(unsigned int));
    if (!ret || header != message_header1)
    {
        return nullptr;
    }
    ret = OnceRead(sk, (char*)&header, sizeof(unsigned int));
    if (!ret || header != message_header2)
    {
        return nullptr;
    }
    ret = OnceRead(sk, (char*)&len, sizeof(int));
    if (!ret)
    {
        return nullptr;
    }

    Memory mem = {};
    mem.len = len;
    buf = new char[len];
    *(int*)buf = len;

    if (len > sizeof(int))
    {
        ret = OnceRead(sk, buf + sizeof(int), len - sizeof(int));
        if (!ret)
        {
            return nullptr;
        }
    }
    mem.mem = buf;

    Data *data = nullptr;
    data = MemoryToData(mem);

    delete []buf;
    return data;
}

bool SendData(int sk, Data *data)
{
    Memory *m = data->GenerateMemory();
    if (!m || m->len <= 0)
    {
        print("[ERROR] SendData() fail, invalid data");
        return false;
    }

    int len = m->len;
    int pkg_len = len + sizeof(unsigned int) * 2;
    char *buf = new char[pkg_len];
    int ofst = 0;

    memcpy_s(buf + ofst, sizeof(unsigned int), &message_header1, sizeof(unsigned int));
    ofst += sizeof(unsigned int);
    memcpy_s(buf + ofst, sizeof(unsigned int), &message_header2, sizeof(unsigned int));
    ofst += sizeof(unsigned int);
    memcpy_s(buf + ofst, m->len, m->mem, m->len);
    ofst += m->len;

    delete []m->mem;
    delete m;

    bool ret = send(sk, buf, pkg_len, 0) == pkg_len;

    delete []buf;
    return ret;
}

mutex &GlobalChatInfoMutex()
{
    static mutex ChatInfoMutex = {};
    return ChatInfoMutex;
}

ChatInformation &GlobalChatInfo()
{
    static ChatInformation globle_info = {};
    return globle_info;
}

void GlobalChatInfoRead(ChatInformation &info)
{
    GlobalChatInfoMutex().lock();
    info = GlobalChatInfo();
    GlobalChatInfoMutex().unlock();
}

void GlobalChatInfoAppendMsg(ChatMessage cm)
{
    GlobalChatInfoMutex().lock();
    GlobalChatInfo().vCM.push_back(cm);
    if (GlobalChatInfo().vCM.size() > 30)
    {
        auto &vcm = GlobalChatInfo().vCM;
        vector<ChatMessage> newVCM;
        for (int i = vcm.size() - 20; i < vcm.size(); ++i)
        {
            newVCM.push_back(vcm[i]);
        }
        vcm.swap(newVCM);
    }
    GlobalChatInfoMutex().unlock();
}

void GlobalChatInfoAddMember(wstring member)
{
    GlobalChatInfoMutex().lock();
    GlobalChatInfo().member_list.push_back(member);
    sort(GlobalChatInfo().member_list.begin(), GlobalChatInfo().member_list.end());
    GlobalChatInfoMutex().unlock();
}

void GlobalChatInfoRemoveMember(wstring member)
{
    GlobalChatInfoMutex().lock();
    int idx = -1;
    for (int i = 0; i < GlobalChatInfo().member_list.size(); ++i)
    {
        if (GlobalChatInfo().member_list[i] == member)
        {
            idx = i;
            break;
        }
    }
    vector<wstring> new_member_list;
    for (int i = 0; i < GlobalChatInfo().member_list.size(); ++i)
    {
        if (i != idx)
        {
            new_member_list.push_back(GlobalChatInfo().member_list[i]);
        }
    }
    GlobalChatInfo().member_list.swap(new_member_list);
    GlobalChatInfoMutex().unlock();
}

void ClientServiceOfInput(int sock)
{
    while (1)
    {
        Data *d = GetData(sock);
        if (d)
        {
            if (!d || d->MyType() != ClientMessage)
            {
                break;
            }
            auto cm = ((Data_ClientMessage*)d)->GetMsg();
            GlobalChatInfoAppendMsg(cm);
        }
        else
        {
            break;
        }
    }
}

void ClientServiceOfDisplay(int sock)
{
    while (1)
    {
        ChatInformation ci;
        GlobalChatInfoRead(ci);
        Data_ServerStatistics *statistics = new Data_ServerStatistics(GetDataTime(), ci);
        bool send_ret = SendData(sock, statistics);
        delete statistics;
        if (!send_ret)
        {
            print("SendData() fail, disconnect ...");
            break;
        }
        usleep(500 * 1000);
    }
}

void ClientServiceProcess(int *p_sock, sockaddr_in *p_addr, socklen_t *p_addr_len)
{
    int ret;
    int &client_sock = *p_sock;
    sockaddr_in &client_addr = *p_addr;
    socklen_t &addr_len = *p_addr_len;
    wstring identity;
    ChatMessage cm;

    Data *d = GetData(client_sock);
    if (!d || d->MyType() != ClientMessage)
    {
        print("Client <", inet_ntoa(client_addr.sin_addr), "> connect error ");
        goto l_done;
    }

    cm = ((Data_ClientMessage*)d)->GetMsg();
    identity = ((Data_ClientMessage*)d)->GetMsg().msg;

    if (identity.size() && identity[0] == 'I')
    {
        print("Client <", inet_ntoa(client_addr.sin_addr), "> connected as input box");
        ClientServiceOfInput(client_sock);
    }
    else if (identity.size() && identity[0] == 'D')
    {
        print("Client <", inet_ntoa(client_addr.sin_addr), "> connected as display");
        ClientServiceOfDisplay(client_sock);
    }
    else
    {
        print("Client <", inet_ntoa(client_addr.sin_addr), "> connect identity error, id is :", identity);
        goto l_done;
    }

l_done:
    print("Client <", inet_ntoa(client_addr.sin_addr), "> service exit");

    ret = close(client_sock);
    if (ret != 0)
    {
        print("[ERROR] Client <", inet_ntoa(client_addr.sin_addr), "> closesocket() fail");
    }

    delete p_addr_len;
    delete p_addr;
    delete p_sock;

    __sync_fetch_and_sub(&connect_cnt, 1);
}

int main(int argc, char *argv[])
{
    int ret;

    char         host_name[256] = {};
    int          sock = {};
    sockaddr_in  sock_addr = {};

    //# print host name
    ret = gethostname(host_name, sizeof(host_name));
    if (ret != 0)
    {
        print("[ERROR] gethostname() fail");
        goto l_error_done;
    }
    print("Host <", host_name, "> start");

    //# create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        print("[ERROR] socket() fail");
        goto l_error_done;
    }

    //# bind
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    sock_addr.sin_port = htons(socket_port);
    ret = bind(sock, (sockaddr*)&sock_addr, sizeof(sockaddr));
    if (ret != 0)
    {
        print("[ERROR] bind() fail");
        goto l_error_done;
    }

    //# start listen
    ret = listen(sock, waiting_connect_max);
    if (ret != 0)
    {
        print("[ERROR] listen() fail");
        goto l_error_done;
    }

    while (1)
    {
        int *client_sock = new int();
        sockaddr_in *client_addr = new sockaddr_in();
        socklen_t *client_addr_len = new socklen_t(sizeof(sockaddr));

        //# accept
        print("waiting connection ...");
        *client_sock = accept(sock, (sockaddr*)client_addr, client_addr_len);
        if (*client_sock == -1)
        {
            print("[ERROR] accept() fail");
            goto l_error_done;
        }

        //# post thread service
        if (__sync_fetch_and_add(&connect_cnt, 1) <= service_connect_max)
        {
            thread thrd = thread(ClientServiceProcess, client_sock, client_addr, client_addr_len);
            thrd.detach();
        }
        else
        {
            __sync_fetch_and_sub(&connect_cnt, 1);
            print("[WARNING] servicing client is too many now, discard client <", inet_ntoa(client_addr->sin_addr), ">");
            delete client_addr_len;
            delete client_addr;
            delete client_sock;
        }
    }

l_error_done:
    print("[ERROR] something error happens, now exit program...");

    ret = close(sock);
    if (ret != 0)
    {
        print("[ERROR] closesocket() fail");
    }

    system("pause");
    return -1;
}
