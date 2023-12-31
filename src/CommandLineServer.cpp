#include "CommandLineServer.h"

#include "CommandLineConnection.h"

struct HelpCommand : public ICommandLineHandler {
    const ICommandLineServer& server;

    HelpCommand(const ICommandLineServer& server) :
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

CommandLineServer::CommandLineServer(const char* welcomeLine) :
    commands(),
    clients(),
    welcomeLine(welcomeLine ? welcomeLine : "") {
    registerCommand("help", new HelpCommand(*this));
    registerCommand("quit", new QuitCommand());
}

CommandLineServer::~CommandLineServer() {}

CommandLineTCPServer::CommandLineTCPServer(uint16_t port, const char* welcomeLine) :
    tcpServer(port),
    commandLineServer(std::make_shared<CommandLineServer>(welcomeLine)) {

    tcpServer.begin();
}

CommandLineTCPServer::CommandLineTCPServer(uint16_t port, std::shared_ptr<ICommandLineServer> existingServer) :
	tcpServer(port),
	commandLineServer(existingServer) {

	tcpServer.begin();
}

CommandLineTCPServer::~CommandLineTCPServer() {}

bool CommandLineServer::update() {
    updateClients();

    return !clients.empty();
}

bool CommandLineTCPServer::update() {
    acceptNewClients();

    return commandLineServer->update();
}

void CommandLineTCPServer::acceptNewClients() {
    while (WiFiClient newClient = tcpServer.available()) {
        std::unique_ptr<Client> ptr = std::make_unique<WiFiClient>(newClient);

        addCustomClient(std::move(ptr));
    }
}

void CommandLineServer::updateClients() {
    for (size_t i = 0; i < clients.size(); ++i) {
        CommandLineConnection& connection = *clients[i].get();

        if (!updateClient(connection) || !connection.connected()) {
            clients.erase(clients.begin() + i);
            break;
        }
    }
}

bool CommandLineServer::updateClient(CommandLineConnection& connection) const {
    return connection.update();
}

void CommandLineServer::registerCommand(const char *name, ICommandLineHandler* commandHandler) {
    commands[name] = new TCommand(commandHandler);
}

ICommandLineHandler* CommandLineServer::getCommandHandlerByName(const std::string& name) const {
    const auto& cmdMap = getCommandList();
    auto iter = cmdMap.find(name);

    if (iter == cmdMap.end())
        return nullptr;

    return iter->second->handler.get();
}

const std::string& CommandLineServer::getWelcomeLine() const {
    return welcomeLine;
}

void CommandLineServer::addCustomClient(std::unique_ptr<Client>&& newClient) {
    clients.emplace_back(std::move(new CommandLineConnection(std::move(newClient), *this)));
}
