#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <memory>
#include <queue>

#include <boost/bind.hpp>
#include <boost/asio.hpp>

#include "Protocol.pb.h"
#include "Session.h"
#include "PacketHandler.h"

#ifdef _DEBUG
#pragma comment(lib, "Protobuf\\Debug\\libprotobufd.lib")
#else
#pragma comment(lib, "Protobuf\\Release\\libprotobuf.lib")
#endif
