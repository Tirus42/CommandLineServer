#include "CommandLineServer.h"

#include "CommandLineConnection.h"

struct HelpCommand : public ICommandLineHandler {
    const CommandLineTCPServer& server;

    HelpCommand(const CommandLineTCPServer& server) :
        server(server) {};

    virtual bool execute(Print& output, const std::vector<std::string>& params) override {
        auto list = server.getCommandList();

        output.printf("List of commands (%u)\n", uint32_t(list.size()));

        for (const auto& entry : list) {
            output.printf("%s\n", entry.first.c_str());
        }

        return true;
    }
};

struct QuitCommand : public ICommandLineHandler {
    virtual bool execute(Print& output, const std::vector<std::string>& params) override {
        // Should never be executed.
        return false;
    }

    virtual bool execute(Print& output, const std::vector<std::string>& params, CommandLineConnection& connection) override {
        connection.close();
        return true;
    }
};

CommandLineTCPServer::TCommand::TCommand(ICommandLineHandler* handler) :
    parameters(),
    handler(std::move(std::unique_ptr<ICommandLineHandler>(handler))) {}

CommandLineTCPServer::CommandLineTCPServer(uint16_t port, const char* welcomeLine) :
    tcpServer(port),
    commands(),
    clients(),
    welcomeLine(welcomeLine) {

    tcpServer.begin();

    registerCommand("help", new HelpCommand(*this));
    registerCommand("quit", new QuitCommand());
}

CommandLineTCPServer::~CommandLineTCPServer() {}

bool CommandLineTCPServer::update() {
    acceptNewClients();
    updateClients();

    return !clients.empty();
}

void CommandLineTCPServer::acceptNewClients() {
    while (WiFiClient newClient = tcpServer.available()) {
        std::unique_ptr<Client> ptr = std::make_unique<WiFiClient>(newClient);

        addCustomClient(std::move(ptr));
    }
}

void CommandLineTCPServer::updateClients() {
    for (size_t i = 0; i < clients.size(); ++i) {
        CommandLineConnection& connection = *clients[i].get();

        if (!updateClient(connection) || !connection.connected()) {
            clients.erase(clients.begin() + i);
            break;
        }
    }
}

bool CommandLineTCPServer::updateClient(CommandLineConnection& connection) const {
    return connection.update();
}

void CommandLineTCPServer::registerCommand(const char *name, ICommandLineHandler* commandHandler) {
    commands[name] = new TCommand(commandHandler);
}

ICommandLineHandler* CommandLineTCPServer::getCommandHandlerByName(const std::string& name) const {
    const auto& cmdMap = getCommandList();
    auto iter = cmdMap.find(name);

    if (iter == cmdMap.end())
        return nullptr;

    return iter->second->handler.get();
}

const std::string& CommandLineTCPServer::getWelcomeLine() const {
    return welcomeLine;
}

void CommandLineTCPServer::addCustomClient(std::unique_ptr<Client>&& newClient) {
    clients.emplace_back(std::move(new CommandLineConnection(std::move(newClient), *this)));
}
