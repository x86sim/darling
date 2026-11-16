// Copyright (C) 2026 Lubos Dolezel
//
// A 64-bit process must be able to start a 32-bit one with posix_spawn and
// fork+exec, plain and with POSIX_SPAWN_START_SUSPENDED (resumed by SIGCONT).
// argv[1] = path of an i386 program that exits with status 0.
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <spawn.h>
#include <sys/wait.h>
#include <mach/machine.h>

extern char** environ;
static int failures;
static void check(int ok, const char* what) { printf("%s: %s\n", ok ? "PASS" : "FAIL", what); if (!ok) failures++; }

int main(int argc, char** argv)
{
	setvbuf(stdout, NULL, _IONBF, 0);
	if (argc < 2) return 2;
	char* args[] = { argv[1], NULL };
	int st = 0;
	pid_t p;

	int rc = posix_spawn(&p, argv[1], NULL, NULL, args, environ);
	check(rc == 0, "posix_spawn i386");
	pid_t r = waitpid(p, &st, 0);
	check(r == p && WIFEXITED(st) && WEXITSTATUS(st) == 0, "posix_spawn i386: exit status 0");

	p = fork();
	if (p == 0) { execv(argv[1], args); _exit(99); }
	r = waitpid(p, &st, 0);
	check(r == p && WIFEXITED(st) && WEXITSTATUS(st) == 0, "fork+exec i386: exit status 0");

	posix_spawnattr_t at; posix_spawnattr_init(&at);
	posix_spawnattr_setflags(&at, POSIX_SPAWN_START_SUSPENDED);
	rc = posix_spawn(&p, argv[1], NULL, &at, args, environ);
	check(rc == 0, "suspended posix_spawn i386");
	usleep(300000);
	kill(p, SIGKILL);
	r = waitpid(p, &st, 0);
	check(r == p && WIFSIGNALED(st) && WTERMSIG(st) == SIGKILL, "suspended i386 child can be killed and reaped");
	// flags used by debuggers: no ASLR, close-on-exec default, architecture preference
	static const struct { int flags; const char* name; } cases[] = {
		{ 0x0100, "no-ASLR" },
		{ POSIX_SPAWN_CLOEXEC_DEFAULT, "cloexec-default" },
		{ POSIX_SPAWN_SETSIGMASK, "sigmask" },
	};
	for (unsigned i = 0; i < sizeof cases / sizeof *cases; i++)
	{
		char msg[96];
		posix_spawnattr_init(&at);
		posix_spawnattr_setflags(&at, cases[i].flags);
		sigset_t none; sigemptyset(&none);
		posix_spawnattr_setsigmask(&at, &none);
		rc = posix_spawn(&p, argv[1], NULL, &at, args, environ);
		r = rc == 0 ? waitpid(p, &st, 0) : -1;
		snprintf(msg, sizeof msg, "i386 child spawned with %s exits with status 0", cases[i].name);
		check(rc == 0 && r == p && WIFEXITED(st) && WEXITSTATUS(st) == 0, msg);
	}
	posix_spawnattr_init(&at);
	cpu_type_t pref = CPU_TYPE_I386; size_t cnt = 1;
	posix_spawnattr_setbinpref_np(&at, 1, &pref, &cnt);
	rc = posix_spawn(&p, argv[1], NULL, &at, args, environ);
	r = rc == 0 ? waitpid(p, &st, 0) : -1;
	check(rc == 0 && r == p && WIFEXITED(st) && WEXITSTATUS(st) == 0, "i386 child spawned with binpref i386 exits with status 0");
	return failures != 0;
}
