# Atrox OS (Gemini Edition)

> A lightweight, 32-bit x86 bare-metal operating system kernel built from scratch in C and Assembly.

Atrox OS is a minimal, multiboot-compliant kernel operating system designed to run in 32-bit Protected Mode. It operates without external runtime dependencies or standard C libraries (`libc`), featuring a custom memory allocator, real-time clock integration, and a built-in virtual file system.
 To Install the os go to : https://github.com/heirotheditt/atroxOs-Project/releases/tag/Os

---

## Features

- **Custom Shell & Command Interpreter:** Interactive CLI environment with standard terminal utilities.
- **Command History:** Scroll through previously executed commands using the Up and Down arrow keys.
- **Memory Management:** Integrated bump memory allocator (`kmalloc` / `kfree`).
- **RAM Directory & File System:** In-memory file system supporting creation (`make`), text editing (`seek`), inspection (`cat`), and deletion (`remove`).
- **I/O Redirection & Echo:** Supports string printing and direct file writes (`echo text > filename`).
- **Real-Time Clock (RTC):** Interfaces directly with CMOS registers to report UTC hardware uptime (`uptime`).
- **Custom Terminal Styling:** Dynamic VGA text mode color management (`color <name>`).
- **System Control:** Hardware-level reboot (`restart`) and ACPI/QEMU shutdown (`shutdown`).

---

## Supported Commands

| Command | Description |
| :--- | :--- |
| `help` | Displays a list of all supported shell commands. |
| `ls` | Lists all files currently stored in memory. |
| `make <file>` | Creates a new file in the RAM directory. |
| `remove <file>` | Deletes a file from the RAM directory. |
| `cat <file>` | Prints the raw text content of a file to the console. |
| `echo <text>` | Prints text or redirects output (`echo text > file`). |
| `seek <file>` | Opens the interactive single-line file editor. |
| `color <name>` | Sets text color (`green`, `red`, `cyan`, `yellow`, `white`). |
| `sysinfo` | Displays system architecture and RAM heap utilization. |
| `uptime` | Reads current time directly from the CMOS Real-Time Clock. |
| `clear` | Clears the VGA text screen. |
| `restart` | Triggers an x86 CPU reset via the keyboard controller. |
| `shutdown` | Powers down the system (QEMU/ACPI-compatible). |

---

## System Requirements

| Specification | Requirement |
| :--- | :--- |
| **Architecture** | x86 (32-bit / 64-bit compatible CPU) |
| **Minimum RAM** | 8 MB |
| **Storage** | None (Runs entirely in RAM) |
| **Boot Mechanism** | Multiboot-compliant bootloader (GRUB) |

---

## Build Prerequisites

To build Atrox OS from source and package it into an ISO image, install the following dependencies on Linux (Ubuntu/Debian):

```bash
sudo apt update
sudo apt install build-essential gcc-multilib nasm qemu-system-x86 grub-pc-bin xorriso mtools
