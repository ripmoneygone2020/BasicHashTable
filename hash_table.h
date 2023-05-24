#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "../Platform/PlatformIncludes.h"
#include "../Platform/PlatformTypes.h"
#include "../String/NgString.h"

#include <cstring>
#include <cstdlib>

namespace ng {

namespace {
    enum hash_table_entry_key_type {
        key_UINT64 = 0,
        key_UINT32 = 1,
        key_UINT16 = 2,
        key_UINT8  = 3,
        key_STRING = 4
    };

    struct hash_table_entry_key {
        hash_table_entry_key_type Type;
        union {
            UINT64 ui64KeyValue;
            UINT32 ui32KeyValue;
            UINT16 ui16KeyValue;
            UINT8  ui8KeyValue;
            CHAR*  sKeyValue;
        } Value;
    };

    struct hash_table_entry_key_value {
        UINT64 Size;
        LPVOID Data;
    };

    struct hash_table_entry {
        hash_table_entry_key       Key;
        hash_table_entry_key_value Value;
        bool                       Empty;
    };
}

class hash_table {
    hash_table_entry** Entries;
    UINT64             Capacity;
    UINT64             Size;

public:
    hash_table();
    ~hash_table();

public:
    bool contains(const CHAR* key) const;
    void assign(const CHAR* key, LPVOID data, UINT64 dataSize);
    void insert(const CHAR* Key, LPVOID Data, UINT64 DataSize);
    void erase(const CHAR* Key);

private:
    UINT64 probe_table(hash_table_entry** entries, UINT64 capacity, UINT64 hashCode);
    void insert_at(hash_table_entry** entries, UINT64 capacity, UINT64 hashCode, hash_table_entry* entry);
    void insert_at(hash_table_entry** entries, UINT64 capacity, UINT64 hashCode, 
                   const CHAR* key, UINT64 keyLength, LPVOID Data, UINT64 DataSize);

    void insert_existing(hash_table_entry** entries, UINT64 capacity, hash_table_entry* entry);
    void rehash(hash_table_entry** entries, UINT64 newCapacity);

public:
    void rehash(UINT64 newCapacity);
    void reserve(UINT64 capacity);

};

}

#endif