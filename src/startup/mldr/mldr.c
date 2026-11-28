/*
This file is part of Darling.

Copyright (C) 2017 Lubos Dolezel

Darling is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Darling is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Darling.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <mach-o/loader.h>
#include <mach-o/fat.h>
#include <dlfcn.h>
#include <endian.h>
#include "commpage.h"
#include "loader.h"
#include "socket_bitmap.h"
#include "image_protection.h"
#include "interpreter_path.h"
#include "fat_slice.h"
#include "exec_policy.h"
#include <sys/resource.h>
#include <sys/prctl.h>
#include <sys/personality.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <darlingserver/rpc.h>
#include <darlingserver/rpc-supplement.h>
#include <sys/ptrace.h>
#include <pthread.h>
#include <sys/utsname.h>
#include <signal.h>
#include <kern/cs_blobs.h>

#ifndef PAGE_SIZE
	// Use the host's page size rather than assuming 4K: on 16K-page kernels (e.g. some
	// aarch64 distros) MAP_FIXED mappings at 4K-rounded addresses fail with EINVAL.
#	define PAGE_SIZE	((size_t) sysconf(_SC_PAGESIZE))
#endif
#define PAGE_ALIGN(x) (x & ~(PAGE_SIZE-1))

#if defined(__aarch64__)
// Older libc <sys/prctl.h> may not define these; they are stable in the Linux uapi.
#	ifndef PR_PAC_SET_ENABLED_KEYS
#		define PR_PAC_SET_ENABLED_KEYS	60
#	endif
#	ifndef PR_PAC_APIAKEY
#		define PR_PAC_APIAKEY	(1UL << 0)
#		define PR_PAC_APIBKEY	(1UL << 1)
#		define PR_PAC_APDAKEY	(1UL << 2)
#		define PR_PAC_APDBKEY	(1UL << 3)
#	endif

// Opt-in: with DisablePointerAuthentication set in the org.darlinghq.security
// domain (darling-prefsd hands it to darlingserver), turn off arm64 pointer
// authentication for every Darwin process started afterwards.
//
// Why: Darling runs arm64e macOS binaries, which sign and authenticate pointers (return
// addresses, C++ vtable and Objective-C method-list pointers, block invoke pointers, Swift
// function pointers) against Apple's arm64e ABI. Darling's own libraries are plain arm64 and
// do not participate, so a pointer signed by an arm64e binary reaches arm64 code still signed,
// or an arm64e authenticate runs on a value the arm64 side never signed, and the process
// crashes (e.g. _dispatch_client_callout jumping to a signed block invoke, swift_task_create).
// Disabling the keys makes every PAC sign/authenticate instruction behave as a no-op for this
// process and its threads, so arm64 and arm64e code agree on plain pointers.
//
// Security tradeoff: the Darwin processes in this container lose PAC hardening against
// memory-corruption exploits (forged return addresses and code pointers). This affects only
// Darling's Darwin processes; the host's Linux processes and any non-Darling process are
// unaffected. That is why it is opt-in and off by default.
//
// Disabling a key while a still-live function has signed its return address with it would
// make that function's return fail. This is called directly from main(), which never returns
// (it ends in start_thread()), and the helper is compiled with branch protection off, so no
// live frame holds a signed return address. Keys reset on execve, so every exec'd mldr
// applies this again; threads inherit the setting.
__attribute__((target("branch-protection=none")))
static void maybe_disable_ptrauth(void)
{
	dserver_system_config_t config;

	if (dserver_rpc_get_system_config(&config, sizeof(config)) != 0 || !config.disable_pointer_authentication)
		return;

	// enabled_keys = 0 -> all four keys disabled for this process and its future threads.
	prctl(PR_PAC_SET_ENABLED_KEYS,
	      PR_PAC_APIAKEY | PR_PAC_APIBKEY | PR_PAC_APDAKEY | PR_PAC_APDBKEY,
	      0, 0, 0);
}
#else
static void maybe_disable_ptrauth(void) {}
#endif

static const char* dyld_path = INSTALL_PREFIX "/libexec/darling/usr/lib/dyld";

struct sockaddr_un __dserver_socket_address_data = {
	.sun_family = AF_UNIX,
	.sun_path = "\0",
};

int __dserver_main_thread_socket_fd = -1;
int __dserver_process_lifetime_pipe_fd = -1;

// The idea of mldr is to load dyld_path into memory and set up the stack
// as described in dyldStartup.S.
// After that, we pass control over to dyld.
//
// Additionally, mldr providers access to native platforms libdl.so APIs (ELF loader).

#if defined(__x86_64__) || defined(__aarch64__)
static void load64(int fd, bool expect_dylinker, struct load_results* lr);
#endif
#ifdef __x86_64__
static void reexec32(char** argv);
#endif
static void load32(int fd, bool expect_dylinker, struct load_results* lr);
static void load_fat(int fd, cpu_type_t cpu, bool expect_dylinker, char** argv, struct load_results* lr);
static void load(const char* path, cpu_type_t cpu, bool expect_dylinker, char** argv, struct load_results* lr);
static int native_prot(int prot);
static void setup_space(struct load_results* lr, bool is_64_bit);
static int code_signing_gate(int fd, uint64_t image_offset, struct load_results* lr);
static void seal_segment(const char* name, uintptr_t address, uint64_t file_size, int prot);
static void process_special_env(struct load_results* lr);
static void start_thread(struct load_results* lr);
static bool is_kernel_at_least(int major, int minor);
static void* compatible_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
#if defined(__x86_64__) || defined(__aarch64__)
static void setup_stack64(const char* filepath, struct load_results* lr);
#endif
static void setup_stack32(const char* filepath, struct load_results* lr);

// UUID of the main executable
uint8_t exe_uuid[16];

// globally visible for debugging/core-dumping purposes
// however, this should not be relied on; a pointer to this should passed around to whoever needs the load_results structure
__attribute__((used))
struct load_results mldr_load_results = {0};

static uint32_t stack_size = 0;

// the path of the executable being started, for messages
static const char* exec_path = NULL;

#ifdef __i386__
// how many times mldr32 executed itself again to place the top of the address space
#define LAYOUT_RETRY_ENV "__mldr_layout_retry"
#define LAYOUT_RETRY_MAX 8
#endif

static const char* const skip_env_vars[] = {
	"__mldr_bprefs=",
	"__mldr_sockpath=",
	"__mldr_lifetime_pipe",
	"__mldr_nofile=",
};

void* __mldr_main_stack_top = NULL;

static int kernel_major = -1;
static int kernel_minor = -1;

#ifdef __x86_64__
// x86Sim: the iOS simulator's dyld shared cache. dyld_sim maps it privately with MAP_FIXED at the addresses the cache
// was built for (x86_64: SHARED_REGION_BASE_X86_64 0x7FF800000000, 6 subcaches, 24 GB, no slide info), so it can't
// move and every process maps the same file pages at the same addresses (clean pages stay shared in the page cache).
// On macOS that range is the shared region and nothing else lives there; on Linux it is where the kernel puts a
// randomized stack and the top-down mmap area. Reserve it (PROT_NONE, no commit) before anything is loaded so later
// mmaps avoid it and dyld_sim's MAP_FIXED mappings replace the reservation. Only for processes that are given a cache
// (DYLD_SHARED_CACHE_DIR). If the range is taken (about half the time with Linux stack ASLR), re-exec once with
// ADDR_NO_RANDOMIZE: the stack is then just below TASK_SIZE, above the range, and children inherit the personality.
// If it still can't be reserved, hide DYLD_SHARED_CACHE_DIR so dyld_sim runs without the cache instead of mapping
// over live memory.
#define SIM_SHARED_REGION_BASE 0x7FF800000000ULL
#define SIM_SHARED_REGION_SIZE 0x600000000ULL

static void reserve_sim_shared_region(char** argv, char** envp)
{
	static const char var[] = "DYLD_SHARED_CACHE_DIR=";
	char* found = NULL;
	for (char** e = envp; *e != NULL; ++e) {
		if (strncmp(*e, var, sizeof(var) - 1) == 0) {
			found = *e;
			break;
		}
	}
	if (found == NULL || getenv("X86SIM_NO_SHARED_REGION") != NULL)
		return;

	void* p = mmap((void*) SIM_SHARED_REGION_BASE, SIM_SHARED_REGION_SIZE, PROT_NONE,
		MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_FIXED_NOREPLACE, -1, 0);
	if (p == (void*) SIM_SHARED_REGION_BASE)
		return;
	if (p != MAP_FAILED)
		munmap(p, SIM_SHARED_REGION_SIZE); // a kernel without MAP_FIXED_NOREPLACE placed it elsewhere

	int pers = personality(0xffffffff);
	if (pers != -1 && !(pers & ADDR_NO_RANDOMIZE) && personality(pers | ADDR_NO_RANDOMIZE) != -1) {
		execve("/proc/self/exe", argv, envp);
		personality(pers);
	}

	fprintf(stderr, "mldr: shared region %#llx-%#llx is in use; running without the dyld shared cache\n",
		SIM_SHARED_REGION_BASE, SIM_SHARED_REGION_BASE + SIM_SHARED_REGION_SIZE);
	found[0] = '_'; // "_YLD_SHARED_CACHE_DIR": dyld_sim doesn't see it, children don't either
}
#endif

#ifdef __x86_64__
// x86Sim (arm64 plan, stage 1): Apple's arm64-only iOS-simulator binaries (iOS 27 runtime) run under a JIT. For an
// arm64 program (runs_as_arm64: arm64-only, or universal in an arm64-only simulator runtime), mldr re-executes itself
// on host/arm64host (an x86_64 macOS program installed next to mldr), which starts like any Darling process and then
// runs the arm64 program in libarm64jit.so (dynarmic). DYLD_* variables are renamed X86SIM_A64_DYLD_* so Darling's
// dyld doesn't act on them; libarm64jit gives them back to dyld_sim.
// X86SIM_A64_EXE is the arm64 program (Darwin path); libsystem_kernel keys simulator behaviour on it.
#define X86SIM_A64_SYSTEM_ROOT "/Volumes/SystemRoot"

// The first bytes of a file (enough for mldr_fat_select) and its size; 0 bytes if it cannot be read.
static size_t read_head(const char* path, uint8_t* buf, size_t len, uint64_t* size)
{
	struct stat st;
	int fd = open(path, O_RDONLY | O_CLOEXEC);
	*size = 0;
	if (fd < 0)
		return 0;
	ssize_t n = fstat(fd, &st) == 0 ? pread(fd, buf, len, 0) : -1;
	close(fd);
	if (n <= 0)
		return 0;
	*size = (uint64_t) st.st_size;
	return (size_t) n;
}

// Whether to run the program under the arm64 translator: an arm64-only Mach-O, or a universal one with an x86_64
// slice too in a simulator process whose runtime is arm64-only (mldr_runs_as_arm64). DYLD_ROOT_PATH (a Darwin path;
// X86SIM_A64_DYLD_ROOT_PATH where an arm64 parent's environment was passed on as is) marks a simulator process.
static bool runs_as_arm64(const char* path, char** envp, const struct load_results* lr)
{
	static uint8_t exe[MLDR_FAT_MAX_HEADER], dyld_sim[MLDR_FAT_MAX_HEADER];
	static char dyld_sim_path[8192];
	uint64_t exe_size, dyld_sim_size = 0;
	size_t exe_len = read_head(path, exe, sizeof(exe), &exe_size), dyld_sim_len = 0;
	const char* root = NULL;

	if (exe_len < 8)
		return false;
	for (char** e = envp; *e != NULL; ++e) {
		if (strncmp(*e, "DYLD_ROOT_PATH=", sizeof("DYLD_ROOT_PATH=") - 1) == 0)
			root = *e + sizeof("DYLD_ROOT_PATH=") - 1;
		else if (root == NULL && strncmp(*e, "X86SIM_A64_DYLD_ROOT_PATH=", sizeof("X86SIM_A64_DYLD_ROOT_PATH=") - 1) == 0)
			root = *e + sizeof("X86SIM_A64_DYLD_ROOT_PATH=") - 1;
	}
	if (root != NULL && *root != '\0') {
		// the runtime's dyld_sim as a Linux path (as arm64_host_setup maps X86SIM_A64_EXE)
		if (strncmp(root, X86SIM_A64_SYSTEM_ROOT "/", sizeof(X86SIM_A64_SYSTEM_ROOT)) == 0)
			snprintf(dyld_sim_path, sizeof(dyld_sim_path), "%s/usr/lib/dyld_sim", root + sizeof(X86SIM_A64_SYSTEM_ROOT) - 1);
		else
			snprintf(dyld_sim_path, sizeof(dyld_sim_path), "%s%s/usr/lib/dyld_sim", lr->root_path ? lr->root_path : "", root);
		dyld_sim_len = read_head(dyld_sim_path, dyld_sim, sizeof(dyld_sim), &dyld_sim_size);
	}
	return mldr_runs_as_arm64(exe, exe_len, exe_size, lr->bprefs, root != NULL && *root != '\0',
		dyld_sim_len ? dyld_sim : NULL, dyld_sim_len, dyld_sim_size);
}

static void arm64_reexec(const char* filename, char** argv, char** envp, struct load_results* lr)
{
	static char self[4096], host[4200], jit[4200], darwin_path[8192];
	if (!runs_as_arm64(filename, envp, lr))
		return;
	ssize_t len = readlink("/proc/self/exe", self, sizeof(self) - 1);
	if (len <= 0) {
		fprintf(stderr, "mldr: cannot find myself\n");
		exit(1);
	}
	self[len] = '\0';
	char* slash = strrchr(self, '/');
	*slash = '\0';
	snprintf(host, sizeof(host), "%s/arm64host", self);
	snprintf(jit, sizeof(jit), "%s/libarm64jit.so", self);
	*slash = '/';
	if (access(host, R_OK) != 0 || access(jit, R_OK) != 0) {
		fprintf(stderr, "mldr: %s is an arm64 binary and the arm64 runtime (%s, %s) is not installed\n", filename, host, jit);
		exit(1);
	}

	// The program's path as Darwin processes see it (as setup_stack64 computes executable_path)
	if (lr->root_path && strncmp(filename, lr->root_path, lr->root_path_length) == 0)
		snprintf(darwin_path, sizeof(darwin_path), "%s", filename + lr->root_path_length);
	else if (lr->root_path)
		snprintf(darwin_path, sizeof(darwin_path), X86SIM_A64_SYSTEM_ROOT "%s", filename);
	else
		snprintf(darwin_path, sizeof(darwin_path), "%s", filename);

	size_t envc = 0;
	while (envp[envc])
		envc++;
	char** nenv = malloc((envc + 3) * sizeof(char*));
	size_t k = 0;
	for (size_t i = 0; i < envc; i++) {
		if (strncmp(envp[i], "X86SIM_A64_EXE=", sizeof("X86SIM_A64_EXE=") - 1) == 0 || strncmp(envp[i], "X86SIM_A64_JIT=", sizeof("X86SIM_A64_JIT=") - 1) == 0)
			continue;
		if (strncmp(envp[i], "DYLD_", 5) == 0) {
			char* r = malloc(strlen(envp[i]) + sizeof("X86SIM_A64_"));
			strcpy(r, "X86SIM_A64_");
			strcat(r, envp[i]);
			nenv[k++] = r;
		} else
			nenv[k++] = envp[i];
	}
	nenv[k] = malloc(strlen(darwin_path) + sizeof("X86SIM_A64_EXE="));
	strcpy(nenv[k], "X86SIM_A64_EXE=");
	strcat(nenv[k++], darwin_path);
	nenv[k] = malloc(strlen(jit) + sizeof("X86SIM_A64_JIT="));
	strcpy(nenv[k], "X86SIM_A64_JIT=");
	strcat(nenv[k++], jit);
	nenv[k] = NULL;

	// argv[1..] are the program's argv in both invocation forms ("mldr!file argv0 ..." and "mldr file ...")
	size_t argc = 0;
	while (argv[argc])
		argc++;
	char** nargv = malloc((argc + 2) * sizeof(char*));
	nargv[0] = malloc(strlen(self) + strlen(host) + 2);
	sprintf(nargv[0], "%s!%s", self, host);
	for (size_t i = 1; i < argc; i++)
		nargv[i] = argv[i];
	if (argc < 2) {
		nargv[1] = (char*) filename;
		argc = 2;
	}
	nargv[argc] = NULL;
	execve("/proc/self/exe", nargv, nenv);
	fprintf(stderr, "mldr: cannot re-execute for %s: %s\n", filename, strerror(errno));
	exit(1);
}

// In the re-executed arm64host: keep the arm64 shared cache's fixed range (0x1C0000000, 4 GB) free for dyld_sim, and
// tell darlingserver the arm64 program is the executable (proc_pidpath, the process's platform).
static const char* arm64_host_setup(struct load_results* lr)
{
	static char linux_path[8192];
	const char* exe = getenv("X86SIM_A64_EXE");
	if (exe == NULL)
		return NULL;
	mmap((void*) 0x1C0000000ULL, 0x100000000ULL, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_FIXED_NOREPLACE, -1, 0);
	// the re-exec through /proc/self/exe named the Linux task "exe" (launchd's logs, proc_name): use the program's name
	const char* base = strrchr(exe, '/');
	prctl(PR_SET_NAME, base ? base + 1 : exe, 0, 0, 0);
	if (strncmp(exe, X86SIM_A64_SYSTEM_ROOT "/", sizeof(X86SIM_A64_SYSTEM_ROOT)) == 0)
		snprintf(linux_path, sizeof(linux_path), "%s", exe + sizeof(X86SIM_A64_SYSTEM_ROOT) - 1);
	else if (lr->root_path)
		snprintf(linux_path, sizeof(linux_path), "%s%s", lr->root_path, exe);
	else
		snprintf(linux_path, sizeof(linux_path), "%s", exe);
	return linux_path;
}
#endif
static unsigned int inherited_nofile[2];
static bool have_inherited_nofile = false;

int main(int argc, char** argv, char** envp)
{
	void** sp;
	int pushCount = 0;
	char *filename, *p = NULL;
	size_t arg_strings_total_size_after = 0;
	size_t orig_argv0_len = 0;
	const char* orig_argv1 = NULL;

	// x86Sim: no transparent huge pages for Darling processes (inherited across fork/exec; arm64 guests already set
	// it in libarm64jit's host.cpp). With the host's THP "always", the x86_64 daemons' sparse malloc/libdispatch
	// regions were backed by 2 MB pages: iokitd ~66 of its ~70 MB PSS was AnonHugePages for a few KB actually used.
	prctl(PR_SET_THP_DISABLE, 1, 0, 0, 0);

	mldr_load_results.kernfd = -1;
	mldr_load_results.argc = argc;
	mldr_load_results.argv = argv;

	while (envp[mldr_load_results.envc] != NULL) {
		++mldr_load_results.envc;
	}
	mldr_load_results.envp = envp;

#ifdef __x86_64__
	reserve_sim_shared_region(argv, envp);
#endif

	// sys_execve() passes the original file path appended to the mldr path in argv[0].
	if (argc > 0)
		p = strchr(argv[0], '!');

	if (argc <= 1)
	{
		if (p == NULL) {
			fprintf(stderr, "mldr is part of Darling. It is not to be executed directly.\n");
			return 1;
		}
		else
		{
			fprintf(stderr, "mldr: warning: Executing with no argv[0]. Continuing anyway, but this is probably a bug.\n");
		}
	}

	if (p != NULL)
	{
		filename = (char*) __builtin_alloca(strlen(argv[0])+1);
		strcpy(filename, p + 1);
	}
	else
	{
		filename = (char*) __builtin_alloca(strlen(argv[1])+1);
		strcpy(filename, argv[1]);
	}

	// allow any process to ptrace us
	// the only process we really care about being able to do this is the server,
	// but we can't just use the server's PID, since it lies outside our PID namespace.
	prctl(PR_SET_PTRACER, PR_SET_PTRACER_ANY, 0, 0, 0);

	process_special_env(&mldr_load_results);

	exec_path = filename;
#ifdef __x86_64__
	arm64_reexec(filename, argv, envp, &mldr_load_results);
	const char* arm64_exe = arm64_host_setup(&mldr_load_results);
#endif

#ifdef __i386__
	load(filename, CPU_TYPE_X86, false, argv, &mldr_load_results); // accept i386 only
#else
	load(filename, 0, false, argv, &mldr_load_results);
#endif

	// this was previously necessary when we were loading the binary from the LKM
	// (presumably because the break was detected incorrectly)
	// but this shouldn't be necessary for loading Mach-O's from userspace (the heap space should already be set up properly).
	// see https://github.com/darlinghq/darling/issues/469 for the issue this originally fixed in the LKM
#if 0
	if (prctl(PR_SET_MM, PR_SET_MM_BRK, PAGE_ALIGN(mldr_load_results.vm_addr_max), 0, 0) < 0) {
		fprintf(stderr, "Failed to set BRK value\n");
		return 1;
	}

	if (prctl(PR_SET_MM, PR_SET_MM_START_BRK, PAGE_ALIGN(mldr_load_results.vm_addr_max), 0, 0) < 0) {
		fprintf(stderr, "Failed to set BRK start\n");
		return 1;
	}
#endif

	// adjust argv (remove mldr's argv[0])
	// NOTE: this code assumes that the current argv array points to contiguous strings.
	//       this is not necessarily true, although AFAIK this is always true on Linux.
	// also note: we do it this way (moving the string contents in addition to the pointers)
	//            so that Linux sees our modified argv array without having to use PR_SET_MM_ARG_START
	//            and PR_SET_MM_ARG_END (since those require CAP_SYS_RESOURCE)

	--mldr_load_results.argc;

	orig_argv0_len = strlen(mldr_load_results.argv[0]) + 1;
	orig_argv1 = mldr_load_results.argv[1];

	for (size_t i = 0; i < mldr_load_results.argc; ++i) {
		mldr_load_results.argv[i] = mldr_load_results.argv[0] + arg_strings_total_size_after;
		arg_strings_total_size_after += strlen(mldr_load_results.argv[i + 1]) + 1;
	}
	mldr_load_results.argv[mldr_load_results.argc] = NULL;

	memmove(mldr_load_results.argv[0], orig_argv1, arg_strings_total_size_after);
	memset(mldr_load_results.argv[0] + arg_strings_total_size_after, 0, orig_argv0_len);

	if (p == NULL) {
		vchroot_unexpand_interpreter(&mldr_load_results);
	}

	// adjust envp (remove special mldr variables)
	// NOTE: same as for argv; here we assume the envp strings are contiguous
	for (size_t i = 0; i < mldr_load_results.envc; ++i) {
		if (!mldr_load_results.envp[i]) {
			mldr_load_results.envc = i;
			break;
		}

		size_t len = strlen(mldr_load_results.envp[i]) + 1;

		#define ENV_VAR_MATCHES(_name) \
			(len > sizeof(_name) - 1 && strncmp(mldr_load_results.envp[i], _name, sizeof(_name) - 1) == 0)

		// Don't pass these special env vars down to userland
		if (
			ENV_VAR_MATCHES("__mldr_bprefs=")   ||
			ENV_VAR_MATCHES("__mldr_sockpath=") ||
			ENV_VAR_MATCHES("__mldr_nofile=")   ||
			ENV_VAR_MATCHES("__mldr_layout_retry=")
		) {
			size_t len_after = 0;
			const char* orig_envp_i_plus_one = mldr_load_results.envp[i + 1];

			--mldr_load_results.envc;

			for (size_t j = i; j < mldr_load_results.envc; ++j) {
				mldr_load_results.envp[j] = mldr_load_results.envp[i] + len_after;
				len_after += strlen(mldr_load_results.envp[j + 1]) + 1;
			}
			mldr_load_results.envp[mldr_load_results.envc] = NULL;

			memmove(mldr_load_results.envp[i], orig_envp_i_plus_one, len_after);
			memset(mldr_load_results.envp[i] + len_after, 0, len);

			// we have to check this index again because it now points to a different string
			--i;
			continue;
		}
		// If we were passed __mldr_DYLD_ROOT_PATH, it is a special case of DYLD_ROOT_PATH needing to be set,
		// so we remove the prefix, so dyld reads it as DYLD_ROOT_PATH
		else if (ENV_VAR_MATCHES("__mldr_DYLD_ROOT_PATH=")) {
			const char* env_p = mldr_load_results.envp[i];
			env_p += (sizeof("__mldr_") - 1);
			size_t len_remaining = strlen(env_p) + 1;
			memmove(mldr_load_results.envp[i], env_p, len_remaining);
		}
	}

	if (mldr_load_results._32on64)
		setup_stack32(filename, &mldr_load_results);
	else
#if defined(__x86_64__) || defined(__aarch64__)
		setup_stack64(filename, &mldr_load_results);
#else
		abort();
#endif

	int status = dserver_rpc_set_dyld_info(mldr_load_results.dyld_all_image_location, mldr_load_results.dyld_all_image_size);
	if (status < 0) {
		fprintf(stderr, "Failed to tell darlingserver about our dyld info\n");
		exit(1);
	}

#ifdef __x86_64__
	if (arm64_exe != NULL)
		filename = (char*) arm64_exe;
#endif
	if (dserver_rpc_set_executable_path(filename, strlen(filename)) < 0) {
		fprintf(stderr, "Failed to tell darlingserver about our executable path\n");
		exit(1);
	}

	if (dserver_rpc_set_executable_uuid(mldr_load_results.uuid) < 0) {
		fprintf(stderr, "Failed to tell darlingserver about our executable UUID\n");
		exit(1);
	}

	__mldr_main_stack_top = (void*)mldr_load_results.stack_top;

	// Before any of the program's code runs; it needs the darlingserver connection,
	// and it must be called from main() (see the helper).
	maybe_disable_ptrauth();

	start_thread(&mldr_load_results);

	__builtin_unreachable();
}

void load(const char* path, cpu_type_t forced_arch, bool expect_dylinker, char** argv, struct load_results* lr)
{
	int fd;
	uint32_t magic;

	fd = open(path, O_RDONLY);
	if (fd == -1)
	{
		fprintf(stderr, "Cannot open %s: %s\n", path, strerror(errno));
		exit(1);
	}

	// We need to read argv[1] and detect whether it's a 32 or 64-bit application.
	// Then load the appropriate version of dyld from the fat file.
	// In case the to-be-executed executable contains both, we prefer the 64-bit version,
	// unless a special property has been passed to sys_posix_spawn() to force the 32-bit
	// version. See posix_spawnattr_setbinpref_np().

	if (read(fd, &magic, sizeof(magic)) != sizeof(magic))
	{
		fprintf(stderr, "Cannot read the file header of %s.\n", path);
		exit(1);
	}

	if (magic == MH_MAGIC_64 || magic == MH_CIGAM_64)
	{
#if defined(__x86_64__) || defined(__aarch64__)
		lseek(fd, 0, SEEK_SET);
		load64(fd, expect_dylinker, lr);
#else
		abort();
#endif
	}
	else if (magic == MH_MAGIC || magic == MH_CIGAM)
	{
#if !__x86_64__
		lseek(fd, 0, SEEK_SET);
		load32(fd, expect_dylinker, lr);
#else
		// Re-run self as mldr32
		reexec32(argv);
#endif
	}
	else if (mldr_fat_is_magic((const uint8_t*) &magic))
	{
		lseek(fd, 0, SEEK_SET);
		load_fat(fd, forced_arch, expect_dylinker, argv, lr);
	}
	else
	{
		fprintf(stderr, "Unknown file format: %s.\n", path);
		exit(1);
	}

	close(fd);
}

static void load_fat(int fd, cpu_type_t forced_arch, bool expect_dylinker, char** argv, struct load_results* lr) {
	uint8_t header[MLDR_FAT_MAX_HEADER];
	struct mldr_fat_slice slice;
	struct stat st;
	uint32_t native, fallback;

#if defined(__x86_64__)
	native = CPU_TYPE_X86_64;
	fallback = CPU_TYPE_X86;
#elif defined(__i386__)
	native = CPU_TYPE_X86;
	fallback = 0;
#elif defined(__aarch64__)
	native = CPU_TYPE_ARM64;
	fallback = CPU_TYPE_ARM;
#else
	#error Unsupported CPU architecture
#endif

	if (fstat(fd, &st) != 0 || st.st_size < 0)
	{
		fprintf(stderr, "Cannot determine the size of the fat file.\n");
		exit(1);
	}

	ssize_t got = pread(fd, header, sizeof(header), 0);
	if (got < 0)
	{
		fprintf(stderr, "Cannot read fat file header.\n");
		exit(1);
	}

	switch (mldr_fat_select(header, (size_t) got, (uint64_t) st.st_size, lr->bprefs, (uint32_t) forced_arch,
			native, fallback, &slice))
	{
		case MLDR_FAT_OK:
			break;
		case MLDR_FAT_NO_MATCH:
			fprintf(stderr, "No supported architecture found in fat binary.\n");
			exit(1);
		default:
			fprintf(stderr, "Malformed fat file header.\n");
			exit(1);
	}

	// the slice offset has to be representable as a file offset (32-bit builds
	// without large file support cannot address more)
	off_t slice_offset = (off_t) slice.offset;
	if (slice_offset < 0 || (uint64_t) slice_offset != slice.offset ||
			lseek(fd, slice_offset, SEEK_SET) == -1)
	{
		fprintf(stderr, "Cannot seek to selected arch in fat binary.\n");
		exit(1);
	}

	if (slice.cputype & CPU_ARCH_ABI64) {
#if defined(__x86_64__) || defined(__aarch64__)
		load64(fd, expect_dylinker, lr);
#else
		abort();
#endif
	} else {
#if !__x86_64__
		load32(fd, expect_dylinker, lr);
#else
		// Re-run self as mldr32
		reexec32(argv);
#endif
	}
};

#if defined(__x86_64__) || defined(__aarch64__)
#define GEN_64BIT
#include "loader.c"
#include "stack.c"
#undef GEN_64BIT
#endif

#define GEN_32BIT
#include "loader.c"
#include "stack.c"
#undef GEN_32BIT

int native_prot(int prot)
{
	int protOut = 0;

	if (prot & VM_PROT_READ)
		protOut |= PROT_READ;
	if (prot & VM_PROT_WRITE)
		protOut |= PROT_WRITE;
	if (prot & VM_PROT_EXECUTE)
		protOut |= PROT_EXEC;

	return protOut;
}

static void reexec32(char** argv)
{
	char selfpath[1024];
	ssize_t len;

	len = readlink("/proc/self/exe", selfpath, sizeof(selfpath)-3);
	if (len == -1)
	{
		perror("Cannot readlink /proc/self/exe");
		abort();
	}

	selfpath[len] = '\0';
	strcat(selfpath, "32");

	execv(selfpath, argv);

	perror("Cannot re-execute as 32-bit process");
	abort();
}

// Given that there's no proper way of passing special parameters to the binary loader
// via execve(), we must do this via env variables
static void process_special_env(struct load_results* lr) {
	const char* str;
	static char root_path[4096];

	lr->bprefs[0] = lr->bprefs[1] = lr->bprefs[2] = lr->bprefs[3] = 0;
	str = getenv("__mldr_bprefs");

	if (str != NULL) {
		sscanf(str, "%x,%x,%x,%x", &lr->bprefs[0], &lr->bprefs[1], &lr->bprefs[2], &lr->bprefs[3]);
	}

	// Without a path the static address would name an abstract socket, which is not where
	// darlingserver listens.
	str = getenv("__mldr_sockpath");

	if (str == NULL || str[0] == '\0' || strlen(str) > sizeof(__dserver_socket_address_data.sun_path) - 1) {
		fprintf(stderr, "mldr: __mldr_sockpath is missing, empty or too long; start programs through darling\n");
		exit(1);
	}
	strncpy(__dserver_socket_address_data.sun_path, str, sizeof(__dserver_socket_address_data.sun_path) - 1);
	__dserver_socket_address_data.sun_path[sizeof(__dserver_socket_address_data.sun_path) - 1] = '\0';

	lr->socket_path = __dserver_socket_address_data.sun_path;

	lr->lifetime_pipe = -1;
	str = getenv("__mldr_lifetime_pipe");

	if (str != NULL) {
		sscanf(str, "%i", &lr->lifetime_pipe);
	}

	// the Darwin RLIMIT_NOFILE of the process that exec'd us; libsystem_kernel
	// keeps it apart from the Linux limit, which must stay above our RPC sockets
	str = getenv("__mldr_nofile");

	if (str != NULL) {
		unsigned int cur, max;
		if (sscanf(str, "%u,%u", &cur, &max) == 2 && cur <= max) {
			inherited_nofile[0] = cur;
			inherited_nofile[1] = max;
			have_inherited_nofile = true;
		}
	}

	str = getenv("__mldr_DYLD_ROOT_PATH");

	if (str != NULL && lr->root_path == NULL) {
		strncpy(root_path, str, sizeof(root_path) - 1);
		root_path[sizeof(root_path) - 1] = '\0';
		lr->root_path = root_path;
		lr->root_path_length = strlen(lr->root_path);
	}
};

static void unset_special_env() {
	unsetenv("__mldr_bprefs");
	unsetenv("__mldr_sockpath");
	unsetenv("__mldr_lifetime_pipe");
	unsetenv("__mldr_nofile");
#ifdef __i386__
	unsetenv(LAYOUT_RETRY_ENV);
#endif
};

int __mldr_inherited_nofile_limit(unsigned int limits[2]) {
	if (!have_inherited_nofile) {
		return -1;
	}

	limits[0] = inherited_nofile[0];
	limits[1] = inherited_nofile[1];
	return 0;
};

static socket_bitmap_t socket_bitmap = {
	.mutex = PTHREAD_MUTEX_INITIALIZER,
	.next_index = 0,
	.bits = NULL,
	.bit_length = 0,
	.highest = -1,
};

void __mldr_socket_bitmap_prefork(void) {
	socket_bitmap_fork_prepare(&socket_bitmap);
}

void __mldr_socket_bitmap_postfork_parent(void) {
	socket_bitmap_fork_parent(&socket_bitmap);
}

// In a fork's child there is only one thread, but the bitmap's mutex may have
// been held by another thread of the parent at the fork. Reinitialize it, both
// for the libc fork path and for forks that bypass the host libc (where the
// postfork_child elf call runs this first).
void __mldr_socket_bitmap_postfork_child(void) {
	pthread_mutex_init(&socket_bitmap.mutex, NULL);
}

int __mldr_create_rpc_socket(void) {
	int pre_fd = -1;
	int fd = -1;

	pre_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (pre_fd < 0) {
		goto err_out;
	}

	fd = socket_bitmap_place(&socket_bitmap, pre_fd);
	if (fd < 0) {
		goto err_out;
	}

	if (fd != pre_fd) {
		close(pre_fd);
	}
	pre_fd = -1;

	int fd_flags = fcntl(fd, F_GETFD);
	if (fd_flags < 0) {
		goto err_out;
	}
	if (fcntl(fd, F_SETFD, fd_flags | FD_CLOEXEC) < 0) {
		goto err_out;
	}

	sa_family_t family = AF_UNIX;
	if (bind(fd, (const struct sockaddr*)&family, sizeof(family)) < 0) {
		goto err_out;
	}

out:
	return fd;

err_out:
	if (fd >= 0) {
		socket_bitmap_put(&socket_bitmap, fd);
		close(fd);
	}

	if (pre_fd >= 0) {
		close(pre_fd);
	}

	return -1;
};

void __mldr_close_rpc_socket(int socket) {
	close(socket);
	socket_bitmap_put(&socket_bitmap, socket);
};

int __mldr_create_process_lifetime_pipe(int* fds) {
	// These pipes are not required for Linux 5.3 or newer,
	// we already have pidfd_open.
	if (is_kernel_at_least(5, 3)) {
		fds[0] = fds[1] = -1;
		return 0;
	}

	int pre_fds[2] = { -1, -1 };
	fds[0] = fds[1] = -1;
	if (pipe(pre_fds) == -1) {
		goto err_out;
	}

	for (int i = 0; i < 2; ++i) {
		fds[i] = socket_bitmap_place(&socket_bitmap, pre_fds[i]);
		if (fds[i] < 0) {
			goto err_out;
		}

		if (fds[i] != pre_fds[i]) {
			close(pre_fds[i]);
		}
		pre_fds[i] = -1;
	}

	return 0;

err_out:
	for (int i = 0; i < 2; ++i) {
		if (fds[i] >= 0) {
			socket_bitmap_put(&socket_bitmap, fds[i]);
			close(fds[i]);
		}

		if (pre_fds[i] >= 0) {
			close(pre_fds[i]);
		}
	}

	return -1;
}

void __mldr_close_process_lifetime_pipe(int fd) {
	if (fd != -1) {
		close(fd);
		socket_bitmap_put(&socket_bitmap, fd);
	}
}

#ifdef __i386__
// A 32-bit process gets its Linux stack (and the mappings next to it) at a
// random place just below the top of the address space, which is where the
// Darwin commpage and the main thread stack below it have to go. If that range
// is taken, execute ourselves again to get another placement.
static void ensure_top_range_free(struct load_results* lr, unsigned long stack_size) {
	unsigned long start = commpage_address(false) - stack_size;
	unsigned long length = commpage_length(false) + stack_size;

	void* probe = compatible_mmap((void*)start, length, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
	if (probe != MAP_FAILED) {
		munmap(probe, length);
		if (probe == (void*)start)
			return;
	} else if (errno != EEXIST) {
		return; // let the mappings below report the error
	}

	const char* str = getenv(LAYOUT_RETRY_ENV);
	int retries = 0;
	if (str != NULL) {
		char* end = NULL;
		errno = 0;
		long value = strtol(str, &end, 10);
		if (str[0] < '0' || str[0] > '9' || end == str ||
		    *end != '\0' || errno == ERANGE ||
		    value < 0 || value >= LAYOUT_RETRY_MAX)
			return; // invalid or exhausted retry budget
		retries = (int)value;
	}

	// Without address space randomization the Linux stack always sits at the
	// very top of a 32-bit address space, so another try would find the same
	// layout. The Darwin stack and commpage are placed by us regardless, so
	// randomization is only given up for the Linux-side placement.
	int persona = personality(0xffffffff);
	if (persona != -1 && (persona & ADDR_NO_RANDOMIZE))
		personality(persona & ~ADDR_NO_RANDOMIZE);

	char selfpath[1024];
	ssize_t len = readlink("/proc/self/exe", selfpath, sizeof(selfpath) - 1);
	if (len == -1)
		return;
	selfpath[len] = '\0';

	char count[16];
	snprintf(count, sizeof(count), "%d", retries + 1);
	if (setenv(LAYOUT_RETRY_ENV, count, 1) != 0)
		return;

	execv(selfpath, lr->argv);
	unsetenv(LAYOUT_RETRY_ENV);
}
#endif

#ifdef __i386__
// How much stack the main thread gets: the stack limit, like the main thread
// of a macOS process (8 MB by default), but at most 8 MB and at least the
// initial size.
#define MAIN_STACK_RESERVE_MAX (8UL * 1024 * 1024)

static unsigned long main_stack_reserve(const struct rlimit* limit, unsigned long initial_size) {
	unsigned long reserve = MAIN_STACK_RESERVE_MAX;
	if (limit->rlim_cur != RLIM_INFINITY && limit->rlim_cur < reserve)
		reserve = limit->rlim_cur;
	if (reserve < initial_size)
		reserve = initial_size;
	return reserve & ~((unsigned long) PAGE_SIZE - 1);
}

// Maps the main thread stack in full, with a guard page at its low end. It goes
// right below the commpage when that range is free, otherwise wherever the
// kernel finds room; lr->stack_top is the end of the mapping either way.
static bool map_main_stack_i386(struct load_results* lr, unsigned long reserve) {
	const int prot = PROT_READ | PROT_WRITE;
	const int flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE;

	void* wanted = (void*)(lr->stack_top - reserve);
	void* stack = compatible_mmap(wanted, reserve, prot, flags | MAP_FIXED_NOREPLACE, -1, 0);
	if (stack == MAP_FAILED && errno == EEXIST) {
		stack = compatible_mmap(NULL, reserve, prot, flags, -1, 0);
		if (stack != MAP_FAILED)
			lr->stack_top = (unsigned long) stack + reserve;
	}
	if (stack == MAP_FAILED)
		return false;

	// The lowest page is a guard, so that a stack overflow faults instead of
	// running into the mapping below. It is not part of the usable stack, which
	// the exec policy may make executable.
	if (mprotect(stack, PAGE_SIZE, PROT_NONE) != 0)
		return false;

	lr->stack_map_size = reserve - PAGE_SIZE;
	return true;
}
#endif

static void setup_space(struct load_results* lr, bool is_64_bit) {
	struct rlimit limit;
	getrlimit(RLIMIT_STACK, &limit);
	// allocate a few pages 16 pages if it's less than the limit; otherwise, allocate the limit
	unsigned long size = PAGE_SIZE * 16;
	if (limit.rlim_cur != RLIM_INFINITY && limit.rlim_cur < size) {
		size = limit.rlim_cur;
	}

#ifdef __i386__
	ensure_top_range_free(lr, size);
#endif

	commpage_setup(is_64_bit);

	// Using the default stack top would cause the stack to be placed just above the commpage
	// and would collide with it eventually.
	// Instead, we manually allocate a new stack below the commpage.
#if __x86_64__ || __aarch64__
	lr->stack_top = commpage_address(true);
#elif __i386__
	lr->stack_top = commpage_address(false);
#else
	#error Unsupported architecture
#endif

	// The personality is inherited across fork and exec, so drop whatever the
	// previous process of this tree had; the main executable's header decides
	// again in mldr_apply_exec_policy().
	mldr_apply_exec_policy(lr, false, 0);

#ifdef __i386__
	// A 32-bit process has the host's initial stack (with argv, envp and the
	// auxiliary vector) at a random place close to the top of the address
	// space, possibly right below the commpage. A main-thread stack that only
	// grows on demand then runs into that mapping without any fault and
	// overwrites those strings and the auxiliary vector (the C library reads
	// the page size from it). So the whole stack is mapped up front, and if
	// the space below the commpage is not free it is put elsewhere.
	if (!map_main_stack_i386(lr, main_stack_reserve(&limit, size))) {
		fprintf(stderr, "Failed to allocate the main thread stack: %d (%s)\n", errno, strerror(errno));
		exit(1);
	}
#else
	lr->stack_map_size = size;
	if (compatible_mmap((void*)(lr->stack_top - size), size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE | MAP_GROWSDOWN, -1, 0) == MAP_FAILED) {
		fprintf(stderr, "Failed to allocate stack of %lu bytes: %d (%s)\n", size, errno, strerror(errno));
		exit(1);
	}
#endif

	unset_special_env();

	lr->kernfd = __mldr_create_rpc_socket();
	if (lr->kernfd < 0) {
		fprintf(stderr, "Failed to create socket\n");
		exit(1);
	}

	__dserver_main_thread_socket_fd = lr->kernfd;

	int lifetime_pipe[2];

	// this process is created using exec from another Darling process.
	// darlingserver should already have the read pipe, so we don't need
	// to check that in.
	if (lr->lifetime_pipe != -1) {
		// move the existing pipe to a higher fd number, and invalidate
		// the old fd to prevent interfering with fds provided by
		// socket_bitmap_get
		lifetime_pipe[1] = socket_bitmap_place(&socket_bitmap, lr->lifetime_pipe);
		if (lifetime_pipe[1] < 0) {
			// it could not be moved; keep it where it is
			lifetime_pipe[1] = lr->lifetime_pipe;
		} else if (lifetime_pipe[1] != lr->lifetime_pipe) {
			close(lr->lifetime_pipe);
		}

		lifetime_pipe[0] = -1;
	} else {
		if (__mldr_create_process_lifetime_pipe(lifetime_pipe) == -1) {
			fprintf(stderr, "Failed to create process lifetime pipe: %d (%s)\n", errno, strerror(errno));
			exit(1);
		}
	}

	lr->lifetime_pipe = lifetime_pipe[1];

	// store the write end of the pipe; the read end is sent to darlingserver.
	__dserver_process_lifetime_pipe_fd = lifetime_pipe[1];

	int dummy_stack_variable;
	if (dserver_rpc_checkin(false, &dummy_stack_variable, lifetime_pipe[0]) < 0) {
		fprintf(stderr, "Failed to checkin with darlingserver\n");
		exit(1);
	}

	// keep our write end while closing the unused read end.
	__mldr_close_process_lifetime_pipe(lifetime_pipe[0]);

	if (!lr->root_path) {
		static char vchroot_buffer[4096];
		uint64_t vchroot_path_length = 0;

		int code = dserver_rpc_vchroot_path(vchroot_buffer, sizeof(vchroot_buffer), &vchroot_path_length);
		if (code < 0) {
			fprintf(stderr, "Failed to retrieve vchroot path from darlingserver: %d\n", code);
			exit(1);
		}

		if (vchroot_path_length >= sizeof(vchroot_buffer)) {
			fprintf(stderr, "Vchroot path is too large for buffer\n");
			exit(1);
		} else if (vchroot_path_length > 0) {
			lr->root_path = vchroot_buffer;
			lr->root_path_length = vchroot_path_length;
		}
	}
};

// The code signing state darlingserver recorded for this process; false when
// it cannot be read.
static bool own_code_signing_state(dserver_cs_state_t* state) {
	return dserver_rpc_cs_get_state(0, state, sizeof(*state)) == 0;
}

// What is wrong with the registered image's signature, or NULL if nothing is.
static const char* code_signing_problem(void) {
	dserver_cs_state_t state;

	if (!own_code_signing_state(&state))
		return "could not be checked";

	switch (state.status) {
		case dserver_cs_status_valid: return NULL;
		case dserver_cs_status_unsigned: return "is missing";
		case dserver_cs_status_invalid: return "is not valid";
		default: return "could not be checked";
	}
}

// Refuses to start the executable. On macOS, a program whose code signature
// is rejected at launch does not make exec fail with an errno: the new process
// is killed with SIGKILL (crash reports name the CODESIGNING termination
// namespace, see Apple's "Understanding the exception types in a crash
// report"). By the time mldr runs, the Linux exec has happened as well, so
// mldr does the same, before anything of the image is mapped. The message is
// Darling's addition; macOS logs the reason to the system log instead.
static void __attribute__((noreturn)) refuse_executable(const char* problem) {
	fprintf(stderr, "mldr: %s: refusing to run it: its code signature %s\n",
		exec_path ? exec_path : "(unknown)", problem);
	fflush(stderr);
	kill(getpid(), SIGKILL);
	_exit(1);
}

// Hands the executable about to be mapped (the slice at `image_offset`) to
// darlingserver, which reads its code signature and records it as the
// process's signing state, the state csops() reports. This runs on every exec,
// after the check-in and before anything is mapped. What happens next depends
// on the process's code signing policy (`CodeSigningPolicy` in the
// org.darlinghq.security domain, as darlingserver recorded it when the
// process started):
//   - off (the default): nothing; the result is not looked at;
//   - warn: an image that `enforce` would refuse is reported on stderr, and
//     runs;
//   - enforce: an image darlingserver does not report as validly signed is
//     not started (refuse_executable()).
// An unknown policy value is treated as `enforce`. If darlingserver cannot be
// asked for the policy, it is taken to be `off`, as in dyld.
//
// Under `enforce`, the whole file is first copied into a sealed snapshot
// (mldr_snapshot_image()); darlingserver checks the snapshot and the image is
// mapped from it, so the file changing between the check and the mapping
// does not matter. This costs a copy of the file in memory, which is why it is
// done only under `enforce`. Without it, the file is checked and mapped as it
// is.
//
// Whatever the policy, the read-only segments of the images mldr maps are
// sealed (`lr->seal_images`, see seal_segment()) when `SealImages` is on and
// the program uses the hardened runtime (CS_RUNTIME) without the
// get-task-allow entitlement, i.e. when it is not meant to be debugged. dyld
// applies the same rule to the images it loads. Not in 32-bit processes,
// which Linux has no mseal() for.
//
// Returns the descriptor to map the image from, positioned at `image_offset`:
// `fd` itself, or the snapshot, which the caller closes after mapping.
static int code_signing_gate(int fd, uint64_t image_offset, struct load_results* lr) {
	dserver_cs_state_t state;
	uint32_t policy = dserver_code_signing_policy_off;
	uint32_t flags = 0;
	int code;

	if (own_code_signing_state(&state))
		policy = state.code_signing_policy;
	if (policy > dserver_code_signing_policy_enforce)
		policy = dserver_code_signing_policy_enforce;

	if (policy == dserver_code_signing_policy_enforce) {
		int snapshot = mldr_snapshot_image(fd);
		if (snapshot < 0) {
			char problem[128];
			snprintf(problem, sizeof(problem), "could not be checked (the file cannot be copied: %s)", strerror(-snapshot));
			refuse_executable(problem);
		}
		if (lseek(snapshot, (off_t)image_offset, SEEK_SET) != (off_t)image_offset)
			refuse_executable("could not be checked (the image is outside the file)");
		fd = snapshot;
	}

	code = dserver_rpc_cs_register_image(fd, image_offset, &flags);

	if (policy == dserver_code_signing_policy_enforce) {
		// under enforce, darlingserver reports CS_VALID only for a valid signature
		if (code < 0)
			refuse_executable("could not be checked");
		if (!(flags & CS_VALID)) {
			const char* problem = code_signing_problem();
			refuse_executable(problem ? problem : "is not valid");
		}
	} else if (policy == dserver_code_signing_policy_warn) {
		// without enforce, darlingserver reports CS_VALID for every image, so the status tells
		const char* problem = (code < 0) ? "could not be checked" : code_signing_problem();
		if (problem) {
			fprintf(stderr, "mldr: warning: %s: its code signature %s; "
				"code signing enforcement would refuse to run it\n",
				exec_path ? exec_path : "(unknown)", problem);
		}
	}

#ifndef __i386__
	// the configuration is read only for a program the setting could apply to
	if (code == 0 && (flags & (CS_VALID | CS_RUNTIME | CS_GET_TASK_ALLOW)) == (CS_VALID | CS_RUNTIME)) {
		dserver_system_config_t config;
		if (dserver_rpc_get_system_config(&config, sizeof(config)) == 0 && config.seal_images)
			lr->seal_images = true;
	}
#else
	(void) lr;
#endif

	return fd;
}

// Seals a read-only segment that was just mapped (see code_signing_gate()):
// it can no longer be made writable, unmapped or replaced. Only the part that
// is mapped from the file is sealed. A host kernel without mseal() (before
// Linux 6.10) leaves the segment unsealed, without a message (dyld reports
// that once per process); other failures are reported and the program runs.
static void seal_segment(const char* name, uintptr_t address, uint64_t file_size, int prot) {
	int ret = mldr_seal_range((void*)address, (size_t)file_size, prot);
	if (ret < 0 && ret != -ENOSYS) {
		fprintf(stderr, "mldr: warning: cannot seal %.16s of %s: %s\n", name,
			exec_path ? exec_path : "(unknown)", strerror(-ret));
	}
}

static void start_thread(struct load_results* lr) {
#ifdef __x86_64__
	__asm__ volatile(
		"mov %1, %%rsp\n"
		"jmpq *%0"
		::
		"m"(lr->entry_point),
		"r"(lr->stack_top)
		:
	);
#elif defined(__i386__)
	__asm__ volatile(
		"mov %1, %%esp\n"
		"jmp *%0"
		::
		"m"(lr->entry_point),
		"r"(lr->stack_top)
		:
	);
#elif defined(__aarch64__)
	__asm__ volatile(
		"mov sp, %1\n"
		"br %0"
		::
		"r"(lr->entry_point),
		"r"(lr->stack_top)
		:
	);
#elif defined(__arm__)
	__asm__ volatile(
		"mov sp, %1\n"
		"bx %0"
		::
		"r"(lr->entry_point),
		"r"(lr->stack_top)
		:
	);
#else
#       error Unsupported platform!
#endif
};

static bool is_kernel_at_least(int major, int minor) {
	if (kernel_major == -1) {
		struct utsname uname_info;
		if (uname(&uname_info) == -1) {
			return false;
		}
		kernel_major = 0;
		kernel_minor = 0;
		size_t pos = 0;
		while (uname_info.release[pos] != '\0' && uname_info.release[pos] != '.') {
			kernel_major = kernel_major * 10 + uname_info.release[pos] - '0';
			++pos;
		}
		++pos;
		while (uname_info.release[pos] != '\0' && uname_info.release[pos] != '.') {
			kernel_minor = kernel_minor * 10 + uname_info.release[pos] - '0';
			++pos;
		}
	}

	if (major != kernel_major) {
		return kernel_major > major;
	}

	return kernel_minor >= minor;
}

void* compatible_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
	// MAP_FIXED_NOREPLACE is not supported on WSL1 (Linux < 4.17).
	bool fixed_noreplace_hack = false;
	if ((flags & MAP_FIXED_NOREPLACE) && !is_kernel_at_least(4, 17)) {
		flags &= ~MAP_FIXED_NOREPLACE;
		fixed_noreplace_hack = true;
	}
	void* result = mmap(addr, length, prot, flags, fd, offset);
	// MAP_GROWSDOWN is not supported on WSL1. See https://github.com/microsoft/WSL/issues/8095.
	if ((result == (void*)MAP_FAILED) && (flags & MAP_GROWSDOWN) && (errno == EOPNOTSUPP)) {
		result = mmap(addr, length, prot, (flags & ~MAP_GROWSDOWN), fd, offset);
	}
	if (fixed_noreplace_hack) {
		if (result != addr && result != (void*)MAP_FAILED) {
			errno = ESRCH;
			munmap(addr, length);
			return MAP_FAILED;
		}
	}
	return result;
}
