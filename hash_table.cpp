#include "../hash_table.h"

static
UINT64 StringLength(const CHAR* string) {
    UINT64 Length = 0;
    const CHAR* Ptr = string;
    while(Ptr != 0 && *Ptr != '\0') {
        Length++;
        Ptr++;
    }

    return Length; 
}

static
void StringCopy(CHAR** Dest, const CHAR* Src, UINT64 SrcLength) {
    if(Dest && Src && SrcLength > 0) {
        *Dest = (CHAR*) malloc(sizeof(CHAR) * SrcLength);
        for(UINT64 I = 0; I < SrcLength; I++)
            *Dest[I] = Src[I];
    }
}

static const UINT64 SELECT_PRIMES[] = {
    53, 97, 193, 389, 769,
    1543, 3079, 6151, 12289,
    24593, 49157, 98317, 196613,
    393241, 786433, 1572869, 3145739,
    6291469, 12582917, 25165843,
    50331653, 100663319, 201326611,
    402653189, 805306457, 1610612741
};

static
UINT64 GetNextPrimeCapacity(UINT64 capacity) {
    UINT64 next_prime_capacity = 0; 
    for(UINT64 I = 0; I < 26; I++) {
        if(capacity < SELECT_PRIMES[I]) {
            next_prime_capacity = SELECT_PRIMES[I];
            break;
        }
    }

    return next_prime_capacity;
}

static
UINT64 Hash(const CHAR* Key, UINT64 KeyLength, UINT64 TableSize) {
    UINT64 Hash = 31;
    for(UINT64 I = 0; I < KeyLength; I++)
        Hash = Hash * 31 + ((UINT64) Key[I]);

    return Hash % TableSize;
}

#define INITIAL_HASH_TABLE_CAPACITY SELECT_PRIMES[0]
#define HASH_TABLE_MAX_LOAD_CAPACITY 0.7f

namespace ng {

static 
void SetHashTableEntryKey(hash_table_entry* Entry, const CHAR* Key, UINT64 KeyLength) {
    Entry->Key.Type = hash_table_entry_key_type::key_STRING;
    StringCopy(&Entry->Key.Value.sKeyValue, Key, KeyLength);
}

static
void SetHashTableEntryValue(hash_table_entry* Entry, LPVOID Data, UINT64 DataSize) {
    Entry->Value.Data = Data;
    Entry->Value.Size = DataSize;
}

hash_table::hash_table() {
    Entries         = 0;
    Capacity        = 0;
    Size            = 0;

    /* I'VE DECIDED THIS TABLE WILL HAVE OVERHEAD
       SO THE TOTAL CAPACITY OR MEMORY USAGE
       WILL EXCEED WHATS REQUIRED BY ALOT PROBABLY */
    reserve(INITIAL_HASH_TABLE_CAPACITY);
}

hash_table::~hash_table() {
    if(Entries) {
        for(UINT64 I = 0; I < Capacity; I++) {
            if(Entries[I]) {
                free(Entries[I]);
                Entries[I] = 0;
            }
        }

        free(Entries);
        Entries = 0;
    }

    Capacity = 0;
    Size     = 0;
}

void hash_table::rehash(hash_table_entry** entries, UINT64 newCapacity) {
    for(UINT64 I = 0; I < Capacity; I++) {
        if(Entries[I]) {
            hash_table_entry* entry = Entries[I];
            insert_existing(entries, newCapacity, entry);
        }
    }
}

void hash_table::rehash(UINT64 newCapacity) {
    if(newCapacity > 0 && newCapacity > Capacity) {
        hash_table_entry** entries = (hash_table_entry**) malloc(sizeof(hash_table_entry*) * newCapacity);
        rehash(entries, newCapacity);

        if(Entries) {
            free(Entries);
            Entries = 0;
        }

        // update internal state vars
        Entries = entries;
        Capacity = newCapacity;
    }
}

void hash_table::reserve(UINT64 capacity) {
    if(capacity > 0 && capacity > Capacity) {
        hash_table_entry** entries = (hash_table_entry**) malloc(sizeof(hash_table_entry*) * capacity);

        for(UINT64 I = 0; I < capacity; I++)
            entries[I] = 0;

        if(Size > 0)
            rehash(entries, capacity);

        if(Entries) {
            free(Entries);
            Entries = nullptr;
        }

        // update internal state vars
        Entries = entries;
        Capacity = capacity;
    }
}

UINT64 hash_table::probe_table(hash_table_entry** entries, UINT64 capacity, UINT64 hashCode) {
    UINT64 probeIdx = hashCode;
    if(entries[probeIdx]) {
        probeIdx = hashCode + 1;
        while(probeIdx < capacity) {
            if(!entries[probeIdx])
                break;

            probeIdx++;
        }
    }

    return probeIdx;
}

void hash_table::insert_at(hash_table_entry** entries, UINT64 capacity, UINT64 hashCode, hash_table_entry* entry) {
    UINT64 probeIdx = probe_table(entries, capacity, hashCode);
    if(probeIdx < capacity)
        entries[probeIdx] = entry;
}

void hash_table::insert_at(hash_table_entry** entries, UINT64 capacity, UINT64 hashCode, 
                           const CHAR* key, UINT64 keyLength, LPVOID data, UINT64 dataSize) {
    
    UINT64 probeIdx = probe_table(entries, capacity, hashCode);
    if(probeIdx < capacity) {
        hash_table_entry* entry = (hash_table_entry*) malloc(sizeof(hash_table_entry));
        SetHashTableEntryKey(entry, key, keyLength);
        SetHashTableEntryValue(entry, data, dataSize);

        entries[probeIdx] = entry;
    }
}

void hash_table::insert_existing(hash_table_entry** entries, UINT64 capacity, hash_table_entry* entry) {
    UINT64 hashCode = 0;

    switch(entry->Key.Type) {
        case hash_table_entry_key_type::key_UINT64: 
        case hash_table_entry_key_type::key_UINT32: 
        case hash_table_entry_key_type::key_UINT16: 
        case hash_table_entry_key_type::key_UINT8:  {
            // hash integer, just mod the tablesize;
            break;
        }
        case hash_table_entry_key_type::key_STRING: {
            UINT64 keyLength = StringLength(entry->Key.Value.sKeyValue);
            hashCode = Hash(entry->Key.Value.sKeyValue, keyLength, capacity);
            break;
        }
    }

    insert_at(entries, capacity, hashCode, entry);
}

void hash_table::erase(const CHAR* Key) {
    UINT64 length = StringLength(Key);
    if(length > 0 && Size > 0) {
        UINT64 hashCode = Hash(Key, length, Capacity);
        if(Entries[hashCode]) {
            free(Entries[hashCode]);
            Entries[hashCode] = 0;
            Size--;
        }
    }
}

void hash_table::assign(const CHAR* key, LPVOID data, UINT64 dataSize) {
    if(contains(key)) {
        UINT64 length = StringLength(key);
        UINT64 hashCode = Hash(key, length, Capacity);

        hash_table_entry* entry = Entries[hashCode];
        SetHashTableEntryValue(entry, data, dataSize);
    }
}

void hash_table::insert(const CHAR* Key, LPVOID Data, UINT64 DataSize) { 
    if((Size / (FLOAT) Capacity) >= HASH_TABLE_MAX_LOAD_CAPACITY)
        rehash(GetNextPrimeCapacity(Capacity));

    UINT64 KeyLength = StringLength(Key);
    if(KeyLength > 0 && Data) {
        UINT64 hashCode = Hash(Key, KeyLength, Capacity);
        insert_at(Entries, Capacity, hashCode, 
                  Key, KeyLength, Data, DataSize);
    }
}

bool hash_table::contains(const CHAR* key) const {
    bool doesContain = false;
    UINT64 length = StringLength(key);

    if(length > 0) {
        UINT64 hashCode = Hash(key, length, Capacity);
        doesContain = (Entries[hashCode] != nullptr);
    }

    return doesContain;
}

}