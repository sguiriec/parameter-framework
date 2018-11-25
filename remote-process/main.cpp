/*
 * Copyright (c) 2011-2014, Intel Corporation
 * Copyright (c) 2018, Renault s.a.s
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <asio.hpp>

#include <iostream>
#include <string>
#include <cstring>
#include <stdlib.h>
#include "RequestMessage.h"
#include "AnswerMessage.h"
#include "Socket.h"
#include "convert.hpp"

using namespace std;

bool sendAndDisplayCommand(asio::generic::stream_protocol::socket &socket,
                           CRequestMessage &requestMessage)
{
    string strError;

    if (requestMessage.serialize(Socket(socket), true, strError) != CRequestMessage::success) {

        cerr << "Unable to send command to target: " << strError << endl;
        return false;
    }

    ///// Get answer
    CAnswerMessage answerMessage;
    if (answerMessage.serialize(Socket(socket), false, strError) != CRequestMessage::success) {

        cerr << "Unable to received answer from target: " << strError << endl;
        return false;
    }

    // Success?
    if (!answerMessage.success()) {

        // Display error answer
        cerr << answerMessage.getAnswer() << endl;
        return false;
    }

    // Display success answer
    cout << answerMessage.getAnswer() << endl;

    return true;
}

// <hostname port|path> command [argument[s]]
// or
// <hostname port|path> < commands
int main(int argc, char *argv[])
{
    int commandPos;

    // Enough args?
    if (argc < 3) {

        cerr << "Missing arguments" << endl;
        cerr << "Usage: " << endl;
        cerr << "Send a single command:" << endl;
        cerr << "\t" << argv[0] << " <hostname port|path> <command> [argument[s]]" << endl;

        return 1;
    }
    asio::io_service io_service;
    asio::generic::stream_protocol::socket connectionSocket(io_service);

    bool isInet = false;
    try {
        string host{argv[1]};
        string port{argv[2]};
        uint16_t testConverter;

        isInet = convertTo(port, testConverter);
        if (isInet) {
            asio::ip::tcp::resolver resolver(io_service);
            asio::ip::tcp::socket tcpSocket(io_service);

            asio::connect(tcpSocket, resolver.resolve(asio::ip::tcp::resolver::query(host, port)));
            connectionSocket = std::move(tcpSocket);

            commandPos = 3;
        } else {
            string path{argv[1]};

            asio::generic::stream_protocol::socket socket(io_service);
            asio::generic::stream_protocol::endpoint endpoint =
                asio::local::stream_protocol::endpoint(path);
            socket.connect(endpoint);
            connectionSocket = std::move(socket);

            commandPos = 2;
        }

    } catch (const asio::system_error &e) {
        string endpoint;

        if (isInet) {
            endpoint = string("tcp://") + argv[1] + ":" + argv[2];
        } else { /* unix */
            endpoint = string("unix://") + argv[1];
        }
        cerr << "Connection to '" << endpoint << "' failed: " << e.what() << endl;
        return 1;
    }

    // Create command message
    CRequestMessage requestMessage(argv[commandPos]);

    // Add arguments
    for (int arg = commandPos + 1; arg < argc; arg++) {

        requestMessage.addArgument(argv[arg]);
    }

    if (!sendAndDisplayCommand(connectionSocket, requestMessage)) {
        return 1;
    }

    // Program status
    return 0;
}
