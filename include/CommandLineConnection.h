#pragma once

#include <WiFiServer.h>
#include <string>
#include <future>

class CommandLineTCPServer;

class CommandLineConnection {
    private:
        static const size_t LINE_SIZE_LIMIT = 4096;

        mutable WiFiClient stream;
        const CommandLineTCPServer& server;

        std::string lineBuffer;
        std::future<bool> activeAsyncCommand;

        bool processLine();
        bool processCommand(const std::string& cmd);

        void printPrompt();

    public:
        CommandLineConnection(WiFiClient stream, const CommandLineTCPServer& server);

        bool update();

        bool connected() const;

        void close();
};
