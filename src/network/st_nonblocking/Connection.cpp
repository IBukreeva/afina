#include "Connection.h"

#include <iostream>
#include <cassert>
#include <sys/uio.h>

namespace Afina {
namespace Network {
namespace STnonblock {

#define MAX_OUT_SIZE 100

// See Connection.h
void Connection::Start() {
    _logger->debug("Connection on {} socket started", _socket);
    _event.data.fd = _socket;
    _event.data.ptr = this;    
    _event.events = EPOLLIN | EPOLLERR | EPOLLHUP;
}

// See Connection.h
void Connection::OnError() {
    _logger->warn("Connection on {} socket has error", _socket);
    _is_alive = false;
}

// See Connection.h
void Connection::OnClose() {
    _logger->debug("Connection on {} socket closed", _socket);
    _is_alive = false;
}

// See Connection.h
void Connection::DoRead() {
    _logger->debug("Do read on {} socket", _socket);
    
    /*/  //seems like it's impossible
    if(!_is_alive){ 
        return;
    }
    /*/

    try {
        int read_now_count = -1;
        while ((read_now_count = read(_socket, _read_buffer + _read_bytes, sizeof(_read_buffer) - _read_bytes)) > 0) {

            _logger->debug("Got {} bytes from socket", read_now_count);
            _read_bytes += read_now_count;

            while (_read_bytes > 0) {
                _logger->debug("Process {} bytes", _read_bytes);
                // There is no command yet
                if (!_command_to_execute) {
                    std::size_t parsed = 0;
                    if (_parser.Parse(_read_buffer, _read_bytes, parsed)) {
                        // There is no command to be launched, continue to parse input stream
                        // Here we are, current chunk finished some command, process it
                        _logger->debug("Found new command: {} in {} bytes", _parser.Name(), parsed);
                        _command_to_execute = _parser.Build(_arg_remains);
                        if (_arg_remains > 0) {
                            _arg_remains += 2;
                        }
                    }

                    // Parsed might fails to consume any bytes from input stream. In real life that could happens,
                    // for example, because we are working with UTF-16 chars and only 1 byte left in stream
                    if (parsed == 0) {
                        break;
                    } else {
                        std::memmove(_read_buffer, _read_buffer + parsed, _read_bytes - parsed);
                        _read_bytes -= parsed;
                    }
                }

                // There is command, but we still wait for argument to arrive...
                if (_command_to_execute && _arg_remains > 0) {
                    _logger->debug("Fill argument: {} bytes of {}", _read_bytes, _arg_remains);
                    // There is some parsed command, and now we are reading argument
                    std::size_t to_read = std::min(_arg_remains, std::size_t(_read_bytes));
                    _argument_for_command.append(_read_buffer, to_read);

                    std::memmove(_read_buffer, _read_buffer + to_read, _read_bytes - to_read);
                    _arg_remains -= to_read;
                    _read_bytes -= to_read;
                }

                // Thre is command & argument - RUN!
                if (_command_to_execute && _arg_remains == 0) {
                    _logger->debug("Start command execution");

                    std::string result;
                    
                    if (_argument_for_command.size()) {
                        _argument_for_command.resize(_argument_for_command.size() - 2);
                    }
                    _command_to_execute->Execute(*_pStorage, _argument_for_command, result);

                    // Send response
                    result += "\r\n";
                    _output.push_back(result);
                    
                    if(! _output.empty()){ // if queue isn't empty we add EPOLLOUT interest
                        _event.events |= EPOLLOUT;
                    }

                    if( _output.size() >= MAX_OUT_SIZE){ // if queue has more than 100 elements, drop EPOLLIN interest
                        _event.events = EPOLLOUT | EPOLLHUP | EPOLLERR;
                    }

                    // Prepare for the next command
                    _command_to_execute.reset();
                    _argument_for_command.resize(0);
                    _parser.Reset();
                }
            } // while (_read_bytes)
        } // while (read_now_count)
        
        if (_read_bytes == 0) {
            _is_reading_ended = true; 
            _logger->debug("Connection closed");
        } else {
            throw std::runtime_error(std::string(strerror(errno)));
        }
    } catch (std::runtime_error &ex) {
        _logger->error("Failed to process connection on descriptor {}: {}", _socket, ex.what());
        _is_reading_ended = true;
    }



}

// See Connection.h
void Connection::DoWrite() {
    _logger->debug("Do write on {} socket, queue_size: {}", _socket, _output.size());
    assert(! _output.empty());

    struct iovec iov[_output.size()];
    std::size_t i=0;
    
    for ( i = 0; i < _output.size(); i++) { // init iovec
        iov[i].iov_base = &(_output[i][0]);
        iov[i].iov_len = _output[i].size();
    }
    iov[0].iov_base = static_cast<char*>(iov[0].iov_base) + _head_offset; //displace the 0th element on offset
    iov[0].iov_len -= _head_offset; //and decrease its length also

    int written_bytes = writev(_socket, iov, i);

    if (written_bytes <= 0) { //an error occured
        if (errno != EINTR && errno != EAGAIN && errno != EPIPE) {
            _is_alive = false;
            throw std::runtime_error("Impossible to send response");
        }
    }

    i = 0;
    for ( i = 0; i <_output.size(); i++) {
        if (written_bytes - _output[i].size() >= 0) {
            written_bytes -=  _output[i].size();
        } else {
            break;
        }
    }
    _output.erase(_output.begin(), _output.begin() + i);
    _head_offset = written_bytes;

    if (_output.size() < MAX_OUT_SIZE){
        _event.events |= EPOLLIN;
    }

    if(_output.empty()){
        _event.events = EPOLLIN | EPOLLHUP | EPOLLERR;
        if( _is_reading_ended){
            _is_alive = false;
        }
    }
    _logger->debug("DoWrite complete, queue_size: {}", _output.size());
}

} // namespace STnonblock
} // namespace Network
} // namespace Afina
