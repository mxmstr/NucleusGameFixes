#ifndef _PROCADDRHELPER_H
#define _PROCADDRHELPER_H

#include <sstream>
#include <vector>
#include <windows.h>
#include <string>
#include <winsock2.h>
#include <algorithm>
#include <DbgHelp.h>
#include <Psapi.h>
using namespace std;

//
// Define types
//
#ifdef COMPILE_X64
typedef unsigned long long  duint;
typedef signed long long    dsint;
#else
typedef unsigned long __w64 duint;
typedef signed long __w64   dsint;
#endif // COMPILE_X64

#define MAX_SECTION_SIZE 10
#define MAX_MODULE_SIZE 256

// Macros to safely access IMAGE_NT_HEADERS fields since the compile-time typedef of this struct may not match the actual file bitness.
// Never access OptionalHeader.xx values directly unless they have the same size and offset on 32 and 64 bit. IMAGE_FILE_HEADER fields are safe to use
#define IMAGE32(NtHeaders) ((NtHeaders) != nullptr && (NtHeaders)->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
#define IMAGE64(NtHeaders) ((NtHeaders) != nullptr && (NtHeaders)->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
#define HEADER_FIELD(NtHeaders, Field) (IMAGE64(NtHeaders) \
    ? ((PIMAGE_NT_HEADERS64)(NtHeaders))->OptionalHeader.Field : (IMAGE32(NtHeaders) \
        ? ((PIMAGE_NT_HEADERS32)(NtHeaders))->OptionalHeader.Field \
        : 0))
#define THUNK_VAL(NtHeaders, Ptr, Val) (IMAGE64(NtHeaders) \
    ? ((PIMAGE_THUNK_DATA64)(Ptr))->Val : (IMAGE32(NtHeaders) \
        ? ((PIMAGE_THUNK_DATA32)(Ptr))->Val \
        : 0))

//std::string vsprintf(_In_z_ _Printf_format_string_ const char* format, va_list args)
//{
//    char sbuffer[64] = "";
//    if (_vsnprintf_s(sbuffer, _TRUNCATE, format, args) != -1)
//        return sbuffer;
//
//    std::vector<char> buffer(256, '\0');
//    while (true)
//    {
//        int res = _vsnprintf_s(buffer.data(), buffer.size(), _TRUNCATE, format, args);
//        if (res == -1)
//        {
//            buffer.resize(buffer.size() * 2);
//            continue;
//        }
//        else
//            break;
//    }
//    return std::string(buffer.data());
//}
//
//std::string sprintf(_In_z_ _Printf_format_string_ const char* format, ...)
//{
//    va_list args;
//    va_start(args, format);
//    auto result = vsprintf(format, args);
//    va_end(args);
//    return result;
//}
//
//std::wstring vsprintf(_In_z_ _Printf_format_string_ const wchar_t* format, va_list args)
//{
//    wchar_t sbuffer[64] = L"";
//    if (_vsnwprintf_s(sbuffer, _TRUNCATE, format, args) != -1)
//        return sbuffer;
//
//    std::vector<wchar_t> buffer(256, L'\0');
//    while (true)
//    {
//        int res = _vsnwprintf_s(buffer.data(), buffer.size(), _TRUNCATE, format, args);
//        if (res == -1)
//        {
//            buffer.resize(buffer.size() * 2);
//            continue;
//        }
//        else
//            break;
//    }
//    return std::wstring(buffer.data());
//}
//
//std::wstring sprintf(_In_z_ _Printf_format_string_ const wchar_t* format, ...)
//{
//    va_list args;
//    va_start(args, format);
//    auto result = vsprintf(format, args);
//    va_end(args);
//    return result;
//}

static int hackicmp(const char* s1, const char* s2)
{
    unsigned char c1, c2;
    while ((c1 = *s1++) == (c2 = *s2++))
        if (c1 == '\0')
            return 0;
    s1--, s2--;
    while ((c1 = tolower(*s1++)) == (c2 = tolower(*s2++)))
        if (c1 == '\0')
            return 0;
    return c1 - c2;
}

struct MODSECTIONINFO
{
    duint addr; // Virtual address
    duint size; // Virtual size
    char name[MAX_SECTION_SIZE * 5]; // Escaped section name
};

struct MODRELOCATIONINFO
{
    DWORD rva; // Virtual address
    BYTE type; // Relocation type (IMAGE_REL_BASED_*)
    WORD size;

    bool Contains(duint Address) const
    {
        return Address >= rva && Address < rva + size;
    }
};

struct PdbValidationData
{
    GUID guid;
    DWORD signature = 0;
    DWORD age = 0;

    PdbValidationData()
    {
        memset(&guid, 0, sizeof(guid));
    }
};

typedef enum
{
    sym_import,
    sym_export,
    sym_symbol
} SYMBOLTYPE;

typedef struct SYMBOLINFO_
{
    duint addr;
    char* decoratedSymbol;
    char* undecoratedSymbol;
    SYMBOLTYPE type;

    // If true: Use BridgeFree(decoratedSymbol) to deallocate
    // Else: The decoratedSymbol pointer is valid until the module unloads
    bool freeDecorated;

    // If true: Use BridgeFree(undecoratedSymbol) to deallcoate
    // Else: The undecoratedSymbol pointer is valid until the module unloads
    bool freeUndecorated;

    // The entry point pseudo-export has ordinal == 0 (invalid ordinal value)
    DWORD ordinal;
} SYMBOLINFO;

struct MODEXPORT
{
    DWORD ordinal = 0;
    DWORD rva = 0;
    bool forwarded = false;
    std::string forwardName;
    std::string name;
    std::string undecoratedName;
};

struct NameIndex
{
    const char* name;
    size_t index;

    bool operator<(const NameIndex& b) const
    {
        return cmp(*this, b, false) < 0;
    }

    static int cmp(const NameIndex& a, const NameIndex& b, bool caseSensitive)
    {
        return (caseSensitive ? strcmp : hackicmp)(a.name, b.name);
    }

    //static bool findByPrefix(const std::vector<NameIndex>& byName, const std::string& prefix, const std::function<bool(const NameIndex&)>& cbFound, bool caseSensitive);
    //static bool findByName(const std::vector<NameIndex>& byName, const std::string& name, NameIndex& foundIndex, bool caseSensitive);
};

struct MODINFO
{
    duint base = 0; // Module base
    duint size = 0; // Module size
    duint hash = 0; // Full module name hash
    duint entry = 0; // Entry point
    duint headerImageBase = 0; // ImageBase field in OptionalHeader

    char name[MAX_MODULE_SIZE]; // Module name (without extension)
    char extension[MAX_MODULE_SIZE]; // File extension (including the dot)
    char path[MAX_PATH]; // File path (in UTF8)

    PIMAGE_NT_HEADERS headers = nullptr; // Image headers. Always use HEADER_FIELD() to access OptionalHeader values

    std::vector<MODSECTIONINFO> sections;
    std::vector<MODRELOCATIONINFO> relocations;
    std::vector<duint> tlsCallbacks;
#if _WIN64
    std::vector<RUNTIME_FUNCTION> runtimeFunctions; //sorted by (begin, end)

    const RUNTIME_FUNCTION* findRuntimeFunction(DWORD rva) const;
#endif // _WIN64

    std::vector<MODEXPORT> exports;
    DWORD exportOrdinalBase = 0; //ordinal - 'exportOrdinalBase' = index in 'exports'
    std::vector<NameIndex> exportsByName; //index in 'exports', sorted by export name
    std::vector<size_t> exportsByRva; //index in 'exports', sorted by rva

    //SymbolSourceBase* symbols = nullptr;
    std::string pdbSignature;
    std::string pdbFile;
    PdbValidationData pdbValidation;
    std::vector<std::string> pdbPaths; // Possible PDB paths (tried in order)

    HANDLE fileHandle = nullptr;
    DWORD loadedSize = 0;
    HANDLE fileMap = nullptr;
    ULONG_PTR fileMapVA = 0;

    //MODULEPARTY party;  // Party. Currently used value: 0: User, 1: System
    bool isVirtual = false;
    //Memory<unsigned char*> mappedData;

    MODINFO()
    {
        memset(name, 0, sizeof(name));
        memset(extension, 0, sizeof(extension));
        memset(path, 0, sizeof(path));
    }

    ~MODINFO()
    {
        //unmapFile();
        //unloadSymbols();
        //GuiInvalidateSymbolSource(base);
    }

    //bool loadSymbols(const std::string& pdbPath, bool forceLoad);
    //void unloadSymbols();
    //void unmapFile();
    //const MODEXPORT* findExport(duint rva) const;
    //duint getProcAddress(const std::string& name, int maxForwardDepth = 10) const;
};

ULONG64 ModRvaToOffset(ULONG64 base, PIMAGE_NT_HEADERS ntHeaders, ULONG64 rva);
void ReadExportDirectory(MODINFO& Info, ULONG_PTR FileMapVA);
void* GetProcAddressManual(HMODULE hModule, LPCSTR lpProcName);

#endif // _PROCADDRHELPER_H