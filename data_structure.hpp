
#pragma once

#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <vector>
#include <queue>
#include <algorithm>
#include <codecvt>
#include <locale>

using namespace std;

const unsigned int message_header1 = 0xfa1f37a5;
const unsigned int message_header2 = 0x906b33e2;
const char *server_addr = "152.136.215.72";
const int socket_port = 19119;

using convert_t = std::codecvt_utf8<wchar_t>;
std::wstring_convert<convert_t, wchar_t> strconverter;

std::string to_string(std::wstring wstr)
{
    return strconverter.to_bytes(wstr);
}

std::wstring to_wstring(std::string str)
{
    return strconverter.from_bytes(str);
}

template<typename Ty>
void _print(Ty a)
{
    wcout << a;
}

template<typename Ty, typename ...Args>
void _print(Ty a, Args...args)
{
    wcout << a;
    _print(args...);
}

template<typename ...Args>
void print(Args...args)
{
    static mutex print_mutex = {};
    print_mutex.lock();
    _print(args...);
    wcout << endl;
    print_mutex.unlock();
}

void print(const wstring &ws)
{
    wcout << ws << endl;
}

struct Memory
{
    int len;
    char *mem;
};

enum DataType
{
    ServerStatistics,
    ServerDismissClient,

    ClientMessage,
};

struct DataTime
{
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int millisecond;
};

bool operator ==(DataTime dt1, DataTime dt2)
{
    if (dt1.year == dt2.year &&
        dt1.month == dt2.month &&
        dt1.day == dt2.day &&
        dt1.hour == dt2.hour &&
        dt1.minute == dt2.minute &&
        dt1.second == dt2.second &&
        dt1.millisecond == dt2.millisecond)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool operator <(DataTime dt1, DataTime dt2)
{
    if (dt1.year < dt2.year)
    {
        return true;
    }
    else if (dt1.year > dt2.year)
    {
        return false;
    }
    if (dt1.month < dt2.month)
    {
        return true;
    }
    else if (dt1.month > dt2.month)
    {
        return false;
    }
    if (dt1.day < dt2.day)
    {
        return true;
    }
    else if (dt1.day > dt2.day)
    {
        return false;
    }
    if (dt1.hour < dt2.hour)
    {
        return true;
    }
    else if (dt1.hour > dt2.hour)
    {
        return false;
    }
    if (dt1.minute < dt2.minute)
    {
        return true;
    }
    else if (dt1.minute > dt2.minute)
    {
        return false;
    }
    if (dt1.second < dt2.second)
    {
        return true;
    }
    else if (dt1.second > dt2.second)
    {
        return false;
    }
    if (dt1.millisecond < dt2.millisecond)
    {
        return true;
    }
    else if (dt1.millisecond > dt2.millisecond)
    {
        return false;
    }
    return false;
}

bool operator >(DataTime dt1, DataTime dt2)
{
    return !(dt1 < dt2) && !(dt1 == dt2);
}

bool operator <=(DataTime dt1, DataTime dt2)
{
    return !(dt1 > dt2);
}

bool operator >=(DataTime dt1, DataTime dt2)
{
    return !(dt1 < dt2);
}

struct ChatMessage
{
    DataTime time;
    wstring  name;
    wstring  msg;
};

struct ChatInformation
{
    vector<wstring> member_list;
    vector<ChatMessage> vCM;
};

Memory *GenerateMemory(const DataTime &time)
{
    Memory *m = new Memory();
    int len = 28 + 4;
    m->len = len;
    m->mem = new char[len];
    memcpy_s(m->mem, sizeof(int), &len, sizeof(int));
    memcpy_s(m->mem + 1 * 4, sizeof(int), &time.year,        sizeof(int));
    memcpy_s(m->mem + 2 * 4, sizeof(int), &time.month,       sizeof(int));
    memcpy_s(m->mem + 3 * 4, sizeof(int), &time.day,         sizeof(int));
    memcpy_s(m->mem + 4 * 4, sizeof(int), &time.hour,        sizeof(int));
    memcpy_s(m->mem + 5 * 4, sizeof(int), &time.minute,      sizeof(int));
    memcpy_s(m->mem + 6 * 4, sizeof(int), &time.second,      sizeof(int));
    memcpy_s(m->mem + 7 * 4, sizeof(int), &time.millisecond, sizeof(int));
    return m;
}

DataTime MemoryToDataTime(Memory mem)
{
    DataTime dt;
    memcpy_s(&dt.year,        sizeof(int), mem.mem + 1 * 4, sizeof(int));
    memcpy_s(&dt.month,       sizeof(int), mem.mem + 2 * 4, sizeof(int));
    memcpy_s(&dt.day,         sizeof(int), mem.mem + 3 * 4, sizeof(int));
    memcpy_s(&dt.hour,        sizeof(int), mem.mem + 4 * 4, sizeof(int));
    memcpy_s(&dt.minute,      sizeof(int), mem.mem + 5 * 4, sizeof(int));
    memcpy_s(&dt.second,      sizeof(int), mem.mem + 6 * 4, sizeof(int));
    memcpy_s(&dt.millisecond, sizeof(int), mem.mem + 7 * 4, sizeof(int));
    return dt;
}

Memory *GenerateMemory(const wstring &ws)
{
    Memory *m = new Memory();
    int len = ws.size() * 2 + sizeof(int);
    m->len = len;
    m->mem = new char[len];
    memcpy_s(m->mem, sizeof(int), &len, sizeof(int));
    memcpy_s(m->mem + sizeof(int), len - sizeof(int), ws.data(), len - sizeof(int));
    return m;
}

wstring MemoryToWString(Memory mem)
{
    wstring str;
    int len = mem.len - 4;
    int sz = len / 2;
    str.resize(sz + 1, 0);
    memcpy_s((void*)str.data(), len, mem.mem + 4, len);
    return str;
}

Memory *GenerateMemory(const ChatMessage &cm)
{
    Memory *time_mem = GenerateMemory(cm.time);
    Memory *name_mem = GenerateMemory(cm.name);
    Memory *msg_mem = GenerateMemory(cm.msg);

    Memory *m = new Memory();
    int len = 4 + time_mem->len + name_mem->len + msg_mem->len;
    m->len = len;
    m->mem = new char[len];
    memcpy_s(m->mem, sizeof(int), &len, sizeof(int));

    int ofst = 4;
    memcpy_s(m->mem + ofst, time_mem->len, time_mem->mem, time_mem->len);
    ofst += time_mem->len;
    memcpy_s(m->mem + ofst, name_mem->len, name_mem->mem, name_mem->len);
    ofst += name_mem->len;
    memcpy_s(m->mem + ofst, msg_mem->len, msg_mem->mem, msg_mem->len);
    ofst += msg_mem->len;

    delete []time_mem->mem;
    delete time_mem;

    delete []name_mem->mem;
    delete name_mem;

    delete []msg_mem->mem;
    delete msg_mem;

    return m;
}

ChatMessage MemoryToChatMessage(Memory mem)
{
    int total = mem.len;
    int ofst = 4;
    Memory time_mem;
    Memory name_mem;
    Memory msg_mem;

    int time_len = *(int*)(mem.mem + ofst);
    time_mem.len = time_len;
    time_mem.mem = mem.mem + ofst;
    ofst += time_len;

    int name_len = *(int*)(mem.mem + ofst);
    name_mem.len = name_len;
    name_mem.mem = mem.mem + ofst;
    ofst += name_len;

    int msg_len = *(int*)(mem.mem + ofst);
    msg_mem.len = msg_len;
    msg_mem.mem = mem.mem + ofst;
    ofst += msg_len;

    ChatMessage cm;
    cm.time = MemoryToDataTime(time_mem);
    cm.name = MemoryToWString(name_mem);
    cm.msg = MemoryToWString(msg_mem);
    return cm;
}

Memory *GenerateMemory(const ChatInformation &ci)
{
    vector<Memory*> vM_member;
    vector<Memory*> vM_chat_msg;

    int len = 0;
    int len_member = 0;
    int len_msg = 0;
    int ofst = 0;

    for (auto &s : ci.member_list)
    {
        Memory *m = ::GenerateMemory(s);
        len_member += m->len;
        vM_member.push_back(m);
    }
    for (auto &mm : ci.vCM)
    {
        Memory *m = ::GenerateMemory(mm);
        len_msg += m->len;
        vM_chat_msg.push_back(m);
    }

    len = sizeof(int) * 3 + len_member + len_msg;
    Memory *m = new Memory();
    m->len = len;
    m->mem = new char[len];

    memcpy_s(m->mem + ofst, sizeof(int), &len, sizeof(int));
    ofst += sizeof(int);
    memcpy_s(m->mem + ofst, sizeof(int), &len_member, sizeof(int));
    ofst += sizeof(int);
    memcpy_s(m->mem + ofst, sizeof(int), &len_msg, sizeof(int));
    ofst += sizeof(int);

    for (auto *mm : vM_member)
    {
        memcpy_s(m->mem + ofst, mm->len, mm->mem, mm->len);
        ofst += mm->len;

        delete []mm->mem;
        delete mm;
    }
    for (auto *mm : vM_chat_msg)
    {
        memcpy_s(m->mem + ofst, mm->len, mm->mem, mm->len);
        ofst += mm->len;

        delete []mm->mem;
        delete mm;
    }
    return m;
}

ChatInformation MemoryToChatInformation(Memory mem)
{
    ChatInformation ci;
    int len = 0;
    int len_member = 0;
    int len_msg = 0;
    int ofst = 0;

    memcpy_s(&len, sizeof(int), mem.mem + ofst, sizeof(int));
    ofst += sizeof(int);
    memcpy_s(&len_member, sizeof(int), mem.mem + ofst, sizeof(int));
    ofst += sizeof(int);
    memcpy_s(&len_msg, sizeof(int), mem.mem + ofst, sizeof(int));
    ofst += sizeof(int);

    int start_of_member = sizeof(int) * 3;
    int start_of_msg = start_of_member + len_member;

    while (ofst < start_of_msg)
    {
        Memory tmp = {};
        memcpy_s(&tmp.len, sizeof(int), mem.mem + ofst, sizeof(int));
        tmp.mem = mem.mem + ofst;
        wstring str = MemoryToWString(tmp);
        ofst += tmp.len;

        ci.member_list.push_back(str);
    }

    while (ofst < len)
    {
        Memory tmp = {};
        memcpy_s(&tmp.len, sizeof(int), mem.mem + ofst, sizeof(int));
        tmp.mem = mem.mem + ofst;
        ChatMessage cm = MemoryToChatMessage(tmp);
        ofst += tmp.len;

        ci.vCM.push_back(cm);
    }

    return ci;
}

class Data
{
public:
    Data() = delete;
    Data(DataType ty, DataTime tm) : type(ty), time(tm) {}
    virtual ~Data() {}

    virtual Memory *GenerateMemory() { return nullptr; }

    DataType MyType() { return type; }
    DataTime MyTime() { return time; }

private:
    DataType type;
    DataTime time;
};

class Data_ServerStatistics : public Data
{
public:
    Data_ServerStatistics() = delete;

    Data_ServerStatistics(DataTime tm) :
        Data(ServerStatistics, tm)
    {}

    Data_ServerStatistics(DataTime tm, ChatInformation ci) :
        Data(ServerStatistics, tm),
        info(ci)
    {}

    ~Data_ServerStatistics() override
    {}

    ChatInformation &Info() { return info; }

    Memory *GenerateMemory() override
    {
        Memory *m = new Memory();
        DataType ty = MyType();
        Memory *time_mem = ::GenerateMemory(MyTime());
        Memory *info_mem = ::GenerateMemory(info);

        int len = sizeof(int) + sizeof(DataType) + time_mem->len + info_mem->len;
        m->len = len;
        m->mem = new char[len];

        int ofst = 0;
        memcpy_s(m->mem + ofst, sizeof(int), &len, sizeof(int));
        ofst += sizeof(int);

        memcpy_s(m->mem + ofst, sizeof(ty), &ty, sizeof(ty));
        ofst += sizeof(ty);

        memcpy_s(m->mem + ofst, time_mem->len, time_mem->mem, time_mem->len);
        ofst += time_mem->len;

        memcpy_s(m->mem + ofst, info_mem->len, info_mem->mem, info_mem->len);
        ofst += info_mem->len;

        delete []time_mem->mem;
        delete time_mem;

        delete []info_mem->mem;
        delete info_mem;

        return m;
    }

    static Data *MemoryToMe(Memory mem)
    {
        int len = 0;
        int ofst = 0;
        DataType dt = {};
        DataTime time = {};
        ChatInformation ci = {};

        memcpy_s(&len, sizeof(int), mem.mem + ofst, sizeof(int));
        ofst += sizeof(int);
        memcpy_s(&dt, sizeof(DataType), mem.mem + ofst, sizeof(DataType));
        ofst += sizeof(DataType);

        Memory time_mem = {};
        memcpy_s(&time_mem.len, sizeof(int), mem.mem + ofst, sizeof(int));
        time_mem.mem = mem.mem + ofst;
        time = MemoryToDataTime(time_mem);
        ofst += time_mem.len;

        Memory ci_mem = {};
        memcpy_s(&ci_mem.len, sizeof(int), mem.mem + ofst, sizeof(int));
        ci_mem.mem = mem.mem + ofst;
        ci = MemoryToChatInformation(ci_mem);
        ofst += ci_mem.len;

        return new Data_ServerStatistics(time, ci);
    }

private:
    ChatInformation info;
};

class Data_ServerDismissClient : public Data
{
public:
    Data_ServerDismissClient() = delete;

    Data_ServerDismissClient(DataTime tm) :
        Data(ServerDismissClient, tm)
    {}

    ~Data_ServerDismissClient() override
    {}

    Memory *GenerateMemory() override
    {
        Memory *m = new Memory();
        DataType ty = MyType();
        Memory *time_mem = ::GenerateMemory(MyTime());

        int len = sizeof(int) + sizeof(DataType) + time_mem->len;
        m->len = len;
        m->mem = new char[len];

        int ofst = 0;
        memcpy_s(m->mem + ofst, sizeof(int), &len, sizeof(int));
        ofst += sizeof(int);

        memcpy_s(m->mem + ofst, sizeof(ty), &ty, sizeof(ty));
        ofst += sizeof(ty);

        memcpy_s(m->mem + ofst, time_mem->len, time_mem->mem, time_mem->len);
        ofst += time_mem->len;

        delete []time_mem->mem;
        delete time_mem;
        return m;
    }

    static Data *MemoryToMe(Memory mem)
    {
        int len = 0;
        int ofst = 0;
        DataType dt = {};
        DataTime time = {};

        memcpy_s(&len, sizeof(int), mem.mem + ofst, sizeof(int));
        ofst += sizeof(int);
        memcpy_s(&dt, sizeof(DataType), mem.mem + ofst, sizeof(DataType));
        ofst += sizeof(DataType);

        Memory time_mem = {};
        memcpy_s(&time_mem.len, sizeof(time_mem.len), mem.mem + ofst, sizeof(time_mem.len));
        time_mem.mem = mem.mem + ofst;
        time = MemoryToDataTime(time_mem);
        ofst += time_mem.len;

        return new Data_ServerDismissClient(time);
    }

};

class Data_ClientMessage : public Data
{
public:
    Data_ClientMessage() = delete;

    Data_ClientMessage(DataTime tm) :
        Data(ClientMessage, tm)
    {}

    Data_ClientMessage(DataTime tm, ChatMessage cm) :
        Data(ClientMessage, tm),
        msg(cm)
    {}

    ~Data_ClientMessage() override
    {}

    ChatMessage GetMsg() { return msg; }

    Memory *GenerateMemory() override
    {
        Memory *m = new Memory();
        DataType ty = MyType();
        Memory *time_mem = ::GenerateMemory(MyTime());
        Memory *msg_mem = ::GenerateMemory(msg);

        int len = sizeof(int) + sizeof(DataType) + time_mem->len + msg_mem->len;
        m->len = len;
        m->mem = new char[len];

        int ofst = 0;
        memcpy_s(m->mem + ofst, sizeof(int), &len, sizeof(int));
        ofst += sizeof(int);

        memcpy_s(m->mem + ofst, sizeof(ty), &ty, sizeof(ty));
        ofst += sizeof(ty);

        memcpy_s(m->mem + ofst, time_mem->len, time_mem->mem, time_mem->len);
        ofst += time_mem->len;

        memcpy_s(m->mem + ofst, msg_mem->len, msg_mem->mem, msg_mem->len);
        ofst += msg_mem->len;

        delete []time_mem->mem;
        delete time_mem;

        delete []msg_mem->mem;
        delete msg_mem;

        return m;
    }

    static Data *MemoryToMe(Memory mem)
    {
        int len = 0;
        int ofst = 0;
        DataType dt = {};
        DataTime time = {};
        ChatMessage m = {};

        memcpy_s(&len, sizeof(len), mem.mem + ofst, sizeof(len));
        ofst += sizeof(len);
        memcpy_s(&dt, sizeof(DataType), mem.mem + ofst, sizeof(DataType));
        ofst += sizeof(DataType);

        Memory time_mem = {};
        memcpy_s(&time_mem.len, sizeof(int), mem.mem + ofst, sizeof(int));
        time_mem.mem = mem.mem + ofst;
        time = MemoryToDataTime(time_mem);
        ofst += time_mem.len;

        Memory msg_mem = {};
        memcpy_s(&msg_mem.len, sizeof(int), mem.mem + ofst, sizeof(int));
        msg_mem.mem = mem.mem + ofst;
        m = MemoryToChatMessage(msg_mem);
        ofst += msg_mem.len;

        return new Data_ClientMessage(time, m);
    }

private:
    ChatMessage msg;
};

Data *MemoryToData(Memory &mem)
{
    DataType ty = {};
    memcpy_s(&ty, sizeof(ty), mem.mem + sizeof(int), sizeof(ty));
    Data *d = nullptr;
    switch (ty)
    {
    case ServerStatistics:
        d = Data_ServerStatistics::MemoryToMe(mem);
        break;
    case ServerDismissClient:
        d = Data_ServerDismissClient::MemoryToMe(mem);
        break;
    case ClientMessage:
        d = Data_ClientMessage::MemoryToMe(mem);
        break;
    default:
        d = nullptr;
    }
    return d;
}
