#include <includes.hpp>
#include "tools/gsc.hpp"
#include "tools/cw/cw.hpp"
#include "actscli.hpp"
#include <decrypt.hpp>

using namespace tool::gsc;

namespace {
    size_t SizeNoEmptyNode(const std::vector<tool::gsc::opcode::ASMContextStatement>& statements) {
        size_t acc = 0;
        for (const auto& stmt : statements) {
            if (stmt.node->m_type != tool::gsc::opcode::TYPE_PRECODEPOS) {
                if (acc || stmt.node->m_type != tool::gsc::opcode::TYPE_END) {
                    acc++;
                }
            }
        }
        return acc;
    }
}

constexpr uint32_t g_constructorName = hashutils::Hash32("__constructor");
constexpr uint32_t g_destructorName = hashutils::Hash32("__destructor");

GscInfoOption::GscInfoOption() {
    // set default formatter
    m_formatter = &tool::gsc::formatter::GetFromName();
}

bool GscInfoOption::Compute(const char** args, INT startIndex, INT endIndex) {
    // default values
    for (size_t i = startIndex; i < endIndex; i++) {
        const char* arg = args[i];

        if (!strcmp("-?", arg) || !_strcmpi("--help", arg) || !strcmp("-h", arg)) {
            m_help = true;
        }
        else if (!strcmp("-g", arg) || !_strcmpi("--gsc", arg)) {
            m_dcomp = true;
        }
        else if (!strcmp("-a", arg) || !_strcmpi("--asm", arg)) {
            m_dasm = true;
        }
        else if (!strcmp("-H", arg) || !_strcmpi("--header", arg)) {
            m_header = true;
        }
        else if (!strcmp("-I", arg) || !_strcmpi("--imports", arg)) {
            m_imports = true;
        }
        else if (!strcmp("-S", arg) || !_strcmpi("--strings", arg)) {
            m_strings = true;
        }
        else if (!strcmp("-G", arg) || !_strcmpi("--gvars", arg)) {
            m_gvars = true;
        }
        else if (!strcmp("-U", arg) || !_strcmpi("--noincludes", arg)) {
            m_includes = false;
        }
        else if (!strcmp("-X", arg) || !_strcmpi("--exptests", arg)) {
            m_exptests = true;
        }
        else if (!_strcmpi("--nofunc", arg)) {
            m_func = false;
        }
        else if (!strcmp("-F", arg) || !_strcmpi("--nofuncheader", arg)) {
            m_func_header = false;
        }
        else if (!strcmp("-p", arg) || !_strcmpi("--postfunchead", arg)) {
            m_func_header_post = true;
        }
        else if (!strcmp("-l", arg) || !_strcmpi("--rloc", arg)) {
            m_func_rloc = true;
        }
        else if (!strcmp("-P", arg) || !_strcmpi("--nopatch", arg)) {
            m_patch = false;
        }
        else if (!strcmp("-V", arg) || !_strcmpi("--vars", arg)) {
            m_show_func_vars = true;
        }
        else if (!_strcmpi("--test-header", arg)) {
            m_test_header = true;
        }
        else if (!strcmp("-t", arg) || !_strcmpi("--type", arg)) {
            if (i + 1 == endIndex) {
                LOG_ERROR("Missing value for param: {}!", arg);
                return false;
            }
            m_platform = opcode::PlatformOf(args[++i]);

            if (!m_platform) {
                LOG_ERROR("Unknown platform: {}!", args[i]);
                return false;
            }
        }
        else if (!strcmp("-f", arg) || !_strcmpi("--format", arg)) {
            if (i + 1 == endIndex) {
                LOG_ERROR("Missing value for param: {}!", arg);
                return false;
            }
            m_formatter = &formatter::GetFromName(args[++i]);

            if (_strcmpi(m_formatter->name, args[i])) {
                LOG_ERROR("Unknown formatter: {}! Use {}", args[i], m_formatter->name);
                return false;
            }
        }
        else if (!strcmp("-v", arg) || !_strcmpi("--vm", arg)) {
            if (i + 1 == endIndex) {
                LOG_ERROR("Missing value for param: {}!", arg);
                return false;
            }
            m_vm = opcode::VMOf(args[++i]);

            if (!m_vm) {
                LOG_ERROR("Unknown vm: {}!", args[i]);
                return false;
            }
        }
        else if (!_strcmpi("--internalblocks", arg)) {
            m_show_internal_blocks = true;
        }
        else if (!_strcmpi("--jumpdelta", arg)) {
            m_show_jump_delta = true;
        }
        else if (!_strcmpi("--prestruct", arg)) {
            m_show_pre_dump = true;
        }
        else if (!_strcmpi("--markjump", arg)) {
            m_mark_jump_type = true;
        }
        else if (!_strcmpi("--refcount", arg)) {
            m_show_ref_count = true;
        }
        else if (!_strcmpi("--displaystack", arg)) {
            m_display_stack = true;
        }
        else if (!strcmp("-i", arg) || !_strcmpi("--ignore", arg)) {
            if (i + 1 == endIndex) {
                LOG_ERROR("Missing value for param: {}!", arg);
                return false;
            }

            for (const char* param = args[++i]; *param; param++) {
                switch (*param) {
                case 'd':
                    m_stepskip |= STEPSKIP_DEV;
                    break;
                case 'D':
                    m_stepskip |= STEPSKIP_DEVBLOCK_INLINE;
                    break;
                case 's':
                    m_stepskip |= STEPSKIP_SWITCH;
                    break;
                case 'e':
                    m_stepskip |= STEPSKIP_FOREACH;
                    break;
                case 'w':
                    m_stepskip |= STEPSKIP_WHILE;
                    break;
                case 'i':
                    m_stepskip |= STEPSKIP_IF;
                    break;
                case 'f':
                    m_stepskip |= STEPSKIP_FOR;
                    break;
                case 'r':
                    m_stepskip |= STEPSKIP_RETURN;
                    break;
                case 'R':
                    m_stepskip |= STEPSKIP_BOOL_RETURN;
                    break;
                case 'c':
                    m_stepskip |= STEPSKIP_CLASSMEMBER_INLINE;
                    break;
                case 'S':
                    m_stepskip |= STEPSKIP_SPECIAL_PATTERN;
                    break;
                case 'a':
                    m_stepskip = ~0;
                    break;
                default:
                    LOG_ERROR("Bad param for {}: '{}'!", arg, *param);
                    return false;
                }
            }
        }
        else if (!strcmp("-o", arg) || !_strcmpi("--output", arg)) {
            if (i + 1 == endIndex) {
                LOG_ERROR("Missing value for param: {}!", arg);
                return false;
            }
            m_outputDir = args[++i];
        }
        else if (!strcmp("-m", arg) || !_strcmpi("--hashmap", arg)) {
            if (i + 1 == endIndex) {
                LOG_ERROR("Missing value for param: {}!", arg);
                return false;
            }
            m_dump_hashmap = args[++i];
        }
        else if (!_strcmpi("--dumpstrings", arg)) {
            if (i + 1 == endIndex) {
                LOG_ERROR("Missing value for param: {}!", arg);
                return false;
            }
            m_dump_strings = args[++i];
        }
        else if (!strcmp("-C", arg) || !_strcmpi("--copyright", arg)) {
            if (i + 1 == endIndex) {
                LOG_ERROR("Missing value for param: {}!", arg);
                return false;
            }
            m_copyright = args[++i];
        }
        else if (!strcmp("-r", arg) || !_strcmpi("--rosetta", arg)) {
            if (i + 1 == endIndex) {
                LOG_ERROR("Missing value for param: {}!", arg);
                return false;
            }
            m_rosetta = args[++i];
        }

        else if (*arg == '-') {
            LOG_ERROR("Unknown option: {}!", arg);
            return false;
        }
        else {
            m_inputFiles.push_back(arg);
        }
    }
    if (!m_inputFiles.size()) {
        m_inputFiles.push_back("scriptparsetree");
    }
    return true;
}

void GscInfoOption::PrintHelp() {
    LOG_INFO("-h --help          : Print help");
    LOG_INFO("-g --gsc           : Produce GSC");
    LOG_INFO("-a --asm           : Produce ASM");
    LOG_INFO("-t --type [t]      : Set type, default PC, values: 'ps', 'xbox', 'pc'");
    LOG_INFO("-o --output [d]    : ASM/GSC output dir, default same.gscasm");
    LOG_INFO("-v --vm            : Set vm, useless for Treyarch VM, values: mw23");
    LOG_INFO("-H --header        : Write file header");
    LOG_INFO("-m --hashmap [f]   : Write hashmap in a file f");
    {
        std::ostringstream formats;

        for (const auto* fmt : formatter::GetFormatters()) {

            formats << " '" << fmt->name << "'";
        }

        LOG_INFO("-f --format [f]    : Use formatter, values:{}", formats.str());
    }
    LOG_INFO("-l --rloc          : Write relative location of the function code");
    LOG_INFO("-C --copyright [t] : Set a comment text to put in front of every file");
    LOG_INFO("--dumpstrings [f]  : Dump strings in f");
    // it's not that I don't want them to be known, it's just to avoid having too many of them in the help
    // it's mostly dev tools
    LOG_DEBUG("-G --gvars         : Write gvars");
    LOG_DEBUG("-U --noincludes    : No includes");
    LOG_DEBUG("-X --exptests      : Enable UNK tests");
    LOG_DEBUG("-V --vars          : Show all func vars");
    LOG_DEBUG("-F --nofuncheader  : No function header");
    LOG_DEBUG("--nofunc           : No function write");
    LOG_DEBUG("-p --postfunchead  : Write post function header in ASM mode");
    LOG_DEBUG("-I --imports       : Write imports");
    LOG_DEBUG("-S --strings       : Write strings");
    LOG_DEBUG("-r --rosetta [f]   : Create Rosetta file");
    LOG_DEBUG("--test-header      : Write test header");
    LOG_DEBUG("--internalblocks   : Show internal blocks ");
    LOG_DEBUG("--jumpdelta        : Show jump delta");
    LOG_DEBUG("--prestruct        : Show prestruct");
    LOG_DEBUG("--refcount         : Show ref count");
    LOG_DEBUG("--markjump         : Show jump type");
    LOG_DEBUG("--displaystack     : Display stack in disassembly");
    LOG_DEBUG("-i --ignore[t + ]  : ignore step : ");
    LOG_DEBUG("                     a : all, d: devblocks, s : switch, e : foreach, w : while, i : if, f : for, r : return");
    LOG_DEBUG("                     R : bool return, c: class members, D: devblocks inline, S : special patterns");
}

static const char* gDumpStrings{};
static std::unordered_set<std::string> gDumpStringsStore{};
static const char* gRosettaOutput{};
static uint64_t gRosettaCurrent{};
static std::map<uint64_t, RosettaFileData> gRosettaBlocks{};

void tool::gsc::RosettaStartFile(GSCOBJReader& reader) {
    if (!gRosettaOutput) {
        return;
    }

    
    auto& block = gRosettaBlocks[gRosettaCurrent = reader.GetName()];
    // clone the header for the finder
    memcpy(&block.header, reader.Ptr(), reader.GetHeaderSize());
}

void tool::gsc::RosettaAddOpCode(uint32_t loc, uint16_t opcode) {
    if (!gRosettaOutput) {
        return;
    }

    auto& block = gRosettaBlocks[gRosettaCurrent].blocks;

    block.push_back(tool::gsc::RosettaOpCodeBlock{ .location = loc, .opcode = opcode });
}

GSCOBJReader::GSCOBJReader(byte* file, const GscInfoOption& opt) : file(file), opt(opt) {}

// by default no remapping
byte GSCOBJReader::RemapFlagsImport(byte flags) {
    return flags;
}
byte GSCOBJReader::RemapFlagsExport(byte flags) {
    return flags;
}

struct GSC_USEANIMTREE_ITEM {
    uint32_t num_address;
    uint32_t address;
};
struct GSC_ANIMTREE_ITEM {
    uint32_t num_address;
    uint32_t address_str1;
    uint32_t address_str2;
};

void GSCOBJReader::PatchCode(T8GSCOBJContext& ctx) {
    if (ctx.m_vmInfo->flags & opcode::VmFlags::VMF_HASH64) {
        if (GetAnimTreeSingleOffset()) {
            // HAS TO BE DONE FIRST BECAUSE THEY ARE STORED USING 1 byte
            uintptr_t unk2c_location = reinterpret_cast<uintptr_t>(file) + GetAnimTreeSingleOffset();
            auto anims_count = (int)GetAnimTreeSingleCount();
            for (size_t i = 0; i < anims_count; i++) {
                const auto* unk2c = reinterpret_cast<GSC_USEANIMTREE_ITEM*>(unk2c_location);

                auto* s = Ptr<char>(unk2c->address);

                uint32_t ref = ctx.AddStringValue(s);
                const auto* vars = reinterpret_cast<const uint32_t*>(&unk2c[1]);

                if (ref > 256) {
                    LOG_ERROR("Too many animtrees single usage");
                }
                else {
                    for (size_t j = 0; j < unk2c->num_address; j++) {
                        Ref(vars[j]) = (byte)ref;
                    }
                }
                unk2c_location += sizeof(*unk2c) + sizeof(*vars) * unk2c->num_address;
            }
        }

        uintptr_t str_location = reinterpret_cast<uintptr_t>(file) + GetStringsOffset();
        auto string_count = (int)GetStringsCount();
        for (size_t i = 0; i < string_count; i++) {

            const auto* str = reinterpret_cast<T8GSCString*>(str_location);
            char* cstr = DecryptString(Ptr<char>(str->string));
            if (gDumpStrings) {
                gDumpStringsStore.insert(cstr);
            }
            hashutils::Add(cstr, false, false);
            hashutils::Add(cstr, false, true);
            uint32_t ref = ctx.AddStringValue(cstr);

            const auto* strings = reinterpret_cast<const uint32_t*>(&str[1]);
            for (size_t j = 0; j < str->num_address; j++) {
                Ref<uint32_t>(strings[j]) = ref;
            }
            str_location += sizeof(*str) + sizeof(*strings) * str->num_address;
        }

        auto imports_count = (int)GetImportsCount();
        uintptr_t import_location = reinterpret_cast<uintptr_t>(file) + GetImportsOffset();
        for (size_t i = 0; i < imports_count; i++) {

            const auto* imp = reinterpret_cast<IW23GSCImport*>(import_location);

            const auto* imports = reinterpret_cast<const uint32_t*>(&imp[1]);
            for (size_t j = 0; j < imp->num_address; j++) {
                uint16_t* loc = Ptr<uint16_t>(imports[j]);
                auto idx = ctx.m_linkedImports.size();
                ctx.m_linkedImports.push_back(*imp);
                *loc = (uint16_t)idx;
            }
            import_location += sizeof(*imp) + sizeof(*imports) * imp->num_address;
        }

        if (GetAnimTreeDoubleOffset()) {
            uintptr_t animt_location = reinterpret_cast<uintptr_t>(file) + GetAnimTreeDoubleOffset();
            auto anims_count = (int)GetAnimTreeDoubleCount();
            for (size_t i = 0; i < anims_count; i++) {
                const auto* animt = reinterpret_cast<GSC_ANIMTREE_ITEM*>(animt_location);

                auto* s1 = Ptr<char>(animt->address_str1);
                auto* s2 = Ptr<char>(animt->address_str2);

                hashutils::Add(s1, true, true);
                hashutils::Add(s2, true, true);
                uint32_t ref1 = ctx.AddStringValue(s1);
                uint32_t ref2 = ctx.AddStringValue(s2);

                const auto* vars = reinterpret_cast<const uint32_t*>(&animt[1]);
                for (size_t j = 0; j < animt->num_address; j++) {
                    auto* loc = Ptr<uint32_t>(vars[j]);
                    // use strings to link them
                    loc[0] = ref1;
                    loc[1] = ref2;
                }
                animt_location += sizeof(*animt) + sizeof(*vars) * animt->num_address;
            }
        }

        return; // mwiii
    }
    // patching imports unlink the script refs to write namespace::import_name instead of the address
    auto imports_count = (int)GetImportsCount();
    uintptr_t import_location = reinterpret_cast<uintptr_t>(file) + GetImportsOffset();
    for (size_t i = 0; i < imports_count; i++) {

        const auto* imp = reinterpret_cast<T8GSCImport*>(import_location);

        const auto* imports = reinterpret_cast<const uint32_t*>(&imp[1]);
        for (size_t j = 0; j < imp->num_address; j++) {
            uint32_t* loc;
            auto remapedFlags = RemapFlagsImport(imp->flags);

            switch (remapedFlags & CALLTYPE_MASK) {
            case FUNC_METHOD:
                loc = PtrAlign<uint64_t, uint32_t>(imports[j] + 2ull);
                break;
            case FUNCTION:
            case FUNCTION_THREAD:
            case FUNCTION_CHILDTHREAD:
            case METHOD:
            case METHOD_THREAD:
            case METHOD_CHILDTHREAD:
                // here the game fix function calls with a bad number of params,
                // but for the decomp/dasm we don't care because we only mind about
                // what we'll find on the stack.
                Ref<byte>(imports[j] + 2ull) = imp->param_count;
                loc = PtrAlign<uint64_t, uint32_t>(imports[j] + 2ull + 1);
                break;
            default:
                loc = nullptr;
                break;
            }
            if (loc) {
                loc[0] = imp->name;

                if (remapedFlags & T8GSCImportFlags::GET_CALL) {
                    // no need for namespace if we are getting the call dynamically (api or inside-code script)
                    loc[1] = 0xc1243180; // ""
                }
                else {
                    loc[1] = imp->import_namespace;
                }

            }
        }
        import_location += sizeof(*imp) + sizeof(*imports) * imp->num_address;
    }

    uintptr_t gvars_location = reinterpret_cast<uintptr_t>(file) + GetGVarsOffset();
    auto globalvar_count = (int)GetGVarsCount();
    for (size_t i = 0; i < globalvar_count; i++) {
        const auto* globalvar = reinterpret_cast<T8GSCGlobalVar*>(gvars_location);
        uint16_t ref = ctx.AddGlobalVarName(globalvar->name);

        const auto* vars = reinterpret_cast<const uint32_t*>(&globalvar[1]);
        for (size_t j = 0; j < globalvar->num_address; j++) {
            // no align, no opcode to pass, directly the fucking location, cool.
            Ref<uint16_t>(vars[j]) = ref;
        }
        gvars_location += sizeof(*globalvar) + sizeof(*vars) * globalvar->num_address;
    }

    uintptr_t str_location = reinterpret_cast<uintptr_t>(file) + GetStringsOffset();
    auto string_count = (int)GetStringsCount();
    for (size_t i = 0; i < string_count; i++) {

        const auto* str = reinterpret_cast<T8GSCString*>(str_location);
        char* cstr = DecryptString(Ptr<char>(str->string));
        if (gDumpStrings) {
            gDumpStringsStore.insert(cstr);
        }
        uint32_t ref = ctx.AddStringValue(cstr);

        const auto* strings = reinterpret_cast<const uint32_t*>(&str[1]);
        for (size_t j = 0; j < str->num_address; j++) {
            // no align too....
            Ref<uint32_t>(strings[j]) = ref;
        }
        str_location += sizeof(*str) + sizeof(*strings) * str->num_address;
    }
}

void tool::gsc::GSCOBJReader::DumpExperimental(std::ostream& asmout, const GscInfoOption& opt) {
}

namespace {

    class T8GSCOBJReader : public GSCOBJReader {
        using GSCOBJReader::GSCOBJReader;

        void DumpHeader(std::ostream& asmout) override {
            auto* data = Ptr<T8GSCOBJ>();
            asmout
                << std::hex
                << "// crc: 0x" << std::hex << data->crc << "\n"
                << std::left << std::setfill(' ')
                << "// size ..... " << std::dec << std::setw(3) << data->script_size << "\n"
                << "// includes . " << std::dec << std::setw(3) << data->include_count << " (offset: 0x" << std::hex << data->include_offset << ")\n"
                << "// strings .. " << std::dec << std::setw(3) << data->string_count << " (offset: 0x" << std::hex << data->string_offset << ")\n"
                << "// exports .. " << std::dec << std::setw(3) << data->exports_count << " (offset: 0x" << std::hex << data->export_table_offset << ")\n"
                << "// imports .. " << std::dec << std::setw(3) << data->imports_count << " (offset: 0x" << std::hex << data->imports_offset << ")\n"
                << "// globals .. " << std::dec << std::setw(3) << data->globalvar_count << " (offset: 0x" << std::hex << data->globalvar_offset << ")\n"
                << "// fixups ... " << std::dec << std::setw(3) << data->fixup_count << " (offset: 0x" << std::hex << data->fixup_offset << ")\n"
                << "// cseg ..... 0x" << std::hex << data->cseg_offset << " + 0x" << std::hex << data->cseg_size << "\n"
                << std::right
                << std::flush;

            if (opt.m_test_header) {
                asmout
                    << "// ukn0c .... " << std::dec << data->pad << " / 0x" << std::hex << data->pad << "\n"
                    << "// ukn2c .... " << std::dec << data->ukn2c << " / 0x" << std::hex << data->ukn2c << "\n"
                    << "// ukn34 .... " << std::dec << data->ukn34 << " / 0x" << std::hex << data->ukn34 << "\n"
                    << "// ukn50 .... " << std::dec << data->ukn50 << " / 0x" << std::hex << data->ukn50 << "\n"
                    << "// ukn5a .... " << std::dec << (int)data->ukn5a << " / 0x" << std::hex << (int)data->ukn5a << "\n"
                    ;
            }
        }
        void DumpExperimental(std::ostream& asmout, const GscInfoOption& opt) override {
            auto* data = Ptr<T8GSCOBJ>();

            // no clue what this thing is doing
            uint64_t* requires_implements_table = reinterpret_cast<uint64_t*>(&data->magic[data->requires_implements_offset]);

            for (size_t i = 0; i < data->requires_implements_count; i++) {
                asmout << "#precache(\"requires_implements\" #\"" << hashutils::ExtractTmp("hash", requires_implements_table[i]) << "\");\n";
            }

            if (data->requires_implements_count) {
                asmout << "\n";
            }

            auto* fixups = reinterpret_cast<T8GSCFixup*>(&data->magic[data->fixup_offset]);

            for (size_t i = 0; i < data->fixup_count; i++) {
                const auto& fixup = fixups[i];
                asmout << std::hex << "#fixup 0x" << fixup.offset << " = 0x" << fixup.address << ";\n";
            }

            if (data->fixup_count) {
                asmout << "\n";
            }
        }

        uint64_t GetName() override {
            return Ptr<T8GSCOBJ>()->name;
        }
        uint16_t GetExportsCount() override {
            return Ptr<T8GSCOBJ>()->exports_count;
        }
        uint32_t GetExportsOffset() override {
            return Ptr<T8GSCOBJ>()->export_table_offset;
        }
        uint16_t GetIncludesCount() override {
            return Ptr<T8GSCOBJ>()->include_count;
        }
        uint32_t GetIncludesOffset() override {
            return Ptr<T8GSCOBJ>()->include_offset;
        }
        uint16_t GetImportsCount() override {
            return Ptr<T8GSCOBJ>()->imports_count;
        }
        uint32_t GetImportsOffset() override {
            return Ptr<T8GSCOBJ>()->imports_offset;
        }
        uint16_t GetGVarsCount() override {
            return Ptr<T8GSCOBJ>()->globalvar_count;
        }
        uint32_t GetGVarsOffset() override {
            return Ptr<T8GSCOBJ>()->globalvar_offset;
        }
        uint16_t GetStringsCount() override {
            return Ptr<T8GSCOBJ>()->string_count;
        }
        uint32_t GetStringsOffset() override {
            return Ptr<T8GSCOBJ>()->string_offset;
        }
        uint32_t GetFileSize() override {
            return Ptr<T8GSCOBJ>()->script_size;
        }
        size_t GetHeaderSize() override {
            return sizeof(T8GSCOBJ);
        }
        char* DecryptString(char* str) override {
            return decrypt::DecryptString(str);
        }
        bool IsValidHeader(size_t size) override {
            return size >= sizeof(T8GSCOBJ) && *reinterpret_cast<uint64_t*>(file) == 0x36000a0d43534780;
        }
        uint16_t GetAnimTreeSingleCount() override {
            return 0;
        };
        uint32_t GetAnimTreeSingleOffset() override {
            return 0;
        };
        uint16_t GetAnimTreeDoubleCount() override {
            return 0;
        };
        uint32_t GetAnimTreeDoubleOffset() override {
            return 0;
        };
    };
    
    class T937GSCOBJReader : public GSCOBJReader {
        using GSCOBJReader::GSCOBJReader;

        void DumpHeader(std::ostream& asmout) override {
            auto* data = Ptr<T937GSCOBJ>();
            asmout
                << std::hex
                << "// crc: 0x" << std::hex << data->crc << "\n"
                << std::left << std::setfill(' ')
                << "// size ..... " << std::dec << std::setw(3) << data->file_size << " (0x" << std::hex << data->file_size << ")" << "\n"
                << "// includes . " << std::dec << std::setw(3) << data->includes_count << " (offset: 0x" << std::hex << data->includes_table << ")\n"
                << "// strings .. " << std::dec << std::setw(3) << data->string_count << " (offset: 0x" << std::hex << data->string_offset << ")\n"
                << "// exports .. " << std::dec << std::setw(3) << data->export_count << " (offset: 0x" << std::hex << data->exports_tables << ")\n"
                << "// imports .. " << std::dec << std::setw(3) << data->imports_count << " (offset: 0x" << std::hex << data->imports_offset << ")\n"
                << "// globals .. " << std::dec << std::setw(3) << data->globalvar_count << " (offset: 0x" << std::hex << data->globalvar_offset << ")\n"
                << "// cseg ..... 0x" << std::hex << data->cseg_offset << " + 0x" << std::hex << data->cseg_size << "\n"
                << std::right
                << std::flush;

            if (opt.m_test_header) {
                asmout
                    << "// ukn0c .... " << std::dec << data->pad0c << " / 0x" << std::hex << data->pad0c << "\n"
                    << "// unk2c .... " << std::dec << data->unk2c << " / 0x" << std::hex << data->unk2c << "\n"
                    << "// unk3a .... " << std::dec << data->unk3a << " / 0x" << std::hex << data->unk3a << "\n"
                    << "// unk48 .... " << std::dec << data->unk48 << " / 0x" << std::hex << data->unk48 << "\n"
                    << "// unk52 .... " << std::dec << data->unk52 << " / 0x" << std::hex << data->unk52 << "\n"
                    << "// unk54 .... " << std::dec << data->unk54 << " / 0x" << std::hex << data->unk54 << "\n"
                    ;
            }
        }
        void DumpExperimental(std::ostream& asmout, const GscInfoOption& opt) override {
            auto* data = Ptr<T937GSCOBJ>();

            auto* fixups = reinterpret_cast<T8GSCFixup*>(&data->magic[data->fixup_offset]);

            for (size_t i = 0; i < data->fixup_count; i++) {
                const auto& fixup = fixups[i];
                asmout << std::hex << "#fixup 0x" << fixup.offset << " = 0x" << fixup.address << ";\n";
            }

            if (data->fixup_count) {
                asmout << "\n";
            }
        }

        uint64_t GetName() override {
            return Ptr<T937GSCOBJ>()->name;
        }
        uint16_t GetExportsCount() override {
            return Ptr<T937GSCOBJ>()->export_count;
        }
        uint32_t GetExportsOffset() override {
            return Ptr<T937GSCOBJ>()->exports_tables;
        }
        uint16_t GetIncludesCount() override {
            return Ptr<T937GSCOBJ>()->includes_count;
        }
        uint32_t GetIncludesOffset() override {
            return Ptr<T937GSCOBJ>()->includes_table;
        }
        uint16_t GetImportsCount() override {
            return Ptr<T937GSCOBJ>()->imports_count;
        }
        uint32_t GetImportsOffset() override {
            return Ptr<T937GSCOBJ>()->imports_offset;
        }
        uint16_t GetGVarsCount() override {
            return Ptr<T937GSCOBJ>()->globalvar_count;
        }
        uint32_t GetGVarsOffset() override {
            return Ptr<T937GSCOBJ>()->globalvar_offset;
        }
        uint16_t GetStringsCount() override {
            return Ptr<T937GSCOBJ>()->string_count;
        }
        uint32_t GetStringsOffset() override {
            return Ptr<T937GSCOBJ>()->string_offset;
        }
        uint32_t GetFileSize() override {
            return Ptr<T937GSCOBJ>()->file_size;
        }
        size_t GetHeaderSize() override {
            return sizeof(T937GSCOBJ);
        }
        char* DecryptString(char* str) override {
            return cw::DecryptString(str);
        }
        bool IsValidHeader(size_t size) override {
            return size >= sizeof(T937GSCOBJ) && *reinterpret_cast<uint64_t*>(file) == 0x37000a0d43534780;
        }

        byte RemapFlagsImport(byte flags) override {
            return flags; // same as bo4?
        }

        byte RemapFlagsExport(byte flags) override {
            return flags;
        }
        uint16_t GetAnimTreeSingleCount() override {
            return 0;
        };
        uint32_t GetAnimTreeSingleOffset() override {
            return 0;
        };
        uint16_t GetAnimTreeDoubleCount() override {
            return 0;
        };
        uint32_t GetAnimTreeDoubleOffset() override {
            return 0;
        };
    };

    class T9GSCOBJReader : public GSCOBJReader {
        using GSCOBJReader::GSCOBJReader;

        void DumpHeader(std::ostream& asmout) override {
            auto* data = Ptr<T9GSCOBJ>();
            asmout
                << std::hex
                << "// crc: 0x" << std::hex << data->crc << "\n"
                << std::left << std::setfill(' ')
                << "// size ..... " << std::dec << std::setw(3) << data->file_size << " (0x" << std::hex << data->file_size << ")" << "\n"
                << "// includes . " << std::dec << std::setw(3) << data->includes_count << " (offset: 0x" << std::hex << data->includes_table << ")\n"
                << "// strings .. " << std::dec << std::setw(3) << data->string_count << " (offset: 0x" << std::hex << data->string_offset << ")\n"
                << "// exports .. " << std::dec << std::setw(3) << data->exports_count << " (offset: 0x" << std::hex << data->exports_tables << ")\n"
                << "// imports .. " << std::dec << std::setw(3) << data->imports_count << " (offset: 0x" << std::hex << data->import_tables << ")\n"
                << "// globals .. " << std::dec << std::setw(3) << data->globalvar_count << " (offset: 0x" << std::hex << data->globalvar_offset << ")\n"
                << "// cseg ..... 0x" << std::hex << data->cseg_offset << " + 0x" << std::hex << data->cseg_size << "\n"
                << std::right
                << std::flush;

            if (opt.m_test_header) {
                asmout
                    << "// ukn0c .... " << std::dec << data->pad0c << " / 0x" << std::hex << data->pad0c << "\n"
                    << "// unk1e .... " << std::dec << data->unk1e << " / 0x" << std::hex << data->unk1e << "\n"
                    << "// unk22 .... " << std::dec << data->unk22 << " / 0x" << std::hex << data->unk22 << "\n"
                    << "// unk26 .... " << std::dec << data->unk26 << " / 0x" << std::hex << data->unk26 << "\n"
                    << "// unk28 .... " << std::dec << data->unk28 << " / 0x" << std::hex << data->unk28 << "\n" // offset
                    << "// unk40 .... " << std::dec << data->unk40 << " / 0x" << std::hex << data->unk40 << "\n" // offset
                    << "// unk4c .... " << std::dec << data->unk4c << " / 0x" << std::hex << data->unk4c << "\n"
                    << "// unk54 .... " << std::dec << data->unk54 << " / 0x" << std::hex << data->unk54 << "\n"
                    ;
            }
        }
        void DumpExperimental(std::ostream& asmout, const GscInfoOption& opt) override {
        }

        uint64_t GetName() override {
            return Ptr<T9GSCOBJ>()->name;
        }
        uint16_t GetExportsCount() override {
            return Ptr<T9GSCOBJ>()->exports_count;
        }
        uint32_t GetExportsOffset() override {
            return Ptr<T9GSCOBJ>()->exports_tables;
        }
        uint16_t GetIncludesCount() override {
            return Ptr<T9GSCOBJ>()->includes_count;
        }
        uint32_t GetIncludesOffset() override {
            return Ptr<T9GSCOBJ>()->includes_table;
        }
        uint16_t GetImportsCount() override {
            return Ptr<T9GSCOBJ>()->imports_count;
        }
        uint32_t GetImportsOffset() override {
            return Ptr<T9GSCOBJ>()->import_tables;
        }
        uint16_t GetGVarsCount() override {
            return Ptr<T9GSCOBJ>()->globalvar_count;
        }
        uint32_t GetGVarsOffset() override {
            return Ptr<T9GSCOBJ>()->globalvar_offset;
        }
        uint16_t GetStringsCount() override {
            return Ptr<T9GSCOBJ>()->string_count;
        }
        uint32_t GetStringsOffset() override {
            return Ptr<T9GSCOBJ>()->string_offset;
        }
        uint32_t GetFileSize() override {
            return Ptr<T9GSCOBJ>()->file_size;
        }
        size_t GetHeaderSize() override {
            return sizeof(T9GSCOBJ);
        }
        char* DecryptString(char* str) override {
            return cw::DecryptString(str);
        }
        bool IsValidHeader(size_t size) override {
            return size >= sizeof(T9GSCOBJ) && *reinterpret_cast<uint64_t*>(file) == 0x38000a0d43534780;
        }
        byte RemapFlagsImport(byte flags) override {
            byte nflags = 0;

            switch (flags & T9_IF_CALLTYPE_MASK) {
            case T9_IF_METHOD_CHILDTHREAD: nflags |= METHOD_CHILDTHREAD; break;
            case T9_IF_METHOD_THREAD: nflags |= METHOD_THREAD; break;
            case T9_IF_FUNCTION_CHILDTHREAD: nflags |= FUNCTION_CHILDTHREAD; break;
            case T9_IF_FUNCTION: nflags |= FUNCTION; break;
            case T9_IF_FUNC_METHOD: nflags |= FUNC_METHOD; break;
            case T9_IF_FUNCTION_THREAD: nflags |= FUNCTION_THREAD; break;
            case T9_IF_METHOD: nflags |= METHOD; break;
            default: nflags |= flags & 0xF; // wtf?
            }

            nflags |= flags & ~T9_IF_CALLTYPE_MASK;

            return nflags;
        }

        byte RemapFlagsExport(byte flags) override {
            if (flags == T9_EF_CLASS_VTABLE) {
                return CLASS_VTABLE;
            }
            byte nflags = 0;

            if (flags & T9_EF_AUTOEXEC) nflags |= AUTOEXEC;
            if (flags & T9_EF_LINKED) nflags |= LINKED;
            if (flags & T9_EF_PRIVATE) nflags |= PRIVATE;
            if (flags & T9_EF_CLASS_MEMBER) nflags |= CLASS_MEMBER;
            if (flags & T9_EF_EVENT) nflags |= EVENT;
            if (flags & T9_EF_VE) nflags |= VE;
            if (flags & T9_EF_CLASS_LINKED) nflags |= CLASS_LINKED;
            if (flags & T9_EF_CLASS_DESTRUCTOR) nflags |= CLASS_DESTRUCTOR;

            return nflags;
        }
        uint16_t GetAnimTreeSingleCount() override {
            return 0;
        };
        uint32_t GetAnimTreeSingleOffset() override {
            return 0;
        };
        uint16_t GetAnimTreeDoubleCount() override {
            return 0;
        };
        uint32_t GetAnimTreeDoubleOffset() override {
            return 0;
        };
    };

    class MW23GSCOBJReader : public GSCOBJReader {
        using GSCOBJReader::GSCOBJReader;

        void DumpHeader(std::ostream& asmout) override {
            auto* data = Ptr<GscObj23>();
            asmout
                << std::left << std::setfill(' ')
                << "// size ...... " << std::dec << std::setw(3) << data->size1 << " (0x" << std::hex << data->size1 << ")" << "\n"
                << "// includes .. " << std::dec << std::setw(3) << data->includes_count << " (offset: 0x" << std::hex << data->include_table << ")\n"
                << "// strings ... " << std::dec << std::setw(3) << data->string_count << " (offset: 0x" << std::hex << data->string_table << ")\n"
                << "// exports ... " << std::dec << std::setw(3) << data->export_count << " (offset: 0x" << std::hex << data->export_offset << ")\n"
                << "// imports ... " << std::dec << std::setw(3) << data->imports_count << " (offset: 0x" << std::hex << data->import_table << ")\n"
                << "// animtree1 . " << std::dec << std::setw(3) << data->animtree_use_count << " (offset: 0x" << std::hex << data->animtree_use_offset << ")\n"
                << "// animtree2 . " << std::dec << std::setw(3) << data->animtree_count << " (offset: 0x" << std::hex << data->animtree_offset << ")\n"
                //<< "// globals .. " << std::dec << std::setw(3) << data->globalvar_count << " (offset: 0x" << std::hex << data->globalvar_offset << ")\n"
                << "// cseg ..... 0x" << std::hex << data->cseg_offset << " + 0x" << std::hex << data->cseg_size << "\n"
                << std::right
                << std::flush;

            if (opt.m_test_header) {
                // fillme
                asmout
                    << "unk16 :" << std::dec << std::setw(3) << (int)data->unk16 << " (0x" << std::hex << data->unk16 << ")\n"
                    << "unk1C :" << std::dec << std::setw(3) << (int)data->unk1C << " (0x" << std::hex << data->unk1C << ")\n"
                    << "unk22 :" << std::dec << std::setw(3) << (int)data->unk22 << " (0x" << std::hex << data->unk22 << ")\n"
                    << "unk26 :" << std::dec << std::setw(3) << (int)data->unk26 << " (0x" << std::hex << data->unk26 << ")\n"
                    << "unk28 :" << std::dec << std::setw(3) << (int)data->unk28 << " (0x" << std::hex << data->unk28 << ")\n"
                    << "unk2A :" << std::dec << std::setw(3) << (int)data->unk2A << " (0x" << std::hex << data->unk2A << ")\n"
                    << "unk3C :" << std::dec << std::setw(3) << (int)data->unk3C << " (0x" << std::hex << data->unk3C << ")\n"
                    << "unk48 :" << std::dec << std::setw(3) << (int)data->size1 << " (0x" << std::hex << data->size1 << ")\n"
                    << "unk54 :" << std::dec << std::setw(3) << (int)data->size2 << " (0x" << std::hex << data->size2 << ")\n"
                    << "unk5C :" << std::dec << std::setw(3) << (int)data->unk5C << " (0x" << std::hex << data->unk5C << ")\n"
                    ;
            }
        }
        void DumpExperimental(std::ostream& asmout, const GscInfoOption& opt) override {
            auto* data = Ptr<GscObj23>();

            if (opt.m_test_header) {
                uintptr_t unk2c_location = reinterpret_cast<uintptr_t>(data->magic) + data->animtree_use_offset;
                for (size_t i = 0; i < data->animtree_use_count; i++) {
                    const auto* unk2c = reinterpret_cast<GSC_USEANIMTREE_ITEM*>(unk2c_location);

                    auto* s = Ptr<char>(unk2c->address);

                    asmout << std::hex << "animtree #" << s << std::endl;

                    hashutils::Add(s, true, true);

                    const auto* vars = reinterpret_cast<const uint32_t*>(&unk2c[1]);
                    asmout << "location(s): ";
                    for (size_t j = 0; j < unk2c->num_address; j++) {
                        // no align, no opcode to pass, directly the fucking location, cool.
                        //Ref<uint16_t>(vars[j]) = ref;
                        if (j) asmout << ",";
                        asmout << std::hex << vars[j];
                    }
                    asmout << "\n";
                    unk2c_location += sizeof(*unk2c) + sizeof(*vars) * unk2c->num_address;
                }
                if (data->animtree_use_count) {
                    asmout << "\n";
                }

                uintptr_t animt_location = reinterpret_cast<uintptr_t>(data->magic) + data->animtree_offset;
                for (size_t i = 0; i < data->animtree_count; i++) {
                    const auto* animt = reinterpret_cast<GSC_ANIMTREE_ITEM*>(animt_location);

                    auto* s1 = Ptr<char>(animt->address_str1);
                    auto* s2 = Ptr<char>(animt->address_str2);

                    hashutils::Add(s1, true, true);
                    hashutils::Add(s2, true, true);

                    asmout << std::hex << "animtree " << s1 << "%" << s2 << std::endl;

                    const auto* vars = reinterpret_cast<const uint32_t*>(&animt[1]);
                    asmout << "location(s): ";
                    for (size_t j = 0; j < animt->num_address; j++) {
                        // no align, no opcode to pass, directly the fucking location, cool.
                        //Ref<uint16_t>(vars[j]) = ref;
                        if (j) asmout << ",";
                        asmout << std::hex << vars[j];
                    }
                    asmout << "\n";
                    animt_location += sizeof(*animt) + sizeof(*vars) * animt->num_address;
                }
                if (data->animtree_count) {
                    asmout << "\n";
                }
            }
        }

        uint64_t GetName() override {
            return Ptr<GscObj23>()->name;
        }
        uint16_t GetExportsCount() override {
            return Ptr<GscObj23>()->export_count;
        }
        uint32_t GetExportsOffset() override {
            return Ptr<GscObj23>()->export_offset;
        }
        uint16_t GetIncludesCount() override {
            return Ptr<GscObj23>()->includes_count;
        }
        uint32_t GetIncludesOffset() override {
            return Ptr<GscObj23>()->include_table;
        }
        uint16_t GetImportsCount() override {
            return Ptr<GscObj23>()->imports_count;
        }
        uint32_t GetImportsOffset() override {
            return Ptr<GscObj23>()->import_table;
        }
        uint16_t GetGVarsCount() override {
            return 0; //return Ptr<GscObj23>()->globalvar_count;
        }
        uint32_t GetGVarsOffset() override {
            return 0; //return Ptr<GscObj23>()->globalvar_offset;
        }
        uint16_t GetStringsCount() override {
            return Ptr<GscObj23>()->string_count;
        }
        uint32_t GetStringsOffset() override {
            return Ptr<GscObj23>()->string_table;
        }
        uint32_t GetFileSize() override {
            return Ptr<GscObj23>()->size1;
        }
        size_t GetHeaderSize() override {
            return sizeof(GscObj23);
        }
        char* DecryptString(char* str) override {
            return str; // iw
        }
        bool IsValidHeader(size_t size) override {
            return size >= sizeof(GscObj23) && *reinterpret_cast<uint64_t*>(file) == 0xa0d4353478a;
        }
        byte RemapFlagsImport(byte flags) override {
            byte nflags = 0;

            switch (flags & 0xF) {
            case 5: nflags |= FUNC_METHOD; break;
            case 4: nflags |= FUNCTION; break;
            case 2: nflags |= FUNCTION_THREAD; break;
            case 1: nflags |= FUNCTION_CHILDTHREAD; break;

            case 3:
            case 6:
            case 7: nflags |= FUNCTION; break; // TODO: unk script calls
            case 8:
            case 0xA:nflags |= FUNCTION; break; // api call
            case 9:
            case 0xB:nflags |= METHOD; break; // api call
            default: nflags |= flags & 0xF; // wtf?
            }

            // 0x10: Dev import
            // 0x20: use file namespace
            nflags |= flags & ~0xF;

            return nflags;
        }

        byte RemapFlagsExport(byte flags) override {
            byte nflags{};
            if (flags & 1) {
                nflags |= T8GSCExportFlags::AUTOEXEC;
            }
            if (flags & 2) {
                nflags |= T8GSCExportFlags::LINKED;
            }
            if (flags & 4) {
                nflags |= T8GSCExportFlags::PRIVATE;
            }

            return nflags;
        }
        uint16_t GetAnimTreeSingleCount() override {
            return Ptr<GscObj23>()->animtree_use_count;
        };
        uint32_t GetAnimTreeSingleOffset() override {
            return Ptr<GscObj23>()->animtree_use_offset;
        };
        uint16_t GetAnimTreeDoubleCount() override {
            return Ptr<GscObj23>()->animtree_count;
        };
        uint32_t GetAnimTreeDoubleOffset() override {
            return Ptr<GscObj23>()->animtree_offset;
        };
    };

    std::unordered_map<byte, std::function<std::shared_ptr<GSCOBJReader> (byte*, const GscInfoOption&)>> gscReaders = {
        { tool::gsc::opcode::VM_T8,[](byte* file, const GscInfoOption& opt) { return std::make_shared<T8GSCOBJReader>(file, opt); }},
        { tool::gsc::opcode::VM_T937,[](byte* file, const GscInfoOption& opt) { return std::make_shared<T937GSCOBJReader>(file, opt); }},
        { tool::gsc::opcode::VM_T9,[](byte* file, const GscInfoOption& opt) { return std::make_shared<T9GSCOBJReader>(file, opt); }},
        { tool::gsc::opcode::VM_MW23,[](byte* file, const GscInfoOption& opt) { return std::make_shared<MW23GSCOBJReader>(file, opt); }},
    };
}


struct H32GSCExportReader : GSCExportReader {
    T8GSCExport* exp{};

    void SetHandle(void* handle) override { exp = (T8GSCExport*)handle; };
    uint64_t GetName() override { return exp->name; };
    uint64_t GetNamespace() override { return exp->name_space; };
    uint64_t GetFileNamespace() override { return exp->callback_event; };
    uint64_t GetChecksum() override { return exp->checksum; };
    uint32_t GetAddress() override { return exp->address; };
    uint8_t GetParamCount() override { return exp->param_count; };
    uint8_t GetFlags() override { return exp->flags; };
    size_t SizeOf() override { return sizeof(*exp); };
};
struct H64GSCExportReader : GSCExportReader {
    IW23GSCExport* exp{};

    void SetHandle(void* handle) override { exp = (IW23GSCExport*)handle; };
    uint64_t GetName() override { return exp->name; };
    uint64_t GetNamespace() override { return exp->name_space; };
    uint64_t GetFileNamespace() override { return exp->file_name_space; };
    uint64_t GetChecksum() override { return exp->checksum; };
    uint32_t GetAddress() override { return exp->address; };
    uint8_t GetParamCount() override { return exp->param_count; };
    uint8_t GetFlags() override { return exp->flags; };
    size_t SizeOf() override { return sizeof(*exp); };
};

int GscInfoHandleData(byte* data, size_t size, const char* path, const GscInfoOption& opt) {

    auto& profiler = actscli::GetProfiler();
    actslib::profiler::ProfiledSection ps{ profiler, path };


    T8GSCOBJContext ctx{};
    auto& gsicInfo = ctx.m_gsicInfo;

    gsicInfo.isGsic = size > 4 && !memcmp(data, "GSIC", 4);
    if (gsicInfo.isGsic) {
        actslib::profiler::ProfiledSection ps{ profiler, "gsic reading"};
        LOG_DEBUG("Reading GSIC Compiled Script data");

        size_t gsicSize = 4; // preamble

        auto numFields = *reinterpret_cast<int32_t*>(&data[gsicSize]);
        gsicSize += 4;

        bool gsicError = false;
        for (size_t i = 0; i < numFields; i++) {
            auto fieldType = *reinterpret_cast<int32_t*>(&data[gsicSize]);
            gsicSize += 4;
            switch (fieldType) {
            case 0: // Detour
            {
                auto detourCount = *reinterpret_cast<int32_t*>(&data[gsicSize]);
                gsicSize += 4;
                for (size_t j = 0; j < detourCount; j++) {
                    GsicDetour* detour = reinterpret_cast<GsicDetour*>(&data[gsicSize]);
                    gsicSize += (size_t)(28 + 256 - 1 - (5 * 4) + 1 - 8);

                    // register detour
                    gsicInfo.detours[detour->fixupOffset] = detour;
                }
            }
            break;
            default:
                LOG_ERROR("Bad GSIC field type: {}", fieldType);
                gsicError = true;
                break;
            }
            if (gsicError) {
                break;
            }
        }

        if (gsicError) {
            return tool::BASIC_ERROR;
        }

        // pass the GSIC data
        gsicInfo.headerSize = gsicSize;
        data += gsicSize;
    }
    byte vm;
    bool iw;

    auto magicVal = *reinterpret_cast<uint64_t*>(data) & ~0xFF00000000000000;
    if (magicVal == 0xa0d4353478a) {
        // IW GSC file, use user input
        if (opt.m_vm == opcode::VM_UNKNOWN) {
            LOG_ERROR("VM type needed with IW GSC file, please use --vm [vm] to set it");
            return tool::BASIC_ERROR;
        }
        vm = opt.m_vm;
        iw = true;
    } 
    else if (magicVal == 0xa0d43534780) {
        // Treyarch GSC file, use revision
        vm = data[7];
        iw = false;
    }
    else {
        LOG_ERROR("Bad magic 0x{:x}", *reinterpret_cast<uint64_t*>(data));
        return tool::BASIC_ERROR;
    }
    hashutils::ReadDefaultFile();


    opcode::VmInfo* vmInfo;
    if (!opcode::IsValidVm(vm, vmInfo)) {
        LOG_ERROR("Bad vm 0x{:x} for file {}", (int)vm, path);
        return -1;
    }
    ctx.m_vmInfo = vmInfo;

    auto readerBuilder = gscReaders.find(vm);

    if (readerBuilder == gscReaders.end()) {
        LOG_ERROR("No reader available for vm 0x{:x} for file {}", (int)vm, path);
        return -1;
    }

    std::shared_ptr<GSCOBJReader> scriptfile = readerBuilder->second(data, opt);

    // we keep it because it should also check the size
    if (!scriptfile->IsValidHeader(size)) {
        LOG_ERROR("Bad header 0x{:x} for file {}", scriptfile->Ref<uint64_t>(), path);
        return -1;
    }

    tool::gsc::RosettaStartFile(*scriptfile);

    char asmfnamebuff[1000];

    if (opt.m_outputDir) {
        const char* name = hashutils::ExtractPtr(scriptfile->GetName());
        if (!name) {
            sprintf_s(asmfnamebuff, "%s/hashed/script/script_%llx.gsc", opt.m_outputDir, scriptfile->GetName());
        }
        else {
            sprintf_s(asmfnamebuff, "%s/%s", opt.m_outputDir, name);
        }
    }
    else {
        sprintf_s(asmfnamebuff, "%sasm", path);
    }
    profiler.GetCurrent().name = asmfnamebuff;

    std::filesystem::path file(asmfnamebuff);

    std::filesystem::create_directories(file.parent_path());

    std::ofstream asmout{ file };

    if (!asmout) {
        LOG_ERROR("Can't open output file {}", asmfnamebuff);
        return -1;
    }
    LOG_INFO("Decompiling into '{}' {}...", asmfnamebuff, (gsicInfo.isGsic ? " (GSIC)" : ""));
    if (opt.m_copyright) {
        asmout << "// " << opt.m_copyright << "\n";
    }

    if (opt.m_header) {
        asmout
            << "// " << hashutils::ExtractTmpScript(scriptfile->GetName()) << " (" << path << ")" << " (size: " << size << " Bytes / " << std::hex << "0x" << size << ")\n";

        if (gsicInfo.isGsic) {
            asmout
                << "// GSIC Compiled script" << ", header: 0x" << std::hex << gsicInfo.headerSize << "\n"
                << "// detours: " << std::dec << gsicInfo.detours.size() << "\n";
            for (const auto& [key, detour] : gsicInfo.detours) {
                asmout << "// - ";

                if (detour->replaceNamespace) {
                    asmout << hashutils::ExtractTmp("namespace", detour->replaceNamespace) << std::flush;
                }
                auto replaceScript = *reinterpret_cast<uint64_t*>(&detour->replaceScriptTop);
                if (replaceScript) {
                    asmout << "<" << hashutils::ExtractTmpScript(replaceScript) << ">" << std::flush;
                }

                if (detour->replaceNamespace) {
                    asmout << "::";
                }

                asmout
                    << hashutils::ExtractTmp("function", detour->replaceFunction) << std::flush
                    << " offset: 0x" << std::hex << detour->fixupOffset << ", size: 0x" << detour->fixupSize << "\n";
            }
        }

        asmout
            << "// magic .... 0x" << scriptfile->Ref<uint64_t>()
            << " vm: ";
        
        if (vmInfo->flags & opcode::VmFlags::VMF_NO_VERSION) {
            asmout << vmInfo->name;
        }
        else {
            asmout << (uint32_t)vmInfo->vm << " (" << vmInfo->name << ")";
        }
        asmout << "\n";

        scriptfile->DumpHeader(asmout);
    }

    // write the strings before the patch to avoid reading pre-decrypted strings
    if (opt.m_strings && scriptfile->GetStringsOffset()) {
        uintptr_t str_location = reinterpret_cast<uintptr_t>(scriptfile->Ptr(scriptfile->GetStringsOffset()));

        for (size_t i = 0; i < scriptfile->GetStringsCount(); i++) {

            const auto* str = reinterpret_cast<T8GSCString*>(str_location);

            asmout << std::hex << "String addr:" << str->string << ", count:" << (int)str->num_address << ", type:" << (int)str->type << std::endl;

            char* encryptedString = scriptfile->Ptr<char>(str->string);

            size_t len{};
            byte type{};
            if (scriptfile->GetVM() == opcode::VM_T8) {
                len = (size_t)reinterpret_cast<byte*>(encryptedString)[1] - 1;
                type = *reinterpret_cast<byte*>(encryptedString);

                if (str->string + len + 1 > scriptfile->GetFileSize()) {
                    asmout << "bad string location\n";
                    break;
                }

                asmout << "encryption: ";
                asmout << "0x" << std::hex << (int)type;
                if ((type & 0xC0) == 0x80) {
                    asmout << "(none)";
                }
                asmout << " len: " << std::dec << len << " -> " << std::flush;

            }
            else {
                auto* ess = reinterpret_cast<byte*>(encryptedString);
                type = ess[0];
                len = (size_t)ess[2] - 1;

                if (str->string + len + 3 > scriptfile->GetFileSize()) {
                    asmout << "bad string location\n";
                    break;
                }

                asmout << "encryption: ";
                asmout << "0x" << std::hex << (int)type;
                if ((type & 0xC0) == 0x80) {
                    asmout << "(none)";
                }
                asmout << " len: " << std::dec << len << ", unk1: 0x" << std::hex << (int)ess[1] << " -> " << std::flush;
            }
            char* cstr = scriptfile->DecryptString(encryptedString);

            asmout << '"' << cstr << "\"" << std::flush;

            if (scriptfile->GetVM() == opcode::VM_T8) {
                size_t lenAfterDecrypt = strnlen_s(cstr, len + 2);

                if (lenAfterDecrypt != len) {
                    asmout << " ERROR LEN (" << std::dec << lenAfterDecrypt << " != " << len << " for type 0x" << std::hex << (int)type << ")";
                    assert(false);
                }
            }

            asmout << "\n";

            asmout << "location(s): ";

            const auto* strings = reinterpret_cast<const uint32_t*>(&str[1]);
            asmout << std::hex << strings[0];
            for (size_t j = 1; j < str->num_address; j++) {
                asmout << std::hex << "," << strings[j];
            }
            asmout << "\n";
            str_location += sizeof(*str) + sizeof(*strings) * str->num_address;
        }
        if (scriptfile->GetStringsCount()) {
            asmout << "\n";
        }
    }

    if (opt.m_patch) {
        actslib::profiler::ProfiledSection ps{ profiler, "patch linking"};
        // unlink the script and write custom gvar/string ids
        scriptfile->PatchCode(ctx);
    }

    if (opt.m_includes && scriptfile->GetIncludesOffset()) {
        auto* includes = scriptfile->Ptr<uint64_t>(scriptfile->GetIncludesOffset());

        for (size_t i = 0; i < scriptfile->GetIncludesCount(); i++) {
            asmout << "#using " << hashutils::ExtractTmpScript(includes[i]) << ";\n";
        }
        if (scriptfile->GetIncludesCount()) {
            asmout << "\n";
        }
    }

    scriptfile->DumpExperimental(asmout, opt);

    if (opt.m_gvars && scriptfile->GetGVarsOffset()) {
        uintptr_t gvars_location = reinterpret_cast<uintptr_t>(scriptfile->Ptr(scriptfile->GetGVarsOffset()));

        for (size_t i = 0; i < scriptfile->GetGVarsCount(); i++) {
            const auto* globalvar = reinterpret_cast<T8GSCGlobalVar*>(gvars_location);
            asmout << std::hex << "Global var " << hashutils::ExtractTmp("var", globalvar->name) << " " << globalvar->num_address << "\n";

            asmout << "location(s): ";

            const auto* vars = reinterpret_cast<const uint32_t*>(&globalvar[1]);
            asmout << std::hex << vars[0];
            for (size_t j = 1; j < globalvar->num_address; j++) {
                asmout << std::hex << "," << vars[j];
            }
            asmout << "\n";
            gvars_location += sizeof(*globalvar) + sizeof(*vars) * globalvar->num_address;
        }
        if (scriptfile->GetGVarsCount()) {
            asmout << "\n";
        }
    }

    if (opt.m_imports) {
        uintptr_t import_location = reinterpret_cast<uintptr_t>(scriptfile->Ptr(scriptfile->GetImportsOffset()));

        for (size_t i = 0; i < scriptfile->GetImportsCount(); i++) {
            uint64_t name_space;
            uint64_t name;
            size_t impSize;
            byte flags;
            byte param_count;
            uint16_t numAddress;

            if (ctx.m_vmInfo->flags & opcode::VmFlags::VMF_HASH64) {
                const auto* imp = reinterpret_cast<IW23GSCImport*>(import_location);
                name_space = imp->name_space;
                name = imp->name;
                flags = imp->flags;
                numAddress = imp->num_address;
                param_count = imp->param_count;
                impSize = sizeof(*imp);
            }
            else {
                const auto* imp = reinterpret_cast<T8GSCImport*>(import_location);
                name_space = imp->import_namespace;
                name = imp->name;
                flags = imp->flags;
                param_count = imp->param_count;
                numAddress = imp->num_address;
                impSize = sizeof(*imp);
            }

            asmout << std::hex << "import ";

            auto remapedFlags = scriptfile->RemapFlagsImport(flags);

            switch (remapedFlags & T8GSCImportFlags::CALLTYPE_MASK) {
            case FUNC_METHOD: asmout << "funcmethod "; break;
            case FUNCTION: asmout << "function "; break;
            case FUNCTION_THREAD: asmout << "function thread "; break;
            case FUNCTION_CHILDTHREAD: asmout << "function childthread "; break;
            case METHOD: asmout << "method "; break;
            case METHOD_THREAD: asmout << "method thread "; break;
            case METHOD_CHILDTHREAD: asmout << "method childthread "; break;
            default:
                asmout << "<errorflag:" << std::hex << (remapedFlags & 0xF) << "> ";
                break;
            }

            if (remapedFlags & T8GSCImportFlags::DEV_CALL) {
                asmout << "devcall ";
            }

            // they both seem unused
            if (remapedFlags & T8GSCImportFlags::UKN40) {
                asmout << "ukn40 ";
            }
            if (remapedFlags & T8GSCImportFlags::UKN80) {
                asmout << "ukn80 ";
            }

            if ((remapedFlags & T8GSCImportFlags::GET_CALL) == 0) {
                // no need for namespace if we are getting the call dynamically (api or inside-code script)
                asmout << hashutils::ExtractTmp("namespace", name_space) << std::flush << "::";
            }

            asmout << std::hex << hashutils::ExtractTmp("function", name) << "\n";

            asmout << std::hex << "address: " << numAddress
                << ", params: " << (int)param_count
                << ", iflags: 0x" << std::hex << (uint16_t)(flags)
                << ", loc: 0x" << std::hex << (import_location - reinterpret_cast<uintptr_t>(scriptfile->Ptr()))
                << "\n";

            asmout << "location(s): ";

            const auto* imports = reinterpret_cast<const uint32_t*>(import_location + impSize);
            asmout << std::hex << imports[0];
            for (size_t j = 1; j < numAddress; j++) {
                asmout << std::hex << "," << imports[j];
            }
            asmout << "\n";

            asmout << "--------------\n";

            import_location += impSize + sizeof(*imports) * numAddress;
        }
        if (scriptfile->GetImportsCount()) {
            asmout << "\n";
        }
    }

    if (opt.m_func) {
        actslib::profiler::ProfiledSection ps{ profiler, "decompiling" };
        // current namespace
        uint64_t currentNSP = 0;

        struct Located {
            uint64_t name_space;
            uint64_t name;
        };
        struct LocatedHash {
            size_t operator()(const Located& k) const {
                return k.name_space ^ RotateLeft64(k.name, 32);
            }
        };
        struct LocatedEquals {
            bool operator()(const Located& a, const Located& b) const {
                return a.name == b.name && a.name_space == b.name_space;
            }
        };

        std::unordered_map<Located, opcode::ASMContext, LocatedHash, LocatedEquals> contextes{};

        std::unique_ptr<GSCExportReader> exp;
        if (ctx.m_vmInfo->flags & opcode::VmFlags::VMF_HASH64) {
            exp = std::make_unique<H64GSCExportReader>();
        }
        else {
            exp = std::make_unique<H32GSCExportReader>();
        }

        for (size_t i = 0; i < scriptfile->GetExportsCount(); i++) {
            void* handle = scriptfile->Ptr(scriptfile->GetExportsOffset()) + i * exp->SizeOf();
            exp->SetHandle(handle);

            std::ofstream nullstream;
            nullstream.setstate(std::ios_base::badbit);

            // if we aren't dumping the ASM, we compute all the nodes first
            std::ostream& output = opt.m_dasm ? asmout : nullstream;

            if (exp->GetNamespace() != currentNSP) {
                currentNSP = exp->GetNamespace();

                if (opt.m_dasm) {
                    output << "#namespace " << hashutils::ExtractTmp("namespace", currentNSP) << ";\n" << std::endl;
                }
            }

            Located rname = { exp->GetNamespace(), exp->GetName() };

            auto r = contextes.try_emplace(rname, scriptfile->Ptr(exp->GetAddress()), *scriptfile, ctx, opt, currentNSP, *exp, handle, vm, opt.m_platform);

            if (!r.second) {
                asmout << "Duplicate node "
                    << hashutils::ExtractTmp("namespace", exp->GetNamespace()) << std::flush << "::"
                    << hashutils::ExtractTmp("function", exp->GetName()) << std::endl;
                continue;
            }

            auto& asmctx = r.first->second;

            DumpFunctionHeader(*exp, output, *scriptfile, ctx, asmctx);

            if (asmctx.m_opt.m_formatter->flags & tool::gsc::formatter::FFL_NEWLINE_AFTER_BLOCK_START) {
                output << "\n";
            }
            else {
                output << " ";
            }
            output << "gscasm {\n";

            DumpAsm(*exp, output, *scriptfile, ctx, asmctx);

            output << "}\n";

            if (asmctx.m_disableDecompiler) {
                if (opt.m_dasm) {
                    output << "// Can't decompile export " << hashutils::ExtractTmp("namespace", exp->GetNamespace()) << "::" << hashutils::ExtractTmp("function", exp->GetName()) << "\n";
                }
                else if (opt.m_dcomp) {
                    asmout << "// Can't decompile export " << hashutils::ExtractTmp("namespace", exp->GetNamespace()) << "::" << hashutils::ExtractTmp("function", exp->GetName()) << "\n\n";
                }
                else {
                    LOG_WARNING("Can't decompile export {}::{}", hashutils::ExtractTmp("namespace", exp->GetNamespace()), hashutils::ExtractTmp("function", exp->GetName()));
                }
            }


            if ((!opt.m_dasm || opt.m_dcomp || opt.m_func_header_post) && !asmctx.m_disableDecompiler) {
                asmctx.ComputeDefaultParamValue();
                if (opt.m_dasm || opt.m_func_header_post) {
                    DumpFunctionHeader(*exp, output, *scriptfile, ctx, asmctx);
                }
                output << std::flush;
                opcode::DecompContext dctx{ 0, 0, asmctx.m_opt };
                if (opt.m_dcomp) {
                    if (scriptfile->RemapFlagsExport(exp->GetFlags()) == T8GSCExportFlags::CLASS_VTABLE) {
                        asmctx.m_bcl = scriptfile->Ptr(exp->GetAddress());

                        if (asmctx.m_opt.m_formatter->flags & tool::gsc::formatter::FFL_NEWLINE_AFTER_BLOCK_START) {
                            output << "\n";
                        }
                        else {
                            output << " ";
                        }
                        output << "{\n";
                        DumpVTable(*exp, output, *scriptfile, ctx, asmctx, dctx);
                        output << "}\n";
                    }
                    else {
                        if (!(asmctx.m_opt.m_stepskip & STEPSKIP_DEV)) {
                            asmctx.ComputeDevBlocks(asmctx);
                        }
                        if (!(asmctx.m_opt.m_stepskip & STEPSKIP_SWITCH)) {
                            asmctx.ComputeSwitchBlocks(asmctx);
                        }
                        if (!(asmctx.m_opt.m_stepskip & STEPSKIP_FOREACH)) {
                            asmctx.ComputeForEachBlocks(asmctx);
                        }
                        if (!(asmctx.m_opt.m_stepskip & STEPSKIP_WHILE)) {
                            asmctx.ComputeWhileBlocks(asmctx);
                        }
                        if (!(asmctx.m_opt.m_stepskip & STEPSKIP_FOR)) {
                            asmctx.ComputeForBlocks(asmctx);
                        }
                        if (!(asmctx.m_opt.m_stepskip & STEPSKIP_IF)) {
                            asmctx.ComputeIfBlocks(asmctx);
                        }
                        if (!(asmctx.m_opt.m_stepskip & STEPSKIP_RETURN)) {
                            asmctx.ComputeReturnJump(asmctx);
                        }
                        if (!(asmctx.m_opt.m_stepskip & STEPSKIP_BOOL_RETURN)) {
                            asmctx.ComputeBoolReturn(asmctx);
                        }
                        asmctx.ComputeSpecialPattern(asmctx);
                        if (opt.m_dasm) {
                            output << " ";
                            asmctx.Dump(output, dctx);
                        }
                    }
                }
                else {
                    output << ";\n";
                }
            }
            output << "\n";
        }

        if (!opt.m_dasm && opt.m_dcomp) {
            // current namespace
            currentNSP = 0;

            for (const auto& [name, cls] : ctx.m_classes) {
                if (cls.name_space != currentNSP) {
                    currentNSP = cls.name_space;

                    asmout << "#namespace " << hashutils::ExtractTmp("namespace", currentNSP) << ";\n" << std::endl;
                }

                asmout << "// Namespace " << hashutils::ExtractTmp("namespace", cls.name_space) << std::endl;
                asmout << "// Method(s) " << std::dec << cls.m_methods.size() << " Total "  << cls.m_vtable.size() << "\n";
                asmout << "class " << hashutils::ExtractTmp("class", name) << std::flush;

                if (cls.m_superClass.size()) {
                    // write superclasses
                    asmout << " : ";
                    auto it = cls.m_superClass.begin();
                    asmout << hashutils::ExtractTmp("class", *it) << std::flush;
                    it++;

                    while (it != cls.m_superClass.end()) {
                        asmout << ", " << hashutils::ExtractTmp("class", *it) << std::flush;
                        it++;
                    }
                }

                if (opt.m_formatter->flags & tool::gsc::formatter::FFL_NEWLINE_AFTER_BLOCK_START) {
                    asmout << "\n";
                }
                else {
                    asmout << " ";
                }
                asmout << "{\n\n";

                

                auto handleMethod = [&opt, &contextes, &asmout, &scriptfile, name, &ctx](uint64_t method, const char* forceName, bool ignoreEmpty) -> void {
                    auto lname = Located{ name, method };

                    auto masmctxit = contextes.find(lname);

                    if (masmctxit == contextes.end()) {
                        return;
                    }

                    auto& e = masmctxit->second;

                    if (e.m_disableDecompiler) {
                        return;
                    }

                    if (!ignoreEmpty || SizeNoEmptyNode(e.m_funcBlock.m_statements) != 0) {
                        // ignore empty exports (constructor/destructors)
                        
                        // set the export handle
                        e.m_exp.SetHandle(e.m_readerHandle);

                        DumpFunctionHeader(e.m_exp, asmout, *scriptfile, ctx, e, 1, forceName);
                        opcode::DecompContext dctx{ 1, 0, e.m_opt };
                        if (opt.m_formatter->flags & tool::gsc::formatter::FFL_NEWLINE_AFTER_BLOCK_START) {
                            dctx.WritePadding(asmout << "\n");
                        }
                        else {
                            asmout << " ";
                        }
                        e.Dump(asmout, dctx);
                        asmout << "\n";
                    }

                    contextes.erase(masmctxit);
                };

                std::unordered_set<uint64_t> selfmembers{};

                for (const auto& method : cls.m_methods) {
                    auto lname = Located{ name, method };

                    auto masmctxit = contextes.find(lname);

                    if (masmctxit == contextes.end()) {
                        LOG_WARNING("Can't find {}", hashutils::ExtractTmp("function", method));
                        continue;
                    }
                    masmctxit->second.ConvertToClassMethod(selfmembers);
                }

                if (selfmembers.size()) {
                    // sort members using string lookup value for a better rendering
                    std::set<std::string> selfMembersSorted{};
                    for (uint64_t field : selfmembers) {
                        selfMembersSorted.insert(hashutils::ExtractTmp("var", field));
                    }

                    for (const std::string& field : selfMembersSorted) {
                        utils::Padding(asmout, 1) << "var " << field << ";\n";
                    }

                    asmout << "\n";
                }

                // handle first the constructor/destructor
                handleMethod(g_constructorName, "constructor", true);
                handleMethod(g_destructorName, "destructor", true);

                for (const auto& method : cls.m_methods) {
                    handleMethod(method, nullptr, false);
                }

                asmout << "}\n\n";
            }

            for (size_t i = 0; i < scriptfile->GetExportsCount(); i++) {
                void* handle = scriptfile->Ptr(scriptfile->GetExportsOffset()) + i * exp->SizeOf();
                exp->SetHandle(handle);

                if (scriptfile->RemapFlagsExport(exp->GetFlags()) == T8GSCExportFlags::CLASS_VTABLE) {
                    continue;
                }

                Located lname = Located{ exp->GetNamespace(), exp->GetName() };

                auto f = contextes.find(lname);

                if (f == contextes.end()) {
                    continue; // already parsed
                }

                if (exp->GetNamespace() != currentNSP) {
                    currentNSP = exp->GetNamespace();

                    asmout << "#namespace " << hashutils::ExtractTmp("namespace", currentNSP) << ";\n" << std::endl;
                }

                auto& asmctx = f->second;

                if (asmctx.m_disableDecompiler) {
                    continue;
                }

                DumpFunctionHeader(*exp, asmout, *scriptfile, ctx, asmctx);
                opcode::DecompContext dctx{ 0, 0, asmctx.m_opt };
                if (opt.m_formatter->flags & tool::gsc::formatter::FFL_NEWLINE_AFTER_BLOCK_START) {
                    dctx.WritePadding(asmout << "\n");
                }
                else {
                    asmout << " ";
                }
                asmctx.Dump(asmout, dctx);
                asmout << "\n";
            }
        }
    }

    asmout.close();

    return 0;
}

int GscInfoFile(const std::filesystem::path& path, const GscInfoOption& opt) {
    if (std::filesystem::is_directory(path)) {
        // directory
        auto ret = 0;
        for (const auto& sub : std::filesystem::directory_iterator{path}) {
            auto lret = GscInfoFile(sub.path(), opt);
            if (!ret) {
                ret = lret;
            }
        }
        return ret;
    }
    auto pathname = path.generic_string();

    if (!(
        pathname.ends_with(".gscc") || pathname.ends_with(".cscc")
        || pathname.ends_with(".gscbin") || pathname.ends_with(".cscbin")
        || pathname.ends_with(".gsic") || pathname.ends_with(".csic") // Serious GSIC format
        )) {
        return 0;
    }
    LOG_DEBUG("Reading {}", pathname);

    void* buffer{};
    size_t size;
    void* bufferNoAlign{};
    size_t sizeNoAlign;
    if (!utils::ReadFileAlign(path, bufferNoAlign, buffer, sizeNoAlign, size)) {
        LOG_ERROR("Can't read file data for {}", path.string());
        return -1;
    }

    if (size < 0x18) { // MAGIC (8), crc(4), pad(4) name(8)
        LOG_ERROR("Bad header, file size: {:x}/{:x} for {}", size, 0x18, path.string());
        std::free(bufferNoAlign);
        return -1;
    }

    auto ret = GscInfoHandleData(reinterpret_cast<byte*>(buffer), size, pathname.c_str(), opt);
    std::free(bufferNoAlign);
    return ret;
}

int DumpInfoFileData(tool::gsc::T8GSCOBJ* data, size_t size, const char* path, std::unordered_map<uint64_t, const char*>& dataset) {
    // name
    dataset.insert({ data->name, "script" });


    uint64_t* includes = reinterpret_cast<uint64_t*>(&data->magic[data->include_offset]);
    for (size_t i = 0; i < data->include_count; i++) {
        dataset.insert({ includes[i], "script" });
    }

    uintptr_t gvars_location = reinterpret_cast<uintptr_t>(data->magic) + data->globalvar_offset;
    for (size_t i = 0; i < data->globalvar_count; i++) {
        const auto* globalvar = reinterpret_cast<tool::gsc::T8GSCGlobalVar*>(gvars_location);
        dataset.insert({ globalvar->name, "var" });

        gvars_location += sizeof(*globalvar) + sizeof(uint32_t) * globalvar->num_address;
    }


    auto* exports = reinterpret_cast<tool::gsc::T8GSCExport*>(&data->magic[data->export_table_offset]);
    for (size_t i = 0; i < data->exports_count; i++) {
        const auto& exp = exports[i];
        dataset.insert({ exp.name_space, "namespace" });
        dataset.insert({ exp.name, "function" });

        if (exp.flags & tool::gsc::T8GSCExportFlags::EVENT) {
            dataset.insert({ exp.callback_event, "event" });
        }
    }

    return 0;
}

int DumpInfoFile(const std::filesystem::path& path, std::unordered_map<uint64_t, const char*>& dataset) {
    if (std::filesystem::is_directory(path)) {
        // directory
        auto ret = 0;
        for (const auto& sub : std::filesystem::directory_iterator{ path }) {
            auto lret = DumpInfoFile(sub.path(), dataset);
            if (!ret) {
                ret = lret;
            }
        }
        return ret;
    }
    auto pathname = path.generic_string();

    if (!(
        pathname.ends_with(".gscc") || pathname.ends_with(".cscc")
        || pathname.ends_with(".gscbin") || pathname.ends_with(".cscbin")
        )) {
        return 0;
    }

    LOG_DEBUG("Reading {}", pathname);

    void* buffer{};
    size_t size;
    void* bufferNoAlign{};
    size_t sizeNoAlign;
    if (!utils::ReadFileAlign(path, bufferNoAlign, buffer, sizeNoAlign, size)) {
        LOG_ERROR("Can't read file data for {}", path.string());
        return -1;
    }

    if (size < sizeof(tool::gsc::T8GSCOBJ)) {
        LOG_ERROR("Bad header, file size: {:x}/{:x} for {}", size, sizeof(tool::gsc::T8GSCOBJ), path.string());
        std::free(bufferNoAlign);
        return -1;
    }

    auto ret = DumpInfoFileData(reinterpret_cast<tool::gsc::T8GSCOBJ*>(buffer), size, pathname.c_str(), dataset);
    std::free(bufferNoAlign);
    return ret;
}

int dumpdataset(Process& proc, int argc, const char* argv[]) {
    hashutils::ReadDefaultFile();
    // scriptparsetree] [output=dataset.txt]
    const char* inputFile = "scriptparsetree";
    const char* outputFile = "dataset.csv";
    if (argc > 2) {
        inputFile = argv[2];
        if (argc > 3) {
            inputFile = argv[3];
        }
    }

    std::unordered_map<uint64_t, const char*> dataset{};

    int ret = DumpInfoFile(inputFile, dataset);
    if (ret) {
        return ret;
    }

    std::ofstream out{ outputFile };
    if (!out) {
        LOG_ERROR("Can't open output file");
    }

    out << "type,name\n";

    for (const auto& [hash, type] : dataset) {
        out << std::hex << type << "," << hashutils::ExtractTmp(type, hash) << "\n";
    }
    out.close();
    return 0;
}

tool::gsc::T8GSCOBJContext::T8GSCOBJContext() {}

// apply ~ to ref to avoid using 0, 1, 2 which might already be used

uint64_t tool::gsc::T8GSCOBJContext::GetGlobalVarName(uint16_t gvarRef) {
    auto f = m_gvars.find(gvarRef);
    if (f == m_gvars.end()) {
        return 0;
    }
    return f->second;
}

const char* tool::gsc::T8GSCOBJContext::GetStringValue(uint32_t stringRef) {
    auto f = m_stringRefs.find(stringRef);
    if (f == m_stringRefs.end()) {
        return nullptr;
    }
    return f->second;
}

uint16_t tool::gsc::T8GSCOBJContext::AddGlobalVarName(uint64_t value) {
    uint16_t id = ((uint16_t)m_gvars.size());
    m_gvars[id] = value;
    return id;
}

uint32_t tool::gsc::T8GSCOBJContext::AddStringValue(const char* value) {
    uint32_t id = ((uint32_t)m_stringRefs.size());
    m_stringRefs[id] = value;
    return id;
}

int tool::gsc::DumpAsm(GSCExportReader& exp, std::ostream& out, GSCOBJReader& gscFile, T8GSCOBJContext& objctx, opcode::ASMContext& ctx) {
    // main reading loop
    while (ctx.FindNextLocation()) {
        while (true) {
            if (objctx.m_vmInfo->flags & opcode::VmFlags::VMF_OPCODE_SHORT) {
                ctx.Aligned<uint16_t>();
            }
            byte*& base = ctx.m_bcl;

            // mark the current location as handled
            auto& loc = ctx.PushLocation();
            loc.handled = true;

            if (ctx.m_lastOpCodeBase == -1) {
                ctx.m_lastOpCodeBase = loc.rloc;
            }
            // print the stack and the fields
            auto printStack = [&ctx, &out, &loc](const char* type) {
                if (ctx.m_opt.m_dcomp && ctx.m_opt.m_display_stack) {
                    out << "." << std::hex << std::setfill('0') << std::setw(sizeof(int32_t) << 1) << loc.rloc << ":"
                        << std::setfill(' ') << std::setw(5) << std::left << " " << std::right
                        << std::setfill(' ') << std::setw(32) << std::left << type << std::right
                        << "stack(" << std::dec << ctx.m_stack.size() << "): "
                        << std::flush;

                    for (const auto* node : ctx.m_stack) {
                        out << "<" << *node << "> ";
                    }
                    out << std::endl;
                    out << "." << std::hex << std::setfill('0') << std::setw(sizeof(int32_t) << 1) << loc.rloc << ":"
                        << std::setfill(' ') << std::setw(32) << std::left << " " << std::right
                        << "fieldid: <";
                    if (ctx.m_fieldId) {
                        out << *ctx.m_fieldId;
                    }
                    else {
                        out << "none";
                    }
                    out << "> objectid: <";
                    if (ctx.m_objectId) {
                        out << *ctx.m_objectId;
                    }
                    else {
                        out << "none";
                    }
                    out << ">"
                        << std::flush;

                    out << std::endl;
                }

            };

            // compute the late operations (OR|AND)
            for (const auto& lateop : loc.m_lateop) {
                lateop->Run(ctx, objctx);
                printStack(lateop->type);
            }

            uint16_t opCode;

            if (objctx.m_vmInfo->flags & opcode::VmFlags::VMF_OPCODE_SHORT) {
                opCode = *(uint16_t*)base;
            }
            else {
                opCode = (uint16_t)*base;
            }

            const auto* handler = ctx.LookupOpCode(opCode);


            out << "." << std::hex << std::setfill('0') << std::setw(sizeof(int32_t) << 1) << loc.rloc << ": " << std::flush;

            if (opCode & ~0xFFF) {
                out << std::hex << "FAILURE, FIND errec: " << handler->m_name << "(" << opCode << ")" << "\n";
                opCode &= 0xFFF;
                ctx.m_disableDecompiler = true;
                break;
            }

            out << std::hex << std::setfill('0') << std::setw(sizeof(int16_t) << 1) << opCode
                << " "
                << std::setfill(' ') << std::setw(25) << std::left << handler->m_name << std::right
                << " " << std::flush;

            // dump rosetta data
            RosettaAddOpCode((uint32_t)(reinterpret_cast<uint64_t>(base) - reinterpret_cast<uint64_t>(gscFile.Ptr())), handler->m_id);

            // pass the opcode

            if (objctx.m_vmInfo->flags & opcode::VmFlags::VMF_OPCODE_SHORT) {
                base += 2;
            }
            else {
                base++;
            }

            // update ASMContext::WritePadding if you change the format

            auto ret = handler->Dump(out, opCode, ctx, objctx);

            if (ret) {
                break;
            }
            printStack("endop");
        }
    }
    // no more location, we can assume the final size
    // maybe we don't end on a return/end, to check?
    out << "// final size: 0x" << std::hex << ctx.FinalSize() << "\n";

    return 0;
}


int tool::gsc::DumpVTable(GSCExportReader& exp, std::ostream& out, GSCOBJReader& gscFile, T8GSCOBJContext& objctx, opcode::ASMContext& ctx, opcode::DecompContext& dctxt) {
    using namespace tool::gsc::opcode;
    uint16_t code = *(uint16_t*)ctx.Aligned<uint16_t>();
    // main reading loop
    const auto* ccp = ctx.LookupOpCode(code);

/*
 * Start
 .00000000: 000d CheckClearParams
 .00000002: 000e PreScriptCall
 .00000004: 0eca ScriptFunctionCall        params: 0 spawnstruct
 .00000010: 0b16 GetZero
 .00000012: 0eef GetGlobalObject           classes
 .00000016: 05c0 EvalFieldVariableRef      cct_shared_warning
 .0000001c: 04ce EvalArrayRef
 .0000001e: 0c18 SetVariableField
*/
    
    if (!ccp || ccp->m_id != OPCODE_CheckClearParams) {
        dctxt.WritePadding(out) << "Bad vtable opcode: " << std::hex << code << ", expected CheckClearParams\n";
        return -1;
    }

    ctx.m_bcl += 2;

    const auto* preScriptCall = ctx.LookupOpCode(code = *(uint16_t*)ctx.Aligned<uint16_t>());


    if (!preScriptCall || preScriptCall->m_id != OPCODE_PreScriptCall) {
        dctxt.WritePadding(out) << "Bad vtable opcode: " << std::hex << code << ", expected PreScriptCall\n";
        return -1;
    }

    ctx.m_bcl += 2;

    const auto* spawnStruct = ctx.LookupOpCode(code = *(uint16_t*)ctx.Aligned<uint16_t>());

    
    if (!spawnStruct) {
        dctxt.WritePadding(out) << "Bad vtable opcode: " << std::hex << code << "\n";
        return -1;
    }

    if (spawnStruct->m_id != OPCODE_ScriptFunctionCall && spawnStruct->m_id != OPCODE_CallBuiltinFunction) {
        if (gscFile.GetVM() == VM_T9) {
            return 0; // crc dump
        }
        dctxt.WritePadding(out) << "Bad vtable opcode: " << std::hex << code << ", expected ScriptFunctionCall\n";
        return -1;
    }

    ctx.m_bcl += 2 + 1;
    ctx.Aligned<uint64_t>() += 8; // assume that we have a spawnstruct

    ctx.Aligned<uint16_t>() += 2; // GetZero


    if (gscFile.GetVM() != VM_T8) {
        ctx.Aligned<uint16_t>() += 2; // EvalFieldVariableFromGlobalObject
        ctx.Aligned<uint16_t>() += 2; // - classes
    }
    else {

        ctx.Aligned<uint16_t>() += 2; // GetGlobalObject
        ctx.Aligned<uint16_t>() += 2; // - classes

        ctx.Aligned<uint16_t>() += 2; // EvalFieldVariableRef
    }

    auto& clsName = ctx.Aligned<uint32_t>();

    uint32_t name = *(uint32_t*)clsName; // __vtable

    auto& cls = objctx.m_classes[name];
    cls.name_space = exp.GetNamespace();

    clsName += 4;

    if (gscFile.GetVM() != VM_T8) {
        ctx.Aligned<uint16_t>() += 2; // SetVariableFieldFromEvalArrayRef
    }
    else {
        ctx.Aligned<uint16_t>() += 2; // EvalArrayRef
        ctx.Aligned<uint16_t>() += 2; // SetVariableField
    }

    while (true) {
        auto& func = ctx.Aligned<uint16_t>();

        uint16_t opcode = *(uint16_t*)func;
        func += 2;

        const auto* funcOpCode = ctx.LookupOpCode(opcode);

        if (!funcOpCode) {
            dctxt.WritePadding(out) << "Bad vtable opcode: " << std::hex << opcode << "\n";
            return -1;
        }
        if (funcOpCode->m_id == OPCODE_End) {
            break; // end
        }
        if (funcOpCode->m_id != OPCODE_GetResolveFunction) {
            dctxt.WritePadding(out) << "Bad vtable opcode: " << std::hex << opcode << ", excepted GetResolveFunction or End\n";
            return -1;
        }

        auto& getFuncBase = ctx.Aligned<uint64_t>();
        uint32_t methodName = ((uint32_t*)getFuncBase)[0];
        uint32_t methodClsName = ((uint32_t*)getFuncBase)[1];
        getFuncBase += 8;

        auto& uidCodeBase = ctx.Aligned<uint16_t>();

        uint16_t uidCodeOp = *(uint16_t*)uidCodeBase;

        const auto* uidCodeOpCode = ctx.LookupOpCode(uidCodeOp);

        uidCodeBase += 2;

        if (!uidCodeOpCode) {
            dctxt.WritePadding(out) << "Bad vtable opcode: " << std::hex << uidCodeOpCode->m_name << ", excepted Getter\n";
            return -1;
        }

        uint64_t uid;

        switch (uidCodeOpCode->m_id) {
        case OPCODE_GetZero: // int32_t
            uid = 0;
            break;
        case OPCODE_GetNegUnsignedInteger: // int32_t
            uid = -*(int32_t*)ctx.Aligned<int32_t>();
            ctx.m_bcl += 4;
            break;
        case OPCODE_GetNegUnsignedShort: // uint16_t
            uid = -*(uint16_t*)ctx.Aligned<uint16_t>();
            ctx.m_bcl += 2;
            break;
        case OPCODE_GetNegByte: // uint8_t
            uid = -*(uint8_t*)ctx.Aligned<uint8_t>();
            ctx.m_bcl++;
            break;
        case OPCODE_GetByte: // byte
            uid = *(byte*)ctx.Aligned<byte>();
            ctx.m_bcl++;
            break;
        case OPCODE_GetInteger: // int32_t
            uid = *(int32_t*)ctx.Aligned<int32_t>();
            ctx.m_bcl += 4;
            break;
        case OPCODE_GetLongInteger: // int64_t
            uid = *(int64_t*)ctx.Aligned<int64_t>();
            ctx.m_bcl += 8;
            break;
        case OPCODE_GetUnsignedInteger: // uint32_t
            uid = *(uint32_t*)ctx.Aligned<uint32_t>();
            ctx.m_bcl += 4;
            break;
        case OPCODE_GetUnsignedShort: // uint16_t
            uid = *(uint16_t*)ctx.Aligned<uint16_t>();
            ctx.m_bcl += 2;
            break;
        default:
            dctxt.WritePadding(out) << "Bad vtable opcode: " << std::hex << uidCodeOpCode->m_name << ", excepted Getter\n";
            return -1;
        }

        if (methodClsName == name) {
            cls.m_methods.push_back(methodName);
        }
        else {
            cls.m_superClass.emplace(methodClsName);
        }
        auto& mtd = cls.m_vtable[uid];
        mtd.name = methodName;
        mtd.nsp = methodClsName;
        dctxt.WritePadding(out) << "0x" << std::hex << std::setfill('0') << std::setw(sizeof(uid)) << uid
            << " -> &" << hashutils::ExtractTmp("class", methodClsName) << std::flush 
            << "::" << hashutils::ExtractTmp("function", methodName) << ";" << std::endl;

        ctx.Aligned<uint16_t>() += 2; // GetZero

        ctx.Aligned<uint16_t>() += 2; // EvalGlobalObjectFieldVariable
        ctx.Aligned<uint16_t>() += 2; // - gvar
        ctx.Aligned<uint32_t>() += 4; // - ref
        ctx.Aligned<uint16_t>() += 2; // EvalArray
        ctx.Aligned<uint16_t>() += 2; // CastFieldObject
        ctx.Aligned<uint16_t>() += 2; // EvalFieldVariableRef
        ctx.Aligned<uint32_t>() += 4; // - ref

        if (gscFile.GetVM() != VM_T8) {
            ctx.Aligned<uint16_t>() += 2; // SetVariableFieldFromEvalArrayRef
        }
        else {
            ctx.Aligned<uint16_t>() += 2; // EvalArrayRef
            ctx.Aligned<uint16_t>() += 2; // SetVariableField
        }
    }
/*
* Field
.000002f4: 086c GetResolveFunction        &cct_shared_warning::__constructor
.00000300: 0a9a GetInteger                674154906
.00000308: 056d GetZero
.0000030a: 07f7 EvalGlobalObjectFieldVariable classes.cct_shared_warning
.00000314: 09e4 EvalArray
.00000316: 05e3 CastFieldObject
.00000318: 0af5 EvalFieldVariableRef      __vtable
.00000320: 0887 EvalArrayRef
.00000322: 00f3 SetVariableField
* End
End
*/
    dctxt.WritePadding(out) << "// class " << hashutils::ExtractTmp("class", name) << std::flush;
    if (cls.m_superClass.size()) {
        out << " : ";
        for (auto it = cls.m_superClass.begin(); it != cls.m_superClass.end(); it++) {
            if (it != cls.m_superClass.begin()) {
                out << ", ";
            }
            out << hashutils::ExtractTmp("class", *it) << std::flush;
        }
    }
    out << "\n";
    return 0;
}

int tool::gsc::ComputeSize(GSCExportReader& exp, byte* gscFile, gsc::opcode::Platform plt, gsc::opcode::VmInfo* vminfo) {
    using namespace opcode;
    byte* loc = gscFile + exp.GetAddress();

    ASMSkipContext ctx{ loc, plt, vminfo };

    while (ctx.FindNextLocation()) {
        while (true) {
            // align to next opcode
            auto& base = ctx.Aligned<uint16_t>();

            // mark the current location as handled
            auto& loc = ctx.PushLocation();
            loc.handled = true;

            uint16_t opCode = *(uint16_t*)base;

            const auto* handler = ctx.LookupOpCode(opCode);

            if (opCode & 0x1000) {
                return 0; // bad code
            }

            // pass the opcode
            base += 2;

            if (handler->Skip(opCode, ctx)) {
                break;
            }
        }
    }

    int max = 0;
    for (auto& [loc, ref] : ctx.m_locs) {
        if (max < ref.rloc) {
            max = ref.rloc;
        }
    }
    return max + 2; // +1 for the Return/End operator
}


void tool::gsc::DumpFunctionHeader(GSCExportReader& exp, std::ostream& asmout, GSCOBJReader& gscFile, T8GSCOBJContext& objctx, opcode::ASMContext& ctx, int padding, const char* forceName) {
    auto remapedFlags = gscFile.RemapFlagsExport(exp.GetFlags());
    bool classMember = remapedFlags & (T8GSCExportFlags::CLASS_MEMBER | T8GSCExportFlags::CLASS_DESTRUCTOR);

    auto detourVal = objctx.m_gsicInfo.detours.find(exp.GetAddress());
    bool isDetour = detourVal != objctx.m_gsicInfo.detours.end();

    if (ctx.m_opt.m_func_header) {
        const char* prefix;
        if (ctx.m_opt.m_formatter->flags & tool::gsc::formatter::FFL_ONE_LINE_HEADER_COMMENTS) {
            utils::Padding(asmout, padding) << "/*\n";
            padding++;
            prefix = "";
        }
        else {
            prefix = "// ";
        }

        utils::Padding(asmout, padding) << prefix << "Namespace "
            << hashutils::ExtractTmp(classMember ? "class" : "namespace", exp.GetNamespace()) << std::flush;

        // some VMs are only using the filename in the second namespace field, the others are using the full name (without .gsc?)
        // so it's better to use spaces. A flag was added to keep the same format.
        if (objctx.m_vmInfo->flags & tool::gsc::opcode::VmFlags::VMF_FULL_FILE_NAMESPACE) {
            asmout << " / ";
        }
        else {
            asmout << "/";
        }

        asmout
            << hashutils::ExtractTmp((remapedFlags & T8GSCExportFlags::EVENT) ? "event" : "namespace", exp.GetFileNamespace()) << std::endl;

        if (isDetour) {
            auto det = detourVal->second;
            utils::Padding(asmout, padding) << prefix 
                << "Detour " << hashutils::ExtractTmp("function", exp.GetName()) << " "
                << "Offset 0x" << std::hex << det->fixupOffset << "/0x" << det->fixupSize
                << "\n"
                ;
        }

        utils::Padding(asmout, padding) << prefix << "Params " << (int)exp.GetParamCount() << ", eflags: 0x" << std::hex << (int)exp.GetFlags();

        if (remapedFlags == T8GSCExportFlags::CLASS_VTABLE) {
            asmout << " vtable";
        }
        else {
            if (remapedFlags & T8GSCExportFlags::LINKED) {
                asmout << " linked";
            }
            if (remapedFlags & T8GSCExportFlags::CLASS_LINKED) {
                asmout << " class_linked";
            }
        }

        asmout << std::endl;
        utils::Padding(asmout, padding) << prefix << std::hex << "Checksum 0x" << exp.GetChecksum() << ", Offset: 0x" << exp.GetAddress() << std::endl;

        auto size = ctx.FinalSize();
        if (size > 2) { // at least one opcode
            utils::Padding(asmout, padding) << prefix << std::hex << "Size: 0x" << size << "\n";
        }
        if (ctx.m_opt.m_formatter->flags & tool::gsc::formatter::FFL_ONE_LINE_HEADER_COMMENTS) {
            padding--;
            utils::Padding(asmout, padding) << "*/\n";
        }
    }

    if (remapedFlags == T8GSCExportFlags::CLASS_VTABLE) {
        utils::Padding(asmout, padding) << "vtable " << (forceName ? forceName : hashutils::ExtractTmp("class", exp.GetName()));
    }
    else {

        bool specialClassMember = !ctx.m_opt.m_dasm && classMember &&
            ((remapedFlags & T8GSCExportFlags::CLASS_DESTRUCTOR) || g_constructorName == exp.GetName());

        utils::Padding(asmout, padding);

        if (!specialClassMember) {
            asmout << "function ";
        }
        if (remapedFlags & T8GSCExportFlags::PRIVATE) {
            asmout << "private ";
        }
        if (remapedFlags & T8GSCExportFlags::AUTOEXEC) {
            asmout << "autoexec ";
        }
        if (remapedFlags & T8GSCExportFlags::EVENT) {
            asmout << "event_handler[" << hashutils::ExtractTmp("event", exp.GetFileNamespace()) << "] " << std::flush;
        }

        if (ctx.m_opt.m_dasm && (classMember || (remapedFlags & T8GSCExportFlags::CLASS_DESTRUCTOR))) {
            asmout << hashutils::ExtractTmp("class", exp.GetNamespace())
                << std::flush << "::";

            if (exp.GetFlags() & T8GSCExportFlags::CLASS_DESTRUCTOR) {
                asmout << "~";
            }
        }

        if (isDetour) {
            auto* detour = detourVal->second;

            asmout << "detour ";
            if (detour->replaceNamespace) {
                asmout << hashutils::ExtractTmp("namespace", detour->replaceNamespace) << std::flush;
            }
            auto replaceScript = *reinterpret_cast<uint64_t*>(&detour->replaceScriptTop);
            if (replaceScript) {
                asmout << "<" << hashutils::ExtractTmpScript(replaceScript) << ">" << std::flush;
            }

            if (detour->replaceNamespace) {
                asmout << "::";
            }

            asmout
                << hashutils::ExtractTmp("function", detour->replaceFunction) << std::flush;
        }
        else {
            asmout << (forceName ? forceName : hashutils::ExtractTmp("function", exp.GetName()));
        }

    }


    asmout << std::flush << "(";

    // local var size = <empty>, <params>, <localvars> so we need to check that we have at least param_count + 1
    if (ctx.m_localvars.size() > exp.GetParamCount()) {
        for (size_t i = 0; i < exp.GetParamCount(); i++) {
            if (i) {
                asmout << ", ";
            }

            // -1 to avoid the <empty> object, -1 because we are in reverse order
            const auto& lvar = ctx.m_localvars[ctx.m_localvars.size() - i - 2];

            if (lvar.flags & tool::gsc::opcode::T8GSCLocalVarFlag::VARIADIC) {
                asmout << "...";
            }
            else {
                if (lvar.flags & tool::gsc::opcode::T8GSCLocalVarFlag::ARRAY_REF) {
                    asmout << "&";
                }
                else if (gscFile.GetVM() != tool::gsc::opcode::VM_T8 && (lvar.flags & tool::gsc::opcode::T8GSCLocalVarFlag::T9_VAR_REF)) {
                    asmout << "*";
                }

                asmout << hashutils::ExtractTmp("var", lvar.name) << std::flush;
            }

            byte mask = ~(tool::gsc::opcode::T8GSCLocalVarFlag::VARIADIC | tool::gsc::opcode::T8GSCLocalVarFlag::ARRAY_REF);

            if (ctx.m_vm != tool::gsc::opcode::VM_T8) {
                mask &= ~tool::gsc::opcode::T8GSCLocalVarFlag::T9_VAR_REF;
            }
            
            if (lvar.flags & mask) {
                asmout << " (unk flags: " << std::hex << (int)lvar.flags << ")";
            }
            if (lvar.defaultValueNode) {
                asmout << " = ";
                opcode::DecompContext dctx = { 0, 0, ctx.m_opt };
                lvar.defaultValueNode->Dump(asmout, dctx);
            }
        }
    }
    asmout << ")";
}

int tool::gsc::gscinfo(Process& proc, int argc, const char* argv[]) {
    GscInfoOption opt{};

    if (!opt.Compute(argv, 2, argc) || opt.m_help) {
        opt.PrintHelp();
        return 0;
    }

    gRosettaOutput = opt.m_rosetta;
    gDumpStrings = opt.m_dump_strings;


    const char* globalHM = actscli::options().dumpHashmap;
    if (!globalHM) {
        // keep the option for backward compatibility
        hashutils::SaveExtracted(opt.m_dump_hashmap != nullptr);
    }
    bool computed{};
    int ret{ tool::OK };
    for (const auto& file : opt.m_inputFiles) {
        auto lret = GscInfoFile(file, opt);
        if (ret == tool::OK) {
            ret = lret;
        }
    }
    if (!globalHM) {
        hashutils::WriteExtracted(opt.m_dump_hashmap);
    }

    if (gDumpStrings) {
        std::ofstream os{ gDumpStrings };
        if (!os) {
            LOG_ERROR("Can't open string output");
        }
        else {
            for (const auto& str : gDumpStringsStore) {
                os << str << "\n";
            }
            os.close();
        }
    }
    if (gRosettaOutput) {
        std::ofstream os{ gRosettaOutput, std::ios::binary };

        if (!os) {
            LOG_ERROR("Can't open rosetta output");
        }
        else {
            os.write("ROSE", 4);

            auto len = gRosettaBlocks.size();
            os.write(reinterpret_cast<const char*>(&len), sizeof(len));

            for (const auto& [key, data] : gRosettaBlocks) {
                // gsc header
                os.write(reinterpret_cast<const char*>(&data.header), sizeof(data.header));
                len = data.blocks.size();
                os.write(reinterpret_cast<const char*>(&len), sizeof(len));
                for (const auto& block : data.blocks) {
                    os.write(reinterpret_cast<const char*>(&block), sizeof(block));
                }
            }

            // TODO: add crc
            os.write("END", 3);


            os.close();
            LOG_INFO("Rosetta index created into '{}'", gRosettaOutput);
        }
    }

    LOG_INFO("done.");
    return ret;
}

ADD_TOOL("gscinfo", " (intput)*", "GSC decompiler/disassembler", nullptr, gscinfo);
ADD_TOOL("dds", " [input=scriptparsetree] [output=dataset.csv]", "dump dataset from gscinfo", nullptr, dumpdataset);