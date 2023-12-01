#include "dbgoutput.h"

bool DBGOutput::openTmpFile() {
    this->fd = mkstemp(filename);
    if (fd == -1) {
        return false;
    }
    this->file = fdopen(fd, "r+");

    // open stream
    this->stream.open(this->filename);
    return true;
}

void DBGOutput::closeTmpFile() {
    close(fd);
    remove(filename);
    stream.close();
}

DBGOutput::DBGOutput(Debugger* t_debugger) : debugger{t_debugger} {}
DBGOutput::DBGOutput() : debugger{nullptr} {}

DBGOutput::~DBGOutput() {
    this->closeTmpFile();
    for (int i = 0; i < this->linesRead.size(); i++) {
        delete this->linesRead[i];
    }
    this->linesRead.clear();
}

bool DBGOutput::open() {
    if (!this->openTmpFile()) return false;

    this->channel = new Sink(file);
    if (this->debugger != nullptr) {
        debugger->channel = this->channel;
    }
    return true;
}
Channel& DBGOutput::getChannel() { return *this->channel; }

std::string* DBGOutput::getLine() {
    if (stream) {
        std::string* line = new std::string;
        std::getline(stream, *line);
        linesRead.push_back(line);
        return line;
    }
    return nullptr;
}

void DBGOutput::appendReadLines(std::string* s) {
    std::string concat;
    uint32_t stringIdx = 0;
    for (const auto& str : this->linesRead) {
        concat += "line ";
        concat += std::to_string(stringIdx++);
        concat += ": ";
        concat += *str;
        concat += "\n";
    }
    s->append(concat);
}

bool DBGOutput::getJSONReply(nlohmann::basic_json<>* dest) {
    std::string* line{};
    while ((line = this->getLine()) != nullptr) {
        try {
            *dest = nlohmann::json::parse(line->c_str());
            return true;
        } catch (const nlohmann::detail::parse_error& e) {
        }
    }
    return false;
}