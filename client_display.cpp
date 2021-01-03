
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <io.h>
#include <fcntl.h>
#include <stdio.h>

#include <WinSock2.h>
#include "data_structure.hpp"
#pragma comment(lib, "ws2_32.lib")

DataTime GetDataTime()
{
    DataTime time = {};
    SYSTEMTIME systime;
    GetLocalTime(&systime);
    time.year = systime.wYear;
    time.month = systime.wMonth;
    time.day = systime.wDay;
    time.hour = systime.wHour;
    time.minute = systime.wMinute;
    time.second = systime.wSecond;
    time.millisecond = systime.wMilliseconds;
    return time;
}

bool OnceRead(SOCKET sk, char *buf, int expect_len)
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
        Sleep(10);
    }
    return read_len == expect_len;
}

Data *GetData(SOCKET sk)
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

bool SendData(SOCKET sk, Data *data)
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

void GlobalChatInfoWrite(ChatInformation &info)
{
    GlobalChatInfoMutex().lock();
    GlobalChatInfo() = info;
    GlobalChatInfoMutex().unlock();
}

void cls()
{
    HANDLE hStdout;

    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

    // Fetch existing console mode so we correctly add a flag and not turn off others
    DWORD mode = 0;
    if (!GetConsoleMode(hStdout, &mode))
    {
        return;
    }

    // Hold original mode to restore on exit to be cooperative with other command-line apps.
    const DWORD originalMode = mode;
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

    // Try to set the mode.
    if (!SetConsoleMode(hStdout, mode))
    {
        return;
    }

    // Write the sequence for clearing the display.
    DWORD written = 0;
    PCWSTR sequence = L"\x1b[2J";
    if (!WriteConsoleW(hStdout, sequence, 5, &written, NULL))
    {
        // If we fail, try to restore the mode on the way out.
        SetConsoleMode(hStdout, originalMode);
        return;
    }

    // To also clear the scroll back, emit L"\x1b[3J" as well.
    // 2J only clears the visible window and 3J only clears the scroll back.

    // Restore the mode on the way out to be nice to other command-line applications.
    SetConsoleMode(hStdout, originalMode);
}

int main(int argc, char *argv[])
{
    int ret;

    _setmode(_fileno(stdout), _O_U16TEXT);

    char          host_name[256] = {};
    WORD          required_version = {};
    WSADATA       wsa_data = {};
    SOCKET        sock = {};
    SOCKADDR_IN   sock_addr = {};
    char          recv_buf[1024] = {};
    int           recv_len = 0;
    Data         *dt = nullptr;
    thread       *thrd = nullptr;
    DataTime      last_update_time = {};
    Data_ClientMessage *msg_data;

    //# load windows socket
    required_version = MAKEWORD(2, 2);
    ret = WSAStartup(required_version, &wsa_data);
    if (ret != 0)
    {
        print(L"[ERROR] WSAStartup() fail");
        goto l_error_done;
    }

    //# print host name
    ret = gethostname(host_name, sizeof(host_name));
    if (ret != 0)
    {
        print(L"[ERROR] gethostname() fail");
        goto l_error_done;
    }
    print(L"Host <", to_wstring(host_name), L"> start");

    //# create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
    {
        print(L"[ERROR] socket() fail");
        goto l_error_done;
    }

    //# connect
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = inet_addr(server_addr);
    sock_addr.sin_port = htons(socket_port);
    ret = connect(sock, (SOCKADDR*)&sock_addr, sizeof(SOCKADDR));
    if (ret != 0)
    {
        print(L"[ERROR] connect() fail");
        goto l_error_done;
    }

    msg_data = new Data_ClientMessage(GetDataTime(), { GetDataTime(), wstring(L"DISPLAY"), wstring(L"D") });
    if (!SendData(sock, msg_data))
    {
        goto l_error_done;
    }
    delete msg_data;

    while (1)
    {
        Data *d = GetData(sock);
        ChatInformation statistics;
        if (d)
        {
            switch (d->MyType())
            {
            case ServerStatistics:
                statistics = ((Data_ServerStatistics*)d)->Info();
                {
                    auto &vec_msg = statistics.vCM;
                    int update_idx = vec_msg.size();
                    while (update_idx - 1 >= 0)
                    {
                        if (vec_msg[update_idx - 1].time > last_update_time)
                        {
                            --update_idx;
                        }
                        else
                        {
                            break;
                        }
                    }
                    while (update_idx < vec_msg.size())
                    {
                        auto &cm = vec_msg[update_idx];
                        wstring ws = L"";
                        ws += to_wstring(cm.time.year) + L".";
                        ws += cm.time.month < 10 ? L"0" : L"";
                        ws += to_wstring(cm.time.month) + L".";
                        ws += cm.time.day < 10 ? L"0" : L"";
                        ws += to_wstring(cm.time.day) + L" ";

                        ws += cm.time.hour < 10 ? L"0" : L"";
                        ws += to_wstring(cm.time.hour) + L":";
                        ws += cm.time.minute < 10 ? L"0" : L"";
                        ws += to_wstring(cm.time.minute) + L":";
                        ws += cm.time.second < 10 ? L"0" : L"";
                        ws += to_wstring(cm.time.second) + L" ";
                        ws += L"<" + cm.name;
                        //+ L">";
                        while (ws.size() && ws.back() == 0)
                        {
                            ws.pop_back();
                        }
                        ws += L">";
                        ws += cm.msg;
                        while (ws.size() && ws.back() == 0)
                        {
                            ws.pop_back();
                        }
                        print(ws);
                        last_update_time = cm.time;
                        ++update_idx;
                    }
                }
                break;
            default:
                print(L"[ERROR] Receive unexpected data");
                goto l_error_done;
            }
            delete d;
        }
        else
        {
            break;
        }
    }

    goto l_done;

l_error_done:
    print(L"[ERROR] something error happens, now exit program...");

l_done:
    delete thrd;

    ret = closesocket(sock);
    if (ret != 0)
    {
        print(L"[ERROR]  closesocket() fail");
    }

    system("pause");
    return ret;
}
