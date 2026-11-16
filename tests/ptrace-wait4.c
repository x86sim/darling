// Copyright (C) 2026 Lubos Dolezel
//
// A process that attached to another one with PT_ATTACH or PT_ATTACHEXC can
// wait4() it although it is not its child (on macOS the tracer is the
// effective parent until it detaches), and sees its exit status. After
// PT_DETACH, or for a process it never attached to, wait4 fails with ECHILD.
//   clang -o /tmp/ptrace-wait4 tests/ptrace-wait4.c   (run inside Darling)
// Exits non-zero on failure.
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

static int failures;
static void check(int ok, const char* what) { printf("%s: %s\n", ok ? "PASS" : "FAIL", what); if (!ok) failures++; }

static pid_t spawn_sleeper(int seconds, int status)
{
	pid_t p = fork();
	if (p == 0) { sleep(seconds); _exit(status); }
	return p;
}

int main(void)
{
	setvbuf(stdout, NULL, _IONBF, 0);
	// targets are children of this process; the tracer is a sibling, so they are not its children
	pid_t t_exit = spawn_sleeper(3, 5);      // PT_ATTACH, continued, exits with status 5
	pid_t t_kill = spawn_sleeper(60, 0);     // PT_ATTACHEXC, killed
	pid_t t_det = spawn_sleeper(60, 0);      // attached then detached
	pid_t t_other = spawn_sleeper(60, 0);    // never attached

	pid_t r = fork();
	if (r == 0)
	{
		int st = 0;
		int rc = ptrace(PT_ATTACH, t_exit, 0, 0);
		check(rc == 0, "PT_ATTACH succeeds");
		usleep(300000); // let the SIGSTOP of the attach take effect
		ptrace(PT_CONTINUE, t_exit, (caddr_t) 1, 0);
		pid_t w = wait4(t_exit, &st, 0, NULL);
		check(w == t_exit && WIFEXITED(st) && WEXITSTATUS(st) == 5, "wait4 on an attached non-child returns its exit status 5");
		w = wait4(t_exit, &st, 0, NULL);
		check(w == -1 && errno == ECHILD, "a second wait4 for the same process fails with ECHILD");

		rc = ptrace(PT_ATTACHEXC, t_kill, 0, 0);
		check(rc == 0, "PT_ATTACHEXC succeeds");
		kill(t_kill, SIGKILL);
		w = wait4(t_kill, &st, 0, NULL);
		check(w == t_kill && WIFSIGNALED(st) && WTERMSIG(st) == SIGKILL, "wait4 on an attached non-child returns death by SIGKILL");

		rc = ptrace(PT_ATTACHEXC, t_det, 0, 0);
		check(rc == 0, "PT_ATTACHEXC (to be detached) succeeds");
		rc = ptrace(PT_DETACH, t_det, 0, 0);
		check(rc == 0, "PT_DETACH succeeds");
		w = wait4(t_det, &st, WNOHANG, NULL);
		check(w == -1 && errno == ECHILD, "wait4 after PT_DETACH fails with ECHILD");

		w = wait4(t_other, &st, WNOHANG, NULL);
		check(w == -1 && errno == ECHILD, "wait4 on a process never attached to fails with ECHILD");

		rc = ptrace(PT_ATTACHEXC, t_det, 0, 0);
		check(rc == 0, "attaching again after PT_DETACH succeeds");
		ptrace(PT_DETACH, t_det, 0, 0);
		_exit(failures != 0);
	}
	int st = 0;
	waitpid(r, &st, 0);
	check(WIFEXITED(st) && WEXITSTATUS(st) == 0, "tracer process finished all checks");
	kill(t_det, SIGKILL); kill(t_other, SIGKILL);
	kill(t_kill, SIGKILL); kill(t_exit, SIGKILL);
	int rest;
	while (waitpid(-1, &rest, 0) > 0) {}
	return failures != 0;
}

