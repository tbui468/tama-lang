#ifndef ELF_HPP
#define ELF_HPP


struct Elf32ElfHeader {
    uint8_t	 m_ident[16] = {0x7f, 'E', 'L', 'F', 1, 1, 1, 0,
                               0,   0,   0,   0, 0, 0, 0, 0}; /* Magic number and other info */
    uint16_t m_type = 1;        /* Object file type */
    uint16_t m_machine = 3;     /* Architecture */
    uint32_t m_version = 1;     /* Object file version */
    uint32_t m_entry = 0;       /* Entry point virtual address */
    uint32_t m_phoff = 0;       /* Program header table file offset */
    uint32_t m_shoff = 0;       /* Section header table file offset */
    uint32_t m_flags = 0;       /* Processor-specific flags */
    uint16_t m_ehsize = 0;      /* ELF header size in bytes */
    uint16_t m_phentsize = 0;   /* Program header table entry size */
    uint16_t m_phnum = 0;       /* Program header table entry count */
    uint16_t m_shentsize = 0;   /* Section header table entry size */
    uint16_t m_shnum = 0;       /* Section header table entry count */
    uint16_t m_shstrndx = 0;    /* Section header string table index */

    void print() {
        printf("type: %d\nmachine: %d\nversion: %d\nentry: 0x%x\nphoff: %d\nshoff: %d\nflags: %d\nehsize: %d\nphentsize: %d\nphnum: %d\n",
                m_type, m_machine, m_version, m_entry, m_phoff, m_shoff, m_flags, m_ehsize, m_phentsize, m_phnum);
    }

};

struct Elf32ProgramHeader {
    uint32_t m_type = 1; //load     /* Segment type */
    uint32_t m_offset = 0;          /* Segment file offset */
    uint32_t m_vaddr = 0;           /* Segment virtual address */
    uint32_t m_paddr = 0;           /* Segment physical address */
    uint32_t m_filesz = 0;          /* Segment size in file */
    uint32_t m_memsz = 0;           /* Segment size in memory */
    uint32_t m_flags = 5;           /* Segment flags */
    uint32_t m_align = 0x1000;      /* Segment alignment */

    void print() {
        printf("type: %d\noffset: %d\nvaddr: 0x%x\npaddr: 0x%x\nfilesz: %d\nmemsz: %d\nflags: %d\nalign: 0x%x\n", 
                m_type, m_offset, m_vaddr, m_paddr, m_filesz, m_memsz, m_flags, m_align);
    }
};

struct Elf32SectionHeader {
    public:
        //Type
        static const int SHT_NULL = 0;
        static const int SHT_PROGBITS = 1;
        static const int SHT_SYMTAB = 2;
        static const int SHT_STRTAB = 3;
        static const int SHT_RELA = 4;
        static const int SHT_HASH = 5;
        static const int SHT_DYNAMIC = 6;
        static const int SHT_NOTE = 7;
        static const int SHT_NOBITS = 8;
        static const int SHT_REL = 9;
        static const int SHT_SHLIB = 10;
        static const int SHT_DYNSYM = 11;
        static const int SHT_LOPROC = 0x70000000;
        static const int SHT_HIPROC = 0x7fffffff;
        static const int SHT_LOUSER = 0x80000000;
        static const int SHT_HIUSER = 0xffffffff;

        //Index
        static const int SHN_UNDEF = 0;
        static const int SHN_LORESERVE = 0xff00;
        static const int SHN_LOPROC = 0xff00;
        static const int SHN_HIPROC = 0xff1f;
        static const int SHN_ABS = 0xfff1;      //symbols not affected by relocation
        static const int SHN_COMMON = 0xfff2;
        static const int SHN_HIRESERVE = 0xffff;

        //Flags
        static const int SHF_WRITE = 0x1;
        static const int SHF_ALLOC = 0x2;
        static const int SHF_EXECINSTR = 0x4;
        static const int SHF_MASKPROC = 0xf0000000;

    public:
        uint32_t m_name;        /* Section name (string tbl index) */
        uint32_t m_type;        /* Section type */
        uint32_t m_flags;       /* Section flags */
        uint32_t m_addr;        /* Section virtual addr at execution */
        uint32_t m_offset;      /* Section file offset */
        uint32_t m_size;        /* Section size in bytes */
        uint32_t m_link;        /* Link to another section */
        uint32_t m_info;        /* Additional section information */
        uint32_t m_addralign;   /* Section alignment */
        uint32_t m_entsize;     /* Entry size if section holds table */
};

struct Elf32Symbol {
    uint32_t m_name;    /* Symbol name (string tbl index) */
    uint32_t m_value;   /* Symbol value */
    uint32_t m_size;    /* Symbol size */
    uint8_t  m_info;    /* Symbol type and binding */
    uint8_t  m_other;   /* Symbol visibility */
    uint16_t m_shndx;   /* Section index */

    int get_type() {
        return m_info & 0xf;
    }

    int get_binding() {
        return m_info >> 4;
    }

    uint8_t to_info(int binding, int type) {
        return (binding << 4) + (type & 0xf);
    }

    void to_string() {
        std::cout << "Name: " << m_name << std::endl;
        std::cout << "Value: " << m_value << std::endl;
        std::cout << "Size: " << m_size << std::endl;
        std::cout << "Info: " << m_info << std::endl;
        std::cout << "Other: " << m_other << std::endl;
        std::cout << "Shndex: " << m_shndx << std::endl;
    }

    static const int STT_NOTYPE = 0;
    static const int STT_OBJECT = 1;
    static const int STT_FUNC = 2;
    static const int STT_SECTION = 3;
    static const int STT_FILE = 4;
    static const int STT_LOPROC = 13;
    //14 also reserved for processor-specific semantics
    static const int STT_HIPROC = 15;


    static const int STB_LOCAL = 0;
    static const int STB_GLOBAL = 1;
    static const int STB_WEAK = 2;
    static const int STB_LOPROC = 13;
    //14 also reserved for processor-specific semantics
    static const int STB_HIPROC = 15;

};


struct Elf32Relocation {
    uint32_t m_offset;  /* Address */
    uint32_t m_info;    /* Relocation type and symbol index */

    static const int R_386_NONE = 0;
    static const int R_386_32 = 1;
    static const int R_386_PC32 = 2;

    int get_type() {
        return (uint8_t)m_info;
    }

    int get_sym_idx() {
        return (m_info >> 8);
    }

    uint32_t to_info(int sym_idx, int rel_type) {
        return (sym_idx << 8) + (uint8_t)rel_type;
    }
};


struct Elf32Section {
    std::string m_name;
    std::vector<uint8_t> m_buf;
}; 


#endif //ELF_HPP
