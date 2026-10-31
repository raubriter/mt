<img src="https://raw.githubusercontent.com/raubriter/mt/refs/heads/main/mt.svg" alt="mt" width="120"/>

<b>mt</b> is a simple simulator for the <a href="https://www.gov.br/inep/pt-br/areas-de-atuacao/avaliacao-e-exames-educacionais/enem">ENEM</a>.

<details>
<summary>Select here to view the author's note</summary>

## Author's Note 🇧🇷

This software is intended exclusively for students who wish to obtain the maximum score on the **Mathematics and Its Technologies** examination of the ENEM—45 correct answers out of 45—and who will use their final score as a qualification requirement for the First Phase of the <a href="https://impatech.edu.br/processo-seletivo/atual/calendario-do-processo-seletivo/">Selective Process</a> for the **Bachelor's Degree in Mathematics, Technology, and Innovation** at <a href="https://impatech.edu.br/institucional/conheca-o-impa-tech/">IMPA Tech</a>.

### What does this program do?

It randomly selects 45 questions from the <a href="https://www.gov.br/inep/pt-br/areas-de-atuacao/avaliacao-e-exames-educacionais/enem/provas-e-gabaritos">official ENEM question set</a> pertaining to the _Mathematics and Its Technologies (MT)_ section, for the period from 2009 to 2025—approximately 765 questions—and presents them to the user for the same duration as the official examination, excluding questions from _Natural Sciences and Their Technologies (CN)_.

### What does this program not do?

- It **does not** display the answer key.
- It **does not** display the edition year.
- It **does not** display hints or “tricks.”
- It **does not** display the question number.
- It **does not** calculate or display a score.
- It **does not** display commented solutions.
- It **does not** guarantee admission to the IMPA Tech selective process or to any other university entrance examination.

</details>

## ToC

- [Prologue](#prologue)
- [Dependencies](#dependencies)
  - [Database](#database)
- [Obtaining the source](#obtaining-the-source)
- [Files](#files)
- [Understanding `config.mk` and the `Makefile`](#understanding-configmk-and-the-makefile)
  - [What `config.mk` is](#what-configmk-is)
  - [Variables in `config.mk`](#variables-in-configmk)
  - [`Makefile` targets](#makefile-targets)
  - [The `make clean install` command](#the-make-clean-install-command)
- [Installation](#installation)
- [Corpus](#corpus)
- [Usage](#usage)
- [Customization](#customization)
  - [Behavior (`config.def.h`)](#behavior-configdefh)
  - [Filename convention](#filename-convention)
- [License](#license)

## Prologue

This program adheres to the <a href="https://suckless.org/philosophy/">suckless.org team philosophy</a> and its conventions for <a href="https://suckless.org/coding_style/">coding style</a>. Users are advised to take some time to familiarize themselves with them.

As stated by the author, the goal of the program is to provide a random selection of 45 questions—though not restricted to that quantity—from the official ENEM question database, such that they can be answered correctly within a five-hour period—likewise not restricted to that duration (see the [Customization](#customization) section).

To this end, a pre-selection range was established and a simple user interface was developed, enabling users to focus entirely on the test content instead of becoming lost in the unnecessary complexity of the UI/UX design options available when preparing for a time-intensive examination such as the ENEM.

At a minimum, users must have:

- A GNU/Linux distribution, such as <a href="https://www.debian.org/">Debian</a>, or a Windows system running WSL with an X11 server (e.g., <a href="https://github.com/microsoft/wslg">WSLg</a>), or a virtual machine (e.g., <a href="https://www.qemu.org/">QEMU</a>, <a href="https://learn.microsoft.com/en-us/windows-server/virtualization/hyper-v/overview">Hyper-V</a>, <a href="https://www.virtualbox.org/">VirtualBox</a>, or <a href="https://www.vmware.com/">VMware</a>).
- Several sheets of paper.
- A pen (preferably a transparent-barreled, black-ink pen, such as the well-known <a href="https://eu.bic.com/pt-pt/stationery/cristal-anniversary">Bic Cristal</a>).
- _and a bottle of water!_

## Dependencies

`mt` depends on the following tools and libraries, most of which are already present in Unix-like systems:

- A C compiler (`cc`) and `make`.
- Xlib headers and library (`libX11`).
- `bzip2` (invoked at runtime to decompress the `.ff.bz2` question images).
- A running X11 display server.
- The ENEM question database, stored under `DATA_DIR` (default `"data"`) as farbfeld images compressed with bzip2 (`.ff.bz2`).

For Debian/Ubuntu, the full set can be installed with:

```
apt install build-essential libx11-dev bzip2
```

### Database

The database can be obtained at Overleaf (see the [Corpus](#corpus) section) and organized using <a href="https://github.com/raubriter/tex2ff">tex2ff</a>—a shell script specifically designed to work with `mt`.

## Obtaining the source

Clone the repository and enter the directory:

```
git clone https://github.com/raubriter/mt.git
cd mt/
```

The resulting tree contains:

<img width="956" height="512" alt="image" src="https://github.com/user-attachments/assets/27158c23-9621-40cb-9f5e-57ae7f1ac15c" /><br/>

The `data/` directory is generally not tracked by the repository, since it would hold large files. Users must create and populate it themselves (see the [Corpus](#corpus) section).

## Files

- `config.def.h` — compile-time configuration.
- `config.mk` — build configuration (compiler, flags, install prefix).
- `mt.c` — the program itself.
- `logo.c`, `logo.h` — the start-screen logo, drawn as X rectangles.
- `util.c`, `util.h` — small allocation and error-handling helpers.
- `data/` — the question database (`.ff.bz2` files).

## Understanding `config.mk` and the `Makefile`

### What `config.mk` is

In the suckless ecosystem, `config.mk` is a file **included by the `Makefile`** that centralizes build variables. It allows the user to adjust how `make` compiles and installs the software **without modifying the `Makefile` logic**, which remains generic. The `Makefile` begins with a line such as:

```
include config.mk
```

so every definition in `config.mk` becomes available to the build rules.

### Variables in `config.mk`

| Variable   | Meaning                                                              | Default                                          |
|------------|----------------------------------------------------------------------|--------------------------------------------------|
| `VERSION`  | Program version, used in `CPPFLAGS` to define the `VERSION` macro.   | `1.0`                                            |
| `PREFIX`   | Installation prefix. The binary is placed in `$(PREFIX)/bin`.        | `/usr/local`                                     |
| `X11INC`   | Directory of the X11 headers.                                        | `/usr/X11R6/include`                             |
| `X11LIB`   | Directory of the X11 libraries.                                      | `/usr/X11R6/lib`                                 |
| `INCS`     | Include flags (`-I...`).                                             | `-I${X11INC}`                                    |
| `LIBS`     | Link flags (`-L... -l...`).                                          | `-L${X11LIB} -lX11`                              |
| `CPPFLAGS` | Preprocessor flags. Defines `_POSIX_C_SOURCE=200809L` and `VERSION`.  | `-D_POSIX_C_SOURCE=200809L -DVERSION=\"${VERSION}\"` |
| `CFLAGS`   | C compiler flags.                                                    | `-std=c99 -pedantic -Wall -Os ${INCS} ${CPPFLAGS}` |
| `LDFLAGS`  | Linker flags.                                                        | `${LIBS}`                                        |
| `CC`       | C compiler.                                                          | `cc`                                             |

**When each variable should be changed:**

- **`PREFIX`** — to install into `~/.local` (without `sudo`) or into `/usr` instead of `/usr/local`.
- **`CC`** — to use `clang` instead of `cc`.
- **`CFLAGS`** — to add `-g` (debug symbols) or to remove `-Os` (size optimization).
- **`X11INC` / `X11LIB`** — only if X11 resides in a non-standard location (rare on modern systems; the defaults usually work).

### `Makefile` targets

| Target          | What it does                                                                 |
|-----------------|------------------------------------------------------------------------------|
| `all` (default) | Compiles the `mt` binary from `mt.o`, `logo.o`, and `util.o`.                |
| `clean`         | Removes object files (`*.o`) and the `mt` binary.                            |
| `install`       | Copies the binary into `$(PREFIX)/bin`, creating the directory if necessary. |
| `uninstall`     | Removes the installed binary.                                                |

Running plain `make` executes the `all` target.

### The `make clean install` command

The most common installation invocation is:

```
make clean install
```

This executes **two targets in sequence**:

1. `clean` — deletes stale objects and binaries, guaranteeing a fresh build.
2. `install` — rebuilds if necessary and installs the binary under `PREFIX`.

If the user cannot write to `/usr/local`, either run `sudo make clean install` or change `PREFIX` to a directory owned by the user (e.g., `PREFIX = $(HOME)/.local`) and run `make clean install` without `sudo`.

## Installation

Edit `config.mk` to match the local setup (`mt` is installed into the `/usr/local` namespace by default).

To compile only—the binary is left in the current directory and nothing is installed:

```
make
```

To build and install `mt` (as root, if necessary):

```
make clean install
```

To remove the installed binary:

```
make uninstall
```

## Corpus

The _Corpus_ is the local database of `.ff.bz2` files consumed by `mt`. Please consult the <a href="https://github.com/raubriter/tex2ff">tex2ff GitHub repository</a> to understand how to use it.

The table below lists the source project for each edition, from 2009 to 2025:

| ENEM   | Overleaf project (read-only) |
|-----------|------------------|
| 2009 | <a href="https://www.overleaf.com/project/6ae30f8db91643c343a814d7/share#d6d9bd9c5f929d0da5930b6a469dff31fb54f150b024923c">d6d9bd9c5f929d0da5930b6a469dff31fb54f150b024923c</a> |
| 2010 | `WIP` |
| 2011 | `WIP` |
| 2012 | `WIP` |
| 2013 | `WIP` |
| 2014 | `WIP` |
| 2015 | `WIP` |
| 2016 | `WIP` |
| 2017 | `WIP` |
| 2018 | `WIP` |
| 2019 | `WIP` |
| 2020 | `WIP` |
| 2021 | `WIP` |
| 2022 | `WIP` |
| 2023 | `WIP` |
| 2024 | `WIP` |
| 2025 | `WIP` |

After accessing and exporting it as a `.zip` file, run `tex2ff` on the resulting file (e.g., `2009.zip`) and move the output into `mt`'s data directory:

<img width="956" height="661" alt="image" src="https://github.com/user-attachments/assets/f269f36e-26b0-4533-9bd4-1fcfa48452a2" />

## Usage

Once the database is in place under `DATA_DIR`, run:

```
mt
```

The program opens a fullscreen window and presents a start screen with the logo. If no `.ff.bz2` file is found under `DATA_DIR`, it exits with: _no questions found in data_

<img width="1914" height="1079" alt="image" src="https://github.com/user-attachments/assets/6d1a2b02-f18a-49e0-8ecf-67ce8e20b39e" /><br/>

On the start screen:

- <kbd>Enter</kbd> — starts the test (the pool is shuffled and the timer starts).
- <kbd>Esc</kbd> — quits.

<table>
  <tr>
    <td align="center">
      <img width="1914" height="1079" alt="image" src="https://github.com/user-attachments/assets/61e96c04-479c-4d6a-84ab-2afb03dd63d6"/>
    </td>
  </tr>
</table>

During the test:

- <kbd>A</kbd>–<kbd>E</kbd> — answers the current question.
- <kbd>Space</kbd> — skips the current question.
- <kbd>←</kbd> <kbd>→</kbd> <kbd>↑</kbd> <kbd>↓</kbd> — pans the image.
- <kbd>+</kbd> / <kbd>-</kbd> — zooms in / out.
- <kbd>Esc</kbd> — quits immediately.

**Answer semantics:**

- A **correct** answer **removes** the question from the active pool.
- A **wrong** answer or a **skip** leaves the question in the pool; the program simply draws another question, which may reappear later.

The test ends when the pool is exhausted, when the timer runs out, or when the waiting period expires.

When the pool is emptied, `mt` enters a waiting state and displays `AGUARDE` together with the timer; the maximum time spent there is controlled by `TIMER_WAIT_MINUTES`.

<table>
  <tr>
    <td align="center">
      <img width="1914" height="1079" alt="image" src="https://github.com/user-attachments/assets/5cc7c75e-5123-427b-a3e2-591b8cac9808" />
    </td>
  </tr>
</table>

The `AGUARDE` message simulates the need to wait until the instructor gives the candidate permission to leave the room on the day of the official test. According to [INEP's recent rules](https://www.in.gov.br/web/dou/-/edital-n-126-de-24-de-agosto-de-2026-727881923), a candidate may leave the room **no earlier than two hours after the start of the test**.

## Customization

### Behavior (`config.def.h`)

`mt` is customized by editing `config.def.h` and recompiling. The following options are available:

| Macro                | Default | Description                                                         |
|----------------------|---------|---------------------------------------------------------------------|
| `PACK_SIZE`          | `45`    | Number of questions in the active pool.                             |
| `TIMER_HOURS`        | `5`     | Test duration, in hours.                                            |
| `TIMER_WAIT_MINUTES` | `120`   | Maximum time spent on the waiting screen after the pool is cleared. |
| `COUNT_UP`           | `0`     | `0` counts down from `TIMER_HOURS`; `1` counts up.                  |
| `DATA_DIR`           | `"data"`| Directory containing the `.ff.bz2` question files.                  |

To apply a change:

1. Edit `config.def.h`.
2. Copy it to `config.h` (if present):
   ```
   cp config.def.h config.h
   ```
3. Rebuild and reinstall:
   ```
   make clean install
   ```

### Filename convention

The answer to each question is encoded in its filename: the character immediately before the `.ff.bz2` extension (e.g., `2009-136-C.ff.bz2` has answer **C**).

This convention must be maintained when adding new files to the database—`mt` parses it with `parse_answer()` in `mt.c`.

## License

Distributed under the GNU General Public License v3 or later (GPLv3+).

See the *LICENSE* file or visit: https://www.gnu.org/licenses/gpl-3.0.html
