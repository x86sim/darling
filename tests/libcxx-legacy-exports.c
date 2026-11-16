// Copyright (C) 2026 Lubos Dolezel
//
// libc++.1.dylib must keep exporting the out-of-line copies of functions that
// later libc++ headers made inline (shared_ptr reference counting here), so
// that binaries built against older libc++ headers still bind.
//   clang -o /tmp/libcxx-legacy-exports tests/libcxx-legacy-exports.c   (run inside Darling; any arch)
// Exits non-zero on failure.
#include <dlfcn.h>
#include <stdio.h>

static const char* const names[] = {
	"_ZNSt3__119__shared_weak_count12__add_sharedEv",
	"_ZNSt3__119__shared_weak_count10__add_weakEv",
	"_ZNSt3__119__shared_weak_count16__release_sharedEv",
	"_ZNSt3__114__shared_count12__add_sharedEv",
	"_ZNSt3__114__shared_count16__release_sharedEv",
};

int main(void)
{
	void* h = dlopen("/usr/lib/libc++.1.dylib", RTLD_NOW);
	int fail = 0;
	if (!h) { printf("FAIL: dlopen libc++\n"); return 1; }
	for (unsigned i = 0; i < sizeof names / sizeof *names; i++) {
		int ok = dlsym(h, names[i]) != NULL;
		printf("%s: export %s\n", ok ? "PASS" : "FAIL", names[i]);
		fail |= !ok;
	}
	return fail;
}
