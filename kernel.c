typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

#define VGA 0xB8000

static inline void outb(uint16_t port, uint8_t val) { asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) ); }
static inline void outw(uint16_t port, uint16_t val) { asm volatile ( "outw %0, %1" : : "a"(val), "Nd"(port) ); }
static inline uint8_t inb(uint16_t port) { uint8_t ret; asm volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) ); return ret; }

uint8_t current_color = 0x0F;
int cursor_x = 0, cursor_y = 0;

void write_serial(char a) {
   while ((inb(0x3F8 + 5) & 0x20) == 0);
   outb(0x3F8, a);
}

void clear_screen() {
    uint8_t *vram = (uint8_t *)VGA;
    for (int i = 0; i < 80 * 25 * 2; i += 2) { vram[i] = ' '; vram[i+1] = current_color; }
    cursor_x = 0; cursor_y = 0;
}

void print_char(char c, uint8_t color) {
    write_serial(c);
    if (c == '\n') { cursor_x = 0; cursor_y++; return; }
    uint8_t *vram = (uint8_t *)VGA;
    int offset = (cursor_y * 80 + cursor_x) * 2;
    vram[offset] = c; vram[offset+1] = color;
    cursor_x++;
    if (cursor_x >= 80) { cursor_x = 0; cursor_y++; }
}

void print(const char *str) { while (*str) { print_char(*str++, current_color); } }

char get_serial_char() {
    while ((inb(0x3F8 + 5) & 1) == 0);
    return inb(0x3F8);
}

/* --- Heap Memory Allocator --- */
#define HEAP_SIZE 1048576
uint8_t heap_memory[HEAP_SIZE];
uint32_t heap_ptr = 0;

void* kmalloc(uint32_t size) {
    if (heap_ptr + size > HEAP_SIZE) return 0;
    void* ptr = &heap_memory[heap_ptr];
    heap_ptr += size;
    return ptr;
}

void kfree(void* ptr) { (void)ptr; }

/* --- Filesystem Setup --- */
struct FileEntry { char name[16]; uint32_t lba_sector; };
struct Directory { uint32_t magic; uint32_t file_count; uint32_t next_free_sector; struct FileEntry files[16]; };

struct Directory ram_dir;
char file_contents[16][512];

int strcmp(const char* a, const char* b) { while (*a && *a == *b) { a++; b++; } return *a - *b; }
int strncmp(const char* a, const char* b, int n) { while (n && *a && *a == *b) { a++; b++; n--; } if (n == 0) return 0; return *a - *b; }
void strcpy(char *dest, const char *src) { int i = 0; while (src[i] != '\0' && i < 15) { dest[i] = src[i]; i++; } dest[i] = '\0'; }
int strlen(const char* str) { int len = 0; while(str[len]) len++; return len; }
void delay(int count) { for (volatile int i = 0; i < count * 1000000; i++); }

void init_filesystem() {
    ram_dir.magic = 0x41545258;
    ram_dir.file_count = 0;
    ram_dir.next_free_sector = 1;
}

int create_file(const char *name) {
    for (uint32_t i = 0; i < ram_dir.file_count; i++) { if (strcmp(ram_dir.files[i].name, name) == 0) return 0; }
    if (ram_dir.file_count >= 16) return -1;
    strcpy(ram_dir.files[ram_dir.file_count].name, name);
    ram_dir.files[ram_dir.file_count].lba_sector = ram_dir.file_count + 1;
    for (int j = 0; j < 512; j++) file_contents[ram_dir.file_count][j] = 0;
    ram_dir.file_count++;
    return 1;
}

int delete_file(const char *name) {
    int target_idx = -1;
    for (uint32_t i = 0; i < ram_dir.file_count; i++) {
        if (strcmp(ram_dir.files[i].name, name) == 0) { target_idx = i; break; }
    }
    if (target_idx == -1) return 0;

    for (int j = 0; j < 512; j++) file_contents[target_idx][j] = 0;
    for (uint32_t i = target_idx; i < ram_dir.file_count - 1; i++) {
        ram_dir.files[i] = ram_dir.files[i + 1];
        for (int j = 0; j < 512; j++) { file_contents[i][j] = file_contents[i + 1][j]; }
    }
    ram_dir.file_count--;
    return 1;
}

uint32_t find_file_sector(const char *name) {
    for (uint32_t i = 0; i < ram_dir.file_count; i++) { if (strcmp(ram_dir.files[i].name, name) == 0) return i + 1; }
    return 0;
}

void list_files() {
    if (ram_dir.file_count == 0) { print("No files on disk.\n"); return; }
    print("Files on Disk:\n");
    for (uint32_t i = 0; i < ram_dir.file_count; i++) { print("  - "); print(ram_dir.files[i].name); print("\n"); }
}

void cat_file(const char* name) {
    uint32_t s = find_file_sector(name);
    if (s == 0) { print("File not found.\n"); return; }
    print(file_contents[s - 1]);
    print("\n");
}

/* --- Real-Time Clock / Uptime --- */
uint8_t get_rtc_register(int reg) {
    outb(0x70, reg);
    return inb(0x71);
}

void print_dec(uint32_t num) {
    if (num == 0) { print_char('0', current_color); return; }
    char buf[12]; int i = 10; buf[11] = 0;
    while (num > 0) { buf[i--] = '0' + (num % 10); num /= 10; }
    print(&buf[i+1]);
}

void show_uptime() {
    uint8_t sec = get_rtc_register(0x00);
    uint8_t min = get_rtc_register(0x02);
    uint8_t hrs = get_rtc_register(0x04);
    sec = (sec & 0x0F) + ((sec / 16) * 10);
    min = (min & 0x0F) + ((min / 16) * 10);
    hrs = (hrs & 0x0F) + ((hrs / 16) * 10);

    print("RTC Time: "); print_dec(hrs); print(":"); print_dec(min); print(":"); print_dec(sec); print(" UTC\n");
}

/* --- Command History --- */
#define HISTORY_MAX 5
char history[HISTORY_MAX][256];
int history_count = 0;
int history_idx = 0;

void add_history(const char* cmd) {
    if (cmd[0] == '\0') return;
    if (history_count < HISTORY_MAX) {
        strcpy(history[history_count++], cmd);
    } else {
        for (int i = 0; i < HISTORY_MAX - 1; i++) strcpy(history[i], history[i+1]);
        strcpy(history[HISTORY_MAX - 1], cmd);
    }
    history_idx = history_count;
}

void get_input(char *buffer) {
    int i = 0;
    while (1) {
        char c = get_serial_char();
        if (c == 0) continue;
        if (c == 27) {
            char next1 = get_serial_char();
            char next2 = get_serial_char();
            if (next1 == '[') {
                if (next2 == 'A' && history_idx > 0) { // Up Arrow
                    history_idx--;
                    while (i > 0) { i--; print("\b \b"); }
                    strcpy(buffer, history[history_idx]);
                    i = strlen(buffer);
                    print(buffer);
                } else if (next2 == 'B' && history_idx < history_count - 1) { // Down Arrow
                    history_idx++;
                    while (i > 0) { i--; print("\b \b"); }
                    strcpy(buffer, history[history_idx]);
                    i = strlen(buffer);
                    print(buffer);
                }
            }
            continue;
        }
        if (c == '\r' || c == '\n') { buffer[i] = '\0'; print("\n"); add_history(buffer); break; }
        else if ((c == '\b' || c == 127) && i > 0) { i--; print("\b \b"); }
        else if (c >= 32 && c <= 126) { buffer[i++] = c; print_char(c, current_color); }
    }
}

void editor(const char* filename, uint32_t sector) {
    print("\n------ "); print(filename); print(" ------\n[ Press ESC to Save & Exit ]\n\n");
    int file_idx = sector - 1;
    int i = 0;
    while (file_contents[file_idx][i] != '\0' && i < 510) { print_char(file_contents[file_idx][i], current_color); i++; }
    while(1) {
        char c = get_serial_char();
        if (c == 0) continue;
        if (c == 27) { file_contents[file_idx][i] = '\0'; break; }
        if (c >= 32 && c <= 126 && i < 510) { file_contents[file_idx][i++] = c; print_char(c, current_color); }
        else if ((c == '\b' || c == 127) && i > 0) { i--; print("\b \b"); }
    }
    print("\nFile Saved!\n");
}

void parse_echo(const char* args) {
    char text[256] = {0};
    char target_file[32] = {0};
    int redirect = 0;
    int i = 0, j = 0;

    while (args[i] != '\0') {
        if (args[i] == '>') { redirect = 1; i++; break; }
        text[j++] = args[i++];
    }
    text[j] = '\0';

    if (redirect) {
        while (args[i] == ' ') i++;
        int k = 0;
        while (args[i] != '\0' && args[i] != ' ') target_file[k++] = args[i++];
        target_file[k] = '\0';

        uint32_t sector = find_file_sector(target_file);
        if (sector == 0) {
            create_file(target_file);
            sector = find_file_sector(target_file);
        }
        if (sector != 0) {
            int file_idx = sector - 1;
            strcpy(file_contents[file_idx], text);
            print("Written to "); print(target_file); print("\n");
        }
    } else {
        print(text); print("\n");
    }
}

void set_color(const char* color_name) {
    if (strcmp(color_name, "green") == 0) current_color = 0x0A;
    else if (strcmp(color_name, "red") == 0) current_color = 0x0C;
    else if (strcmp(color_name, "cyan") == 0) current_color = 0x0B;
    else if (strcmp(color_name, "yellow") == 0) current_color = 0x0E;
    else if (strcmp(color_name, "white") == 0) current_color = 0x0F;
    else print("Unknown color. Try: green, red, cyan, yellow, white\n");
}

void sysinfo() {
    print("--- System Info ---\n");
    print("OS Name: Atrox OS (Gemini Edition)\n");
    print("Architecture: x86 (32-bit Protected Mode)\n");
    print("Heap Usage: "); print_dec(heap_ptr); print(" / "); print_dec(HEAP_SIZE); print(" Bytes\n");
    print("Files Allocated: "); print_dec(ram_dir.file_count); print(" / 16\n");
}

void system_reboot() { uint8_t good = 0x02; while (good & 0x02) { good = inb(0x64); } outb(0x64, 0xFE); }
void system_shutdown() { outw(0x604, 0x2000); asm volatile("cli; hlt"); }

void kernel_main() {
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x80);
    outb(0x3F8 + 0, 0x03);
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x03);
    outb(0x3F8 + 2, 0xC7);
    outb(0x3F8 + 4, 0x0B);

    clear_screen();
    print("   ___   __                 ____         \n");
    print("  / _ | / /________ __ __  / __ \\___     \n");
    print(" / __ |/ __/ __/ _ \\\\ \\ / / /_/ (_-<     \n");
    print("/_/ |_|\\__/_/  \\___/___/  \\____/___/     \n");
    print("      X  G E M I N I                     \n\n");
    print("loading system file pls wait :).. \n\n");
    init_filesystem();
    print("System Ready.\n\n");

    char cmd[256];
    while (1) {
        print("atrox@what $ "); get_input(cmd);
        if (strncmp(cmd, "make ", 5) == 0) {
            const char *fn = &cmd[5]; int res = create_file(fn);
            if (res == 1) { print("Created file: "); print(fn); print("\n"); }
            else if (res == 0) print("File already exists.\n");
            else print("Disk directory full.\n");
        } 
        else if (strncmp(cmd, "remove ", 7) == 0) {
            const char *fn = &cmd[7]; int res = delete_file(fn);
            if (res == 1) { print("Deleted file: "); print(fn); print("\n"); }
            else print("File not found.\n");
        }
        else if (strncmp(cmd, "cat ", 4) == 0) cat_file(&cmd[4]);
        else if (strncmp(cmd, "echo ", 5) == 0) parse_echo(&cmd[5]);
        else if (strncmp(cmd, "color ", 6) == 0) set_color(&cmd[6]);
        else if (strncmp(cmd, "seek ", 5) == 0) {
            const char *fn = &cmd[5]; uint32_t s = find_file_sector(fn);
            if (s != 0) editor(fn, s);
            else print("File not found! Use 'make <filename>' first.\n");
        }
        else if (strcmp(cmd, "ls") == 0) list_files();
        else if (strcmp(cmd, "uptime") == 0) show_uptime();
        else if (strcmp(cmd, "sysinfo") == 0) sysinfo();
        else if (strcmp(cmd, "help") == 0) {
            print("Available Commands:\n");
            print("  make <file>       - Create file\n");
            print("  remove <file>     - Delete file\n");
            print("  cat <file>        - Print file text\n");
            print("  echo <text>       - Print text or redirect (> file)\n");
            print("  color <name>      - Change text color (green/red/cyan/yellow/white)\n");
            print("  seek <file>       - Edit file\n");
            print("  ls                - List files\n");
            print("  sysinfo           - System status & memory\n");
            print("  uptime            - Show Real-Time Clock\n");
            print("  clear             - Clear screen\n");
            print("  restart           - Reboot system\n");
            print("  shutdown          - Power down system\n");
        }
        else if (strcmp(cmd, "shutdown") == 0) { print("Shutting down...\n"); delay(10); system_shutdown(); }
        else if (strcmp(cmd, "restart") == 0) { print("Restarting system...\n"); delay(10); system_reboot(); }
        else if (strcmp(cmd, "clear") == 0) clear_screen();
        else if (cmd[0] != '\0') print("Command not found. Type 'help' for commands.\n");
    }
}
