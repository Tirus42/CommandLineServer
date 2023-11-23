#pragma once

#include <Client.h>

#include <string>
#include <future>
#include <memory>

class ICommandLineServer;

class CommandLineConnection {
    private:
        static const size_t LINE_SIZE_LIMIT = 4096;

        mutable std::unique_ptr<Client> stream;
        const ICommandLineServer& server;

        std::string lineBuffer;
        std::future<bool> activeAsyncCommand;

        bool processLine();
        bool processCommand(const std::string& cmd);

        void printPrompt();

    public:
        CommandLineConnection(std::unique_ptr<Client>&& stream, const ICommandLineServer& server);

        bool update();

        bool connected() const;

        void close();
};
