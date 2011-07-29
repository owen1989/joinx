#pragma once

#include "common/Variant.hpp"

#include <boost/format.hpp>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
#include <sstream>

class RemappedContig {
public:
    RemappedContig(
            const std::string& chrom,
            uint32_t start,
            uint32_t stop,
            const std::string pre,
            const char* var,
            std::string::size_type varLen,
            const std::string post,
            int insertedBases
            )
    {
        using boost::format;
        using std::string;

        _sequence = pre;
        _sequence.append(var, varLen);
        _sequence += post;

        std::stringstream cigar;
        if (!pre.empty()) cigar << pre.size() << '=';
        if (insertedBases == 0)     cigar << "1X"; // snv
        else if (insertedBases > 0) cigar << insertedBases << 'I'; // ins
        else                        cigar << (-insertedBases) << 'D'; // del
        if (!post.empty()) cigar << post.size() << '=';

        _name = str(format("REMAP-%1%|%2%|%3%-%4%")
            %chrom
            %start
            %stop
            %cigar.str()
            );

        if (varLen) {
            _name += '-';
            _name.append(var, varLen);
        }
    }

    const std::string& name() const     { return _name; }
    const std::string& sequence() const { return _sequence; }

protected:
    std::string _name;
    std::string _sequence;
};

template<typename RefSeq, typename Callback>
class RemappedContigGenerator {
public:

    RemappedContigGenerator(RefSeq& refSeq, unsigned flankSize, Callback& cb)
        : _refSeq(refSeq)
        , _flankSize(flankSize)
        , _cb(cb)
    {
    }

    void generate(const Variant& v) {
        using std::string;
        int32_t start = v.start() > _flankSize ? v.start() - _flankSize : 0;
        int32_t stop = v.stop() + _flankSize;

        string preFlank;
        string postFlank;
        _refSeq.sequence(v.chrom(), start, v.start(), preFlank);
        try {
            stop = _refSeq.sequence(v.chrom(), v.stop(), stop, postFlank);
        } catch (std::length_error&) {
            stop = v.stop();
        }

        const string& ref = v.reference().data();
        const string& var = v.variant().data();

        if (v.type() == Variant::SNP) {
            const char* bases = translateIub(var);
            while (*bases) {
                if (*bases != ref[0]) {
                    RemappedContig ctg(
                        v.chrom(),
                        start,
                        stop,
                        preFlank,
                        bases, 1,
                        postFlank,
                        0
                        );
                    _cb(ctg);
                }
                ++bases;
            }
        } else {
            int isize = v.type() == Variant::DEL ? -ref.size() : var.size();
            int strLen = v.type() == Variant::DEL ? 0 : var.size();
            RemappedContig ctg(
                v.chrom(),
                start,
                stop,
                preFlank,
                &var[0], strLen,
                postFlank,
                isize
                );
                _cb(ctg);
        }
    }
protected:
    RefSeq& _refSeq;
    unsigned _flankSize;
    Callback& _cb;
};
