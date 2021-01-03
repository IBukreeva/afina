#ifndef AFINA_NETWORK_MT_NONBLOCKING_CONNECTION_H
#define AFINA_NETWORK_MT_NONBLOCKING_CONNECTION_H

#include <cstring>

#include <sys/epoll.h>

#include <afina/execute/Command.h>
#include <protocol/Parser.h>
#include <spdlog/logger.h>

#include <vector>
#include <atomic>

namespace Afina {
namespace Network {
namespace MTnonblock {

class Connection {
public:
    Connection(int s, std::shared_ptr<Afina::Storage>& ps, std::shared_ptr<spdlog::logger>& pl)
     : _socket(s),  _pStorage(ps), _logger(pl) {
        std::memset(&_event, 0, sizeof(struct epoll_event));
        _is_alive.store(true, std::memory_order_release);
        _is_reading_ended.store(false, std::memory_order_release);
        _read_bytes = 0;
        _head_offset = 0;
        std::memset(_read_buffer, 0, 4096);
        _arg_remains = 0;
        _event.data.ptr = this;
    }

    inline bool isAlive() const { return _is_alive.load(std::memory_order_acquire); }

    void Start();

protected:
    void OnError();
    void OnClose();
    void DoRead();
    void DoWrite();

private:
    friend class Worker;
    friend class ServerImpl;

    int _socket;
    struct epoll_event _event;

    std::atomic<bool> _is_alive;
    std::atomic<bool> _is_reading_ended;

    std::vector<std::string> _output;

    std::shared_ptr<spdlog::logger> _logger;
    std::shared_ptr<Afina::Storage> _pStorage;

    //for DoWrite()
    std::size_t _head_offset;
    
    //for DoRead()
    char _read_buffer[4096];
    std::size_t _read_bytes;
    std::size_t _arg_remains;
    Protocol::Parser _parser;
    std::string _argument_for_command;
    std::unique_ptr<Execute::Command> _command_to_execute;
};

} // namespace MTnonblock
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_MT_NONBLOCKING_CONNECTION_H
