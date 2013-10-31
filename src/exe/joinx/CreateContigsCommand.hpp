#pragma once

#include "ui/CommandBase.hpp"
#include "fileformats/StreamHandler.hpp"

#include <string>

class CreateContigsCommand : public CommandBase {
public:
    using CommandBase::ptr;

    CreateContigsCommand();
    ptr create(int argc, char** argv);

    std::string name() const { return "create-contigs"; }
    std::string description() const {
        return "generate contigs from variant files";
    }

    void exec();

protected:
    void parseArguments(int argc, char** argv);

protected:
    std::string _referenceFasta;
    std::string _variantsFile;
    std::string _outputFasta;
    std::string _outputRemap;
    int _flankSize;
    int _minQuality;
    StreamHandler _streams;
};
