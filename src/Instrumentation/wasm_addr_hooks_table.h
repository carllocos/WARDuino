#pragma once
#include <cstdint>
#include <cstdlib>

#include "./instrumentation_structs.h"

/*
 * Compact hash table mapping a wasm address to its hooks (HooksWasmAddr).
 *
 * Analyses such as instruction coverage instrument (nearly) every instruction
 * of a module, so the per address overhead of the table matters. A
 * std::unordered_map allocates a node per entry (next pointer, cached hash and
 * malloc overhead) next to a bucket array. This table instead stores the
 * entries inline in a single array using open addressing with linear probing,
 * which takes 16 bytes per entry (64-bit).
 *
 * To keep the table densely filled, the capacity is not restricted to powers
 * of two: it grows by a factor 1.5 once the load factor exceeds 4/5, so the
 * load factor stays between ~0.53 and 0.8. Hash values are mapped onto the
 * capacity with a multiply-shift (no modulo needed).
 *
 * The address field of an entry is the key. Empty slots have the address
 * `EMPTY_ADDR`, which can never be a valid wasm address.
 *
 * Pointers returned by `find` and `insert` stay valid until the next `insert`
 * or `erase` on the table.
 */
class WasmAddrHooksTable {
   public:
    static constexpr uint32_t EMPTY_ADDR = UINT32_MAX;

    WasmAddrHooksTable() = default;
    WasmAddrHooksTable(const WasmAddrHooksTable &) = delete;
    WasmAddrHooksTable &operator=(const WasmAddrHooksTable &) = delete;
    ~WasmAddrHooksTable() { free(this->entries); }

    uint32_t size() const { return this->count; }

    bool contains(uint32_t addr) const { return this->find(addr) != nullptr; }

    HooksWasmAddr *find(uint32_t addr) const {
        if (this->count == 0 || addr == EMPTY_ADDR) return nullptr;
        uint32_t i = this->home(addr);
        while (this->entries[i].address != EMPTY_ADDR) {
            if (this->entries[i].address == addr) return &this->entries[i];
            i = this->next(i);
        }
        return nullptr;
    }

    /*
     * Returns the entry for `addr`, creating an empty entry if absent.
     * Returns nullptr when out of memory.
     */
    HooksWasmAddr *insert(uint32_t addr) {
        if (addr == EMPTY_ADDR) return nullptr;
        HooksWasmAddr *existing = this->find(addr);
        if (existing != nullptr) return existing;
        if ((uint64_t)(this->count + 1) * 5 > (uint64_t)this->capacity * 4) {
            uint32_t newCapacity =
                this->capacity == 0 ? MIN_CAPACITY
                                    : this->capacity + this->capacity / 2;
            if (!this->resize(newCapacity)) return nullptr;
        }
        uint32_t i = this->home(addr);
        while (this->entries[i].address != EMPTY_ADDR) {
            i = this->next(i);
        }
        this->entries[i] = HooksWasmAddr{};
        this->entries[i].address = addr;
        this->count += 1;
        return &this->entries[i];
    }

    bool erase(uint32_t addr) {
        HooksWasmAddr *entry = this->find(addr);
        if (entry == nullptr) return false;
        uint32_t i = (uint32_t)(entry - this->entries);
        // backward shift deletion: move up entries whose probe sequence
        // passes through the freed slot so that lookups stay correct without
        // tombstones
        uint32_t j = i;
        while (true) {
            j = this->next(j);
            if (this->entries[j].address == EMPTY_ADDR) break;
            uint32_t k = this->home(this->entries[j].address);
            bool kInCyclicRange = (i <= j) ? (i < k && k <= j)
                                           : (i < k || k <= j);  // (i, j]
            if (!kInCyclicRange) {
                this->entries[i] = this->entries[j];
                i = j;
            }
        }
        this->entries[i].address = EMPTY_ADDR;
        this->count -= 1;
        if (this->count == 0) {
            // release the memory once no address is instrumented anymore
            free(this->entries);
            this->entries = nullptr;
            this->capacity = 0;
        }
        return true;
    }

   private:
    static constexpr uint32_t MIN_CAPACITY = 16;

    HooksWasmAddr *entries = nullptr;
    uint32_t capacity = 0;
    uint32_t count = 0;

    uint32_t next(uint32_t i) const {
        return i + 1 == this->capacity ? 0 : i + 1;
    }

    uint32_t home(uint32_t addr) const {
        // integer hash (lowbias32) so that nearby addresses spread out
        uint32_t h = addr;
        h ^= h >> 16;
        h *= 0x7feb352dU;
        h ^= h >> 15;
        h *= 0x846ca68bU;
        h ^= h >> 16;
        // map the hash onto [0, capacity)
        return (uint32_t)(((uint64_t)h * this->capacity) >> 32);
    }

    bool resize(uint32_t newCapacity) {
        auto *newEntries =
            (HooksWasmAddr *)malloc(newCapacity * sizeof(HooksWasmAddr));
        if (newEntries == nullptr) return false;
        for (uint32_t i = 0; i < newCapacity; i++) {
            newEntries[i] = HooksWasmAddr{};
            newEntries[i].address = EMPTY_ADDR;
        }
        HooksWasmAddr *oldEntries = this->entries;
        uint32_t oldCapacity = this->capacity;
        this->entries = newEntries;
        this->capacity = newCapacity;
        for (uint32_t i = 0; i < oldCapacity; i++) {
            if (oldEntries[i].address == EMPTY_ADDR) continue;
            uint32_t j = this->home(oldEntries[i].address);
            while (this->entries[j].address != EMPTY_ADDR) {
                j = this->next(j);
            }
            this->entries[j] = oldEntries[i];
        }
        free(oldEntries);
        return true;
    }
};
