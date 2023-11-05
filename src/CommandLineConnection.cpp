#include "CommandLineConnection.h"

#include "CommandLineServer.h"

using namespace std::chrono_literals;

CommandLineConnection::CommandLineConnection(std::unique_ptr<Client>&& stream_, const CommandLineTCPServer& server) :
    stream(std::move(stream_)),
    server(server),
    lineBuffer(),
    activeAsyncCommand() {

    if (!server.getWelcomeLine().empty()) {
        stream->printf("%s\n", server.getWelcomeLine().c_str());
    }

    stream->printf("You can type your commands, type 'help' for a list of commands\n");
    stream->printf("===========\n> ");
}

static char ReadChar(Stream& stream) {
    char c;
    stream.readBytes(reinterpret_cast<uint8_t*>(&c), sizeof(c));
    return c;
}

bool CommandLineConnection::update() {
    if (activeAsyncCommand.valid()) {
        auto status = activeAsyncCommand.wait_for(0ms);

        if (status == std::future_status::ready) {
            bool result = activeAsyncCommand.get();

            printPrompt();
        } else {
            return true;
        }
    }

    while (int bytesAvailable = stream->available()) {
        char c = ReadChar(*stream);

        if (c == '\n') {
            processLine();

            if (!activeAsyncCommand.valid()) {
                printPrompt();
            }

        } else if (c == '\r') {}
        else if (c == 4) {  // EOF
            stream->printf("Goodbye (EOF)\n");
            return false;
        } else {
            if (lineBuffer.size() == LINE_SIZE_LIMIT) {
                stream->printf("== Line buffer size exceeded, aborting ==\n");
                return false;
            }

            lineBuffer += c;
        }
    }

    return stream->connected();
}

bool CommandLineConnection::connected() const {
    return stream->connected();
}

void CommandLineConnection::close() {
    stream->stop();
}

static std::vector<std::string> SplitByWhitespace(const std::string& line) {
    std::vector<std::string> result;

    const char *token = strtok(const_cast<char*>(line.c_str()), (char*)" ");
    while (token != nullptr) {
        result.push_back(std::string(token));
        token = strtok(nullptr, (char*)" ");
    }

    return result;
}

static bool ThreadFunction(ICommandLineHandler* commandHandler, Print& output, std::vector<std::string>&& tokens, CommandLineConnection* connection) {
    return commandHandler->execute(output, tokens, *connection);
}

bool CommandLineConnection::processLine() {
    while (true) {
        size_t offsetSeperator = lineBuffer.find(';');

        if (offsetSeperator == std::string::npos) {
            bool result = processCommand(lineBuffer);
            lineBuffer.clear();
            return result;
        }

        std::string cmd = lineBuffer.substr(0, offsetSeperator);
        lineBuffer = lineBuffer.substr(offsetSeperator + 1);

        bool result = processCommand(cmd);

        if (!result) {
            lineBuffer.clear();
            return false;
        }
    }
}

bool CommandLineConnection::processCommand(const std::string& cmd) {
    std::vector<std::string> tokens = SplitByWhitespace(cmd);

    if (tokens.empty())
        return false;

    ICommandLineHandler* handler = server.getCommandHandlerByName(tokens[0]);

    if (handler == nullptr) {
        stream->printf("Command '%s' not found\n", tokens[0].c_str());
        return false;
    }

    tokens = std::vector<std::string>(tokens.begin() + 1, tokens.end());

    if (handler->executeAsync()) {
        activeAsyncCommand = std::async(std::launch::async, ThreadFunction, handler, std::ref(*stream), std::move(tokens), this);
        return true;
    }

    return handler->execute(*stream, tokens, *this);
}

void CommandLineConnection::printPrompt() {
    stream->printf("> ");
}
