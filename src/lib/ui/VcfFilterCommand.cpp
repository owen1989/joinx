#include "VcfFilterCommand.hpp"

#include "fileformats/InputStream.hpp"
#include "fileformats/OutputWriter.hpp"
#include "fileformats/TypedStream.hpp"
#include "fileformats/vcf/Entry.hpp"
#include "fileformats/vcf/Header.hpp"

#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/function.hpp>
#include <boost/program_options.hpp>

#include <stdexcept>

namespace po = boost::program_options;
using boost::format;
using namespace std;

VcfFilterCommand::VcfFilterCommand()
    : _infile("-")
    , _outputFile("-")
    , _minDepth(0)
{
}

void VcfFilterCommand::configureOptions() {
    _opts.add_options()
        ("input-file,i",
            po::value<string>(&_infile)->default_value("-"),
            "input file (empty or - means stdin, which is the default)")

        ("output-file,o",
            po::value<string>(&_outputFile)->default_value("-"),
            "output file (empty or - means stdout, which is the default)")

        ("min-depth,d",
            po::value<uint32_t>(&_minDepth)->default_value(0),
            "minimum depth")
        ;

    _posOpts.add("input-file", -1);
}

void VcfFilterCommand::exec() {
    InputStream::ptr instream(_streams.openForReading(_infile));
    ostream* out = _streams.get<ostream>(_outputFile);
    if (_streams.cinReferences() > 1)
        throw runtime_error("stdin listed more than once!");

    typedef boost::function<void(const Vcf::Header*, string&, Vcf::Entry&)> VcfExtractor;
    typedef TypedStream<Vcf::Entry, VcfExtractor> ReaderType;
    typedef boost::shared_ptr<ReaderType> ReaderPtr;
    typedef OutputWriter<Vcf::Entry> WriterType;

    VcfExtractor extractor = boost::bind(&Vcf::Entry::parseLine, _1, _2, _3);
    WriterType writer(*out);
    ReaderType reader(extractor, *instream);
    Vcf::Entry e;
    *out << reader.header();
    while (reader.next(e)) {
        e.sampleData().removeLowDepthGenotypes(_minDepth);
        if (e.sampleData().samplesWithData())
            writer(e);
    }
}