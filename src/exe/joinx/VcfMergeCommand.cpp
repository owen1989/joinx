#include "VcfMergeCommand.hpp"

#include "fileformats/FastaReader.hpp"
#include "fileformats/InputStream.hpp"
#include "fileformats/OutputWriter.hpp"
#include "fileformats/TypedStream.hpp"
#include "fileformats/vcf/Builder.hpp"
#include "fileformats/vcf/Entry.hpp"
#include "fileformats/vcf/Header.hpp"
#include "fileformats/vcf/MergeStrategy.hpp"
#include "processors/MergeSorted.hpp"

#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <cstdint>
#include <functional>
#include <stdexcept>

namespace po = boost::program_options;
using boost::format;
using namespace std;
using namespace std::placeholders;

CommandBase::ptr VcfMergeCommand::create(int argc, char** argv) {
    std::shared_ptr<VcfMergeCommand> app(new VcfMergeCommand);
    app->parseArguments(argc, argv);
    return app;
}

VcfMergeCommand::VcfMergeCommand()
    : _outputFile("-")
    , _clearFilters(false)
    , _mergeSamples(false)
{
}

void VcfMergeCommand::parseArguments(int argc, char** argv) {
    po::options_description opts("Available Options");
    opts.add_options()
        ("help,h", "this message")
        ("input-file,i", po::value< vector<string> >(&_filenames), "input file(s) (positional arguments work also)")
        ("output-file,o", po::value<string>(&_outputFile), "output file (empty or - means stdout, which is the default)")
        ("merge-strategy-file,M", po::value<string>(&_mergeStrategyFile), "merge strategy file for info fields (see man page for format)")
        ("clear-filters,c", "When set, merged entries will have FILTER data stripped out")
        ("merge-samples,s", "allow input files with overlapping samples")
        ;
    po::positional_options_description posOpts;
    posOpts.add("input-file", -1);

    po::variables_map vm;
    po::store(
        po::command_line_parser(argc, argv)
            .options(opts)
            .positional(posOpts).run(),
        vm
    );
    po::notify(vm);

    if (vm.count("help")) {
        stringstream ss;
        ss << opts;
        throw runtime_error(ss.str());
    }

    if (!vm.count("input-file"))
        _filenames.push_back("-");

    if (vm.count("clear-filters"))
        _clearFilters = true;

    if (vm.count("merge-samples"))
        _mergeSamples = true;
}

namespace {

}

void VcfMergeCommand::exec() {
    vector<InputStream::ptr> inputStreams = _streams.wrap<istream, InputStream>(_filenames);
    ostream* out = _streams.get<ostream>(_outputFile);
    if (_streams.cinReferences() > 1)
        throw runtime_error("stdin listed more than once!");

    typedef function<void(const Vcf::Header*, string&, Vcf::Entry&)> VcfExtractor;
    typedef TypedStream<Vcf::Entry, VcfExtractor> ReaderType;
    typedef shared_ptr<ReaderType> ReaderPtr;
    typedef OutputWriter<Vcf::Entry> WriterType;

    vector<ReaderPtr> readers;
    VcfExtractor extractor = bind(&Vcf::Entry::parseLine, _1, _2, _3);
    uint32_t headerIndex(0);

    Vcf::Header mergedHeader;
    for (size_t i = 0; i < inputStreams.size(); ++i) {
        readers.push_back(ReaderPtr(new ReaderType(extractor, *inputStreams[i])));
        mergedHeader.merge(readers.back()->header(), _mergeSamples);
        readers.back()->header().sourceIndex(headerIndex++);
    }

    WriterType writer(*out);
    Vcf::MergeStrategy mergeStrategy(&mergedHeader);
    if (!_mergeStrategyFile.empty()) {
        InputStream::ptr msFile(_streams.wrap<istream, InputStream>(_mergeStrategyFile));
        mergeStrategy.parse(*msFile);
    }
    mergeStrategy.clearFilters(_clearFilters);
    mergeStrategy.mergeSamples(_mergeSamples);
    mergeStrategy.primarySampleStreamIndex(0);

    Vcf::Builder builder(mergeStrategy, &mergedHeader, writer);
    *out << mergedHeader;
    MergeSorted<Vcf::Entry, ReaderPtr, Vcf::Builder> merger(readers, builder);
    merger.execute();
    builder.flush();
}
