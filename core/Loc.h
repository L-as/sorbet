#ifndef SORBET_AST_LOC_H
#define SORBET_AST_LOC_H

#include "Files.h"

namespace sorbet::core {
namespace serialize {
class SerializerImpl;
}
class GlobalState;

constexpr int INVALID_POS_LOC = 0xffffff;
struct LocOffsets {
    unsigned int beginLoc : 24;
    unsigned int endLoc : 24;
    u4 beginPos() const {
        return beginLoc;
    };

    u4 endPos() const {
        return endLoc;
    }
    bool exists() const {
        return endLoc != INVALID_POS_LOC && beginLoc != INVALID_POS_LOC;
    }
} __attribute__((packed, aligned(1)));
CheckSize(LocOffsets, 6, 1);

class Loc final {
    struct {
        LocOffsets offsets;
        unsigned int fileRef : 24;
    } __attribute__((packed, aligned(8))) storage;
    template <typename H> friend H AbslHashValue(H h, const Loc &m);
    friend class sorbet::core::serialize::SerializerImpl;

    void setFile(core::FileRef file) {
        storage.fileRef = file.id();
    }

public:
    static Loc none(FileRef file = FileRef()) {
        return Loc{file, INVALID_POS_LOC, INVALID_POS_LOC};
    }

    bool exists() const {
        return storage.fileRef != 0 && storage.offsets.exists();
    }

    Loc join(Loc other) const;

    u4 beginPos() const {
        return storage.offsets.beginLoc;
    };

    u4 endPos() const {
        return storage.offsets.endLoc;
    }

    FileRef file() const {
        return FileRef(storage.fileRef);
    }

    bool isTombStoned(const GlobalState &gs) const {
        auto f = file();
        if (!f.exists()) {
            return false;
        } else {
            return file().data(gs).sourceType == File::Type::TombStone;
        }
    }

    inline Loc(FileRef file, u4 begin, u4 end) : storage{{begin, end}, file.id()} {
        ENFORCE(begin <= INVALID_POS_LOC);
        ENFORCE(end <= INVALID_POS_LOC);
        ENFORCE(begin <= end);
    }

    Loc() : Loc(0, INVALID_POS_LOC, INVALID_POS_LOC){};

    Loc &operator=(const Loc &rhs) = default;
    Loc &operator=(Loc &&rhs) = default;
    Loc(const Loc &rhs) = default;
    Loc(Loc &&rhs) = default;

    struct Detail {
        u4 line, column;
    };

    bool contains(const Loc &other) const;
    std::pair<Detail, Detail> position(const GlobalState &gs) const;
    std::string toStringWithTabs(const GlobalState &gs, int tabs = 0) const;
    std::string toString(const GlobalState &gs) const {
        return toStringWithTabs(gs);
    }
    std::string showRaw(const GlobalState &gs) const;
    std::string filePosToString(const GlobalState &gs) const;
    std::string source(const GlobalState &gs) const;

    bool operator==(const Loc &rhs) const;

    bool operator!=(const Loc &rhs) const;
    static std::optional<u4> pos2Offset(const File &file, Detail pos);
    static Detail offset2Pos(const File &file, u4 off);
    static std::optional<Loc> fromDetails(const GlobalState &gs, FileRef fileRef, Detail begin, Detail end);
    std::pair<u4, u4> getAs2u4() const {
        auto low = (((u4)storage.offsets.beginLoc) << 8) + ((((u4)storage.fileRef) >> 8) & ((1 << 8) - 1));
        auto high = (((u4)storage.offsets.endLoc) << 8) + ((((u4)storage.fileRef)) & ((1 << 8) - 1));
        return {low, high};
    };

    // Intentionally not a constructor because we don't want to ever be able to call it unintentionally
    void setFrom2u4(u4 low, u4 high) {
        storage.fileRef = (high & ((1 << 8) - 1)) + ((low & ((1 << 8) - 1)) << 8);
        storage.offsets.endLoc = high >> 8;
        storage.offsets.beginLoc = low >> 8;
    }

    // For a given Loc, returns
    //
    // - the Loc corresponding to the first non-whitespace character on this line, and
    // - how many characters of the start of this line are whitespace.
    //
    std::pair<Loc, u4> findStartOfLine(const GlobalState &gs) const;

    // For a given Loc, returns a zero-length version that starts at the same location.
    Loc copyWithZeroLength() const {
        return Loc(file(), beginPos(), beginPos());
    }
};
CheckSize(Loc, 16, 8);

template <typename H> H AbslHashValue(H h, const Loc &m) {
    return H::combine(std::move(h), m.storage.offsets.beginLoc, m.storage.offsets.endLoc, m.storage.fileRef);
}
} // namespace sorbet::core

#endif // SORBET_AST_LOC_H
