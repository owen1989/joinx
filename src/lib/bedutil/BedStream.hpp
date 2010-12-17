#pragma once

#include "Bed.hpp"
#include "intconfig.hpp"

#include <iostream>
#include <string>
#include <vector>

class BedFilterBase;

class BedStream {
public:
    BedStream(const std::string& name, std::istream& in);

    void addFilter(BedFilterBase* filter);

    operator bool() const {
        return !eof();
    }

    const std::string& name() const;
    uint64_t lineNum() const;
    uint64_t bedCount() const;

    bool eof() const;
    void checkEof() const; // check and throw
    bool peek(Bed** bed);
    bool next(Bed& bed);

protected:
    std::string nextLine();
    bool exclude(const Bed& bed);

protected:
    std::string _name;
    std::istream& _in;
    uint64_t _lineNum;
    uint64_t _bedCount;
    std::vector<BedFilterBase*> _filters;

    bool _cached;
    Bed _cachedBed;
};

BedStream& operator>>(BedStream& s, Bed& bed);

inline const std::string& BedStream::name() const {
    return _name;
}

inline uint64_t BedStream::lineNum() const {
    return _lineNum;
}

inline uint64_t BedStream::bedCount() const {
    return _bedCount;
}
