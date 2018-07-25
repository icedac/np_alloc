#pragma once
/****************************************************************************
 *
 *  np_bitset.h
 *      ($\np_alloc\src)
 *
 *  by icedac 
 *
 ***/
#ifndef _____NP_ALLOC__NP_BITSET_H_
#define _____NP_ALLOC__NP_BITSET_H_

#include "np_common.h"

namespace np {

    /****************************************************************************
     *
     *  popcnt support
     *
     * 
     *     'popcnt' supports from Intel Neharlem(2008)
     *     'lzcnt' supports from Intel Haswell(2013)
     *         i7/i5/i3 4XXX/5XXX
     *         Xeon E3/E5/E7 v3
     *     SSE4a (inc. popcnt, lzcnt) supports from AMD K10 (2007)
     *
     *     so nowdays modern cpu has 'popcnt', so use it here
     *
     */
    
    /****************************************************************************
     *  template<> bitset
     *
     *  fast bitset
     *      with 'popcnt', 'bsr' intrinsic
     */
    template < uint64 >
    class bitset;

    template<>
    class bitset<64>
    {
    public:
        static const auto kIndexSize = 64;
        bitset(uint64 b = 0) : bits_(b) {}

        uint64 get_frist() const        { return bits_ == 0 ? kIndexSize : np::clz(bits_); }
        bool get(uint64 pos) const      { return (bits_ & (1ll << (63 - pos)))>0; }
        void set(uint64 pos)            { bits_ |= (1ll << (63 - pos)); }
        bool clear(uint64 pos)          { bits_ &= ~(1ll << (63 - pos)); return bits_ == 0; }
        uint64 size() const             { return kIndexSize; }
        uint64 count() const            { return np::popcnt(bits_); }
        void set_all()                  { bits_ = (uint64)-1; }
        void clear_all()                { bits_ = 0; }
        uint64 get_raw_bits() const     { return bits_; }

        class iterator {
        public:
            bool end() const            { return pos_ == kIndexSize; }
            void set()                  { self_.set(pos_); }
            bool clear()                { return self_.clear(pos_); }
            bool operator*() const      { return self_.get(pos_); }
            byte get_pos() const        { return pos_; }
            iterator& next() {
                ++pos_;
                if (end()) return *this;
                bitset t((self_.get_raw_bits() << pos_) >> pos_);
                pos_ = (byte)t.get_frist();
                return *this;
            }
            bool operator == (const iterator& r) {
                if (self_ == r.self_ && pos_ == r.pos_) return true;
                return false;
            }
            iterator(bitset& self, uint64 pos = kIndexSize) : self_(self), pos_((byte)pos) {}
            iterator(const iterator& r) : self_(r.self_), pos_(r.pos_) {}
            iterator& operator = (const iterator& r) { self_ = r.self_; pos_ = r.pos_; return *this; }
        private:
            bitset& self_;
            byte pos_ = 0;
        };

        iterator begin()                { return iterator(*this, 0); }
        iterator at( uint64 pos )       { return iterator(*this, pos); }
        iterator end()                  { return iterator(*this, kIndexSize); }
        bool operator == (const bitset& r) {
            if (get_raw_bits() == r.get_raw_bits()) return true;
            return false;
        }

    private:
        friend class iterator;
        uint64 bits_ = 0;
    };

    // working for size = 64 ~ 64^2(4096)
    template<uint64 TSize>
    class bitset
    {
    public:
        constexpr static auto kIndexSize = TSize;
        constexpr static auto kSubIndexSize = 64;
        constexpr static auto kSubIndexCount = ( (kIndexSize + (kSubIndexSize-1)) / kSubIndexSize);

        static_assert(TSize > 64, "size should be greather than 64");
        static_assert(kSubIndexCount <= 64, "size can not be greater than 64^2(4096)");

        typedef bitset<64>              index_t;
        typedef bitset<kSubIndexSize>   sub_index_t;

        bitset() {}

        struct subpos_t {
            uint64 idx = 0;
            uint64 pos = 0;
        };

        auto get_subpos(uint64 pos) const {
            assert(pos <= kIndexSize);
            subpos_t p;
            p.idx = pos / kSubIndexSize;
            p.pos = pos % kSubIndexSize;
            return p;
        }

        uint64 get_frist() const { 
            auto idx = index_.get_frist();
            if (idx >= kSubIndexCount) return kIndexSize; // all zero
            auto sub_idx = sub_index_[idx].get_frist();
            return idx * kSubIndexSize + sub_idx;
        }
        bool get(uint64 pos) const { 
            auto subpos = get_subpos(pos);
            return sub_index_[subpos.idx].get(subpos.pos);
        }
        void set(uint64 pos) { 
            auto subpos = get_subpos(pos);
            index_.set(subpos.idx);
            sub_index_[subpos.idx].set(subpos.pos);
        }
        bool clear(uint64 pos) { 
            auto subpos = get_subpos(pos);
            if ( sub_index_[subpos.idx].clear(subpos.pos) )
                return index_.clear(subpos.idx);
            return false;
        }
        uint64 size() const { return kIndexSize; }
        uint64 count() const { 
            uint64 c = 0;
            for (const auto& sub : sub_index_) {
                c += sub.count();
            }
            return c;
        }
        void set_all() { 
            index_.set_all();
            for (auto& sub : sub_index_) {
                sub.set_all();
            }
        }
        void clear_all() { 
            index_.clear_all();
            for (auto& sub : sub_index_) {
                sub.clear_all();
            }
        }

        // uint64 get_raw_bits() const { return bits_; }

        class iterator {
        public:
            bool end() const { return pos_ == kIndexSize; }
            void set() { self_.set(pos_); }
            bool clear() { return self_.clear(pos_); }
            bool operator*() const { return self_.get(pos_); }
            uint64 get_pos() const { return pos_; }

            // bad performance
            iterator& next() {
                ++pos_;
                if (end()) return *this;

                for (pos_; pos_ < kIndexSize; ++pos_) {
                    if (self_.get(pos_)) {
                        return *this;
                    }
                }
                return *this;
            }

            iterator(bitset& self, uint64 pos = kIndexSize) : self_(self), pos_(pos) {}
            iterator(const iterator& r) : self_(r.self_), pos_(r.pos_) {}
            iterator& operator = (const iterator& r) { 
                self_ = r.self_; 
                pos_ = r.pos_; 
                return *this; 
            }
        private:
            bitset& self_;
            uint64 pos_ = 0;
        };

        iterator begin() { return iterator(*this, 0); }
        iterator at(uint64 pos) { return iterator(*this, pos); }
        iterator end() { return iterator(*this, kIndexSize); }

    private:
        friend class iterator;
        index_t index_;
        sub_index_t sub_index_[kSubIndexCount];
    };
}


#endif// _____NP_ALLOC__NP_BITSET_H_

