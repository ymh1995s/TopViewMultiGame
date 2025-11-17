#pragma once
#include "pch.h"
using namespace std;

class tempPacket
{
public:
    void SetBody(const std::string& str)
    {
        body = str;
        size = static_cast<uint32_t>(body.size());
    }

    void SetBody(const char* data, size_t len/* Body Len */)
    {
        body.assign(data, len); // assign : 복사 함수 
        size = static_cast<uint32_t>(body.size());
	}

    uint32_t GetSize() const { return size; }
    const std::string& GetBody() const { return body; }

private:
    uint32_t size;
    string body;
};

