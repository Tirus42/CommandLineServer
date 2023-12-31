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

class ICommandLineServer {
    protected:
        enum class DataType {
            u8
        };

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

    public:
        /**
        * Update function, should be called on every tick of the main loop.
        * Processes open connections, accepts new clients.
        */
        virtual bool update() = 0;

        virtual void registerCommand(const char* name, ICommandLineHandler* commandHandler) = 0;

        /**
        * \returns a name -> instance map of all registered command handlers.
        */
        virtual const std::map<std::string, TCommand*>& getCommandList() const = 0;

        /**
        * \returns the searched command line handler by name, or nullptr if not found.
        */
        virtual ICommandLineHandler* getCommandHandlerByName(const std::string& name) const = 0;

        /**
        * \returns the configured welcome line, will be a empty string when not set.
        */
        virtual const std::string& getWelcomeLine() const = 0;

        /**
        * Adds a custom client to the active connection list.
        * Will accept commands from them until connected() returns false.
        */
        virtual void addCustomClient(std::unique_ptr<Client>&& newClient) = 0;
};

/**
* Basic command line server implementation.
* Handles clients and processes the commands,
* but does not provide a network server by itself.
*
* Clients must be added individually.
* E.g. the Serial stream can be added as client instance
* to use the command line server via serial port.
*/
class CommandLineServer : public ICommandLineServer {
    private:
        std::map<std::string, TCommand*> commands;
        std::vector<std::unique_ptr<CommandLineConnection>> clients;
        std::string welcomeLine;

        void updateClients();
        bool updateClient(CommandLineConnection& connection) const;

    public:
        CommandLineServer(const char* welcomeLine = nullptr);
        virtual ~CommandLineServer();

        virtual bool update() override;

        virtual void registerCommand(const char* name, ICommandLineHandler* commandHandler) override;

        virtual const std::map<std::string, TCommand*>& getCommandList() const override {
            return commands;
        }

        virtual ICommandLineHandler* getCommandHandlerByName(const std::string& name) const override;

        virtual const std::string& getWelcomeLine() const override;

        virtual void addCustomClient(std::unique_ptr<Client>&& newClient) override;
};

/**
* TCP server based version of the CommandLineServer.
* Opens a TCP server on the specified port and accepts new clients.
* The clients can connect and use the command line via a simple client (like netcat, putty or telnet)
*/
class CommandLineTCPServer : public ICommandLineServer {
    private:
        WiFiServer tcpServer;

        std::shared_ptr<ICommandLineServer> commandLineServer;

        void acceptNewClients();

    public:
        /**
        * Constructor, creates a new instance with a own internal CommandLineServer instance.
        */
        CommandLineTCPServer(uint16_t port, const char* welcomeLine = nullptr);

        /**
        * Constructor, create a new instance by using a existing CommandLineServer instance.
        * All methods of the ICommandLineServer interface will be passed to this instance.
        */
        CommandLineTCPServer(uint16_t port, std::shared_ptr<ICommandLineServer> existingServer);

        virtual ~CommandLineTCPServer();

        virtual bool update() override;

        virtual void registerCommand(const char* name, ICommandLineHandler* commandHandler) override {
            commandLineServer->registerCommand(name, commandHandler);
        }

        virtual const std::map<std::string, TCommand*>& getCommandList() const override {
            return commandLineServer->getCommandList();;
        }

        virtual ICommandLineHandler* getCommandHandlerByName(const std::string& name) const override {
            return commandLineServer->getCommandHandlerByName(name);
        }

        virtual const std::string& getWelcomeLine() const override {
            return commandLineServer->getWelcomeLine();
        }

        virtual void addCustomClient(std::unique_ptr<Client>&& newClient) override {
            commandLineServer->addCustomClient(std::move(newClient));
        }
};
