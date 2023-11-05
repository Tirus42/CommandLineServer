#pragma once

#include <WiFi.h>
#include <WiFiServer.h>

#include <vector>
#include <map>
#include <memory>

class CommandLineConnection;

class ICommandLineHandler {
    public:
        virtual bool execute(Print& output, const std::vector<std::string>& params) = 0;

        virtual bool execute(Print& output, const std::vector<std::string>& params, CommandLineConnection& /*connection*/) {
            return execute(output, params);
        }

        virtual bool executeAsync() const {
            return false;
        }
};

class CommandLineTCPServer {
    public:
        enum class DataType {
            u8
        };

    private:
        struct Parameter {
            std::string name;
            DataType dataType;
        };

        struct TCommand {
            std::vector<Parameter> parameters;
            std::unique_ptr<ICommandLineHandler> handler;

            TCommand() = default;
            TCommand(ICommandLineHandler* handler);
            TCommand(const TCommand&) = delete;
            TCommand& operator=(const TCommand&) = delete;
            TCommand(TCommand&&) = default;
        };

        WiFiServer tcpServer;

        std::map<std::string, TCommand*> commands;
        std::vector<std::unique_ptr<CommandLineConnection>> clients;
        std::string welcomeLine;

        void acceptNewClients();
        void updateClients();
        bool updateClient(CommandLineConnection& connection) const;

    public:
        CommandLineTCPServer(uint16_t port, const char* welcomeLine = nullptr);
        ~CommandLineTCPServer();

        bool update();

        void registerCommand(const char* name, ICommandLineHandler* commandHandler);

        const std::map<std::string, TCommand*>& getCommandList() const {
            return commands;
        }

        ICommandLineHandler* getCommandHandlerByName(const std::string& name) const;

        const std::string& getWelcomeLine() const;

        /**
        * Adds a custom client to the active connection list.
        * Will accept commands from them until connected() returns false.
        */
        void addCustomClient(std::unique_ptr<Client>&& newClient);
};
