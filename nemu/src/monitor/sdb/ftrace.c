#include <elf.h>
#include <stdio.h>
#include <string.h>
#include <common.h>
#include <stdarg.h>
#include <limits.h>

typedef MUXDEF(CONFIG_ISA64, Elf64_Ehdr, Elf32_Ehdr) ehdr_t;
typedef MUXDEF(CONFIG_ISA64, Elf64_Shdr, Elf32_Shdr) shdr_t;
typedef MUXDEF(CONFIG_ISA64, Elf64_Sym, Elf32_Sym) sym_t;
typedef MUXDEF(CONFIG_ISA64, Elf64_Off, Elf32_Off) elf_off_t;

#define ELF_ST_TYPE MUXDEF(CONFIG_ISA64, ELF64_ST_TYPE, ELF32_ST_TYPE)

#define MAX_FUNC_NUM 1024

typedef struct {
    char* name;
    word_t addr;
    int call_count;
} function_info_t;

static function_info_t func_table[MAX_FUNC_NUM];
static int func_count = 0;
static int call_depth = 0;

static FILE *ftrace_log_fp = NULL;
static bool ftrace_enabled = false;

static const function_info_t *find_function(word_t addr) {
    for (int i = 0; i < func_count; i++) {
        if (func_table[i].addr == addr) {
            return &func_table[i];
        }
    }
    return NULL;
}

static const function_info_t *find_function_within(word_t addr) {
    word_t min_addr = UINT32_MAX;
    const function_info_t *result = NULL;
    for (int i = 0; i < func_count; i++) {
        if (func_table[i].addr <= addr && func_table[i].addr < min_addr) {
            min_addr = func_table[i].addr;
            result = &func_table[i];
        }
    }
    return result;
}

static void ftrace_log(const char *format, ...) {
    if (ftrace_log_fp == NULL) {
        return;
    }
    va_list args;
    va_start(args, format);
    vfprintf(ftrace_log_fp, format, args);
    va_end(args);
    fflush(ftrace_log_fp);
}

static void print_trace_prefix(word_t pc) {
    ftrace_log(FMT_WORD ": %*s", pc, call_depth * 2, "");
}

void init_ftrace(const char *elf_file, const char *ftrace_log_file) {
    if (elf_file == NULL || ftrace_log_file == NULL) {
        return;
    }
    ftrace_log_fp = fopen(ftrace_log_file, "w");
    if (ftrace_log_fp == NULL) {
        LogError("Failed to open ftrace log file '%s'", ftrace_log_file);
        return;
    }
    Log("Function trace log will be written to '%s'", ftrace_log_file);

    func_count = 0;
    call_depth = 0;

    FILE *fp = fopen(elf_file, "rb");
    if (fp == NULL) {
        LogError("Failed to open ELF file '%s'", elf_file);
        return;
    }

    // parse ELF header
    ehdr_t ehdr;
    assert(fread(&ehdr, sizeof(ehdr), 1, fp) == 1);
    if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0) {
        LogError("Invalid ELF file '%s'", elf_file);
        fclose(fp);
        return;
    }

    // parse section headers to find symbol table and string table
    shdr_t shdr;
    elf_off_t strtab_offset = 0, symtab_offset = 0;
    int symbol_count = 0;
    fseek(fp, ehdr.e_shoff, SEEK_SET);
    for (int i = 0; i < ehdr.e_shnum; i++) {
        assert(fread(&shdr, sizeof(shdr), 1, fp) == 1);
        if (shdr.sh_type == SHT_STRTAB && i != ehdr.e_shstrndx) {
            strtab_offset = shdr.sh_offset;
        }
        if (shdr.sh_type == SHT_SYMTAB) {
            symtab_offset = shdr.sh_offset;
            symbol_count = shdr.sh_size / shdr.sh_entsize;
        }
    }

    // parse symbol table to find function symbols
    sym_t sym;
    elf_off_t func_name_offset[MAX_FUNC_NUM];
    fseek(fp, symtab_offset, SEEK_SET);
    for (int i = 0; i < symbol_count; i++) {
        assert(fread(&sym, sizeof(sym), 1, fp) == 1);
        if (ELF_ST_TYPE(sym.st_info) == STT_FUNC) {
            if (func_count >= MAX_FUNC_NUM) {
                break;
            }
            func_table[func_count].addr = sym.st_value;
            func_table[func_count].call_count = 0;
            func_name_offset[func_count] = sym.st_name;
            func_count++;
        }
    }

    // parse string table to get function names
    for (int i = 0; i < func_count; i++) {
        fseek(fp, strtab_offset + func_name_offset[i], SEEK_SET);
        char name[256];
        assert(fgets(name, sizeof(name), fp) != NULL);
        func_table[i].name = strdup(name);
        Log("Function %d: name = %s, addr = " FMT_WORD, i, func_table[i].name, func_table[i].addr);
    }
    
    Log("Loaded %d functions from ELF file '%s'", func_count, elf_file);


    Log("ELF file '%s' loaded successfully", elf_file);
    ftrace_enabled = true;
    fclose(fp);
}

void ftrace_call(word_t pc, word_t target_addr) {
    if (!ftrace_enabled) {
        return;
    }

    const function_info_t *func = find_function(target_addr);
    print_trace_prefix(pc);
    if (func != NULL) {
        ftrace_log("call [%s@" FMT_WORD "]\n", func->name, target_addr);
    } else {
        ftrace_log("call [unknown@" FMT_WORD "]\n", target_addr);
    }

    for (int i = 0; i < func_count; i++) {
        if (func_table[i].addr == target_addr) {
            func_table[i].call_count++;
            break;
        }
    }

    call_depth++;
}

void ftrace_ret(word_t pc, word_t target_addr) {
    if (!ftrace_enabled) {
        return;
    }

    const function_info_t *func = find_function_within(target_addr);

    if (call_depth > 0) {
        call_depth--;
    }
    print_trace_prefix(pc);
    if (func != NULL) {
        ftrace_log("ret [%s@" FMT_WORD "]\n", func->name, target_addr);
    } else {
        ftrace_log("ret [unknown@" FMT_WORD "]\n", target_addr);
    }
}

void ftrace_statistic() {
    if (!ftrace_enabled) {
        return;
    }
    if (ftrace_log_fp != NULL) {
        fclose(ftrace_log_fp);
        ftrace_log_fp = NULL;
    }
    Log("Function call statistics:");
    for (int i = 0; i < func_count; i++) {
        Log("%s: %d calls", func_table[i].name, func_table[i].call_count);
    }
}