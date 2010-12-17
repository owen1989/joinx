#pragma once

#include "intconfig.hpp"
#include <algorithm>
#include <string>
#include <sstream>

struct Bed
{
    enum Type {
        SNV,
        INDEL
    };

    Bed()
        : start(0)
        , end(0)
    {}

    Bed(const std::string& chrom, uint64_t start, uint64_t end, const std::string& refCall, const std::string& qual)
        : chrom(chrom)
        , start(start)
        , end(end)
        , refCall(refCall)
        , qual(qual)
    {
        std::stringstream ss;
        ss << chrom << "\t" << start << "\t" << end << "\t" << refCall << "\t"
            << qual;
        line = ss.str();
    }

    std::string chrom;
    uint64_t start;
    uint64_t end;
    std::string refCall;
    std::string qual;
    std::string line;

    void swap(Bed& rhs) {
        chrom.swap(rhs.chrom);
        std::swap(start, rhs.start);
        std::swap(end, rhs.end);
        refCall.swap(rhs.refCall);
        qual.swap(rhs.qual);
        line.swap(rhs.line);
    }

    int cmp(const Bed& rhs) const;
    bool operator<(const Bed& rhs) const {
        return cmp(rhs) < 0;
    }

    bool operator==(const Bed& rhs) const {
        return cmp(rhs) == 0;
    }

    bool type() const {
        return (end == start+1) ? SNV : INDEL;
    }

    static void parseLine(std::string& line, Bed& bed);

private:
};

std::ostream& operator<<(std::ostream& s, const Bed& bed);
