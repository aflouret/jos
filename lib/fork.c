// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW 0x800

extern void _pgfault_upcall(void);

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	if ((err & FEC_WR) == 0 || (err & FEC_PR) == 0 ||
	    (uvpt[(uint32_t) addr / PGSIZE] & PTE_COW) == 0)
		panic("pgfault");

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	sys_page_alloc(0, PFTEMP, PTE_U | PTE_P | PTE_W);
	memmove(PFTEMP, ROUNDDOWN(addr, PGSIZE), PGSIZE);
	sys_page_map(0, PFTEMP, 0, ROUNDDOWN(addr, PGSIZE), PTE_U | PTE_P | PTE_W);
	sys_page_unmap(0, PFTEMP);
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;
	int perm = uvpt[pn] & PTE_SYSCALL;

	void *addr = (void *) (pn * PGSIZE);

	if ((r = sys_page_map(0, addr, envid, addr, perm)) < 0)
		return r;

	if (perm & PTE_W || perm & PTE_COW) {
		perm |= PTE_COW;
		perm &= ~PTE_W;
		if ((r = sys_page_map(0, addr, 0, addr, perm)) < 0)
			return r;
		if ((r = sys_page_map(envid, addr, envid, addr, perm)) < 0)
			return r;
	}

	return 0;
}

static void
dup_or_share(envid_t dstenv, void *va, int perm)
{
	perm &= PTE_SYSCALL;
	int r;
	if ((perm & PTE_W) == 0) {
		sys_page_map(0, va, dstenv, va, perm);
		return;
	}
	sys_page_alloc(dstenv, va, perm);
	sys_page_map(dstenv, va, 0, UTEMP, perm);
	memmove(UTEMP, va, PGSIZE);
	sys_page_unmap(0, UTEMP);
}

envid_t
fork_v0(void)
{
	envid_t envid;
	uint8_t *addr;
	int r;

	envid = sys_exofork();
	if (envid < 0)
		panic("sys_exofork: %e", envid);
	if (envid == 0) {
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}

	for (addr = 0; (uint32_t) addr < UTOP; addr += PGSIZE) {
		int perm = PTE_U | PTE_P;
		pde_t pde = uvpd[PDX(addr)];
		if ((pde & PTE_P) == 0)
			continue;

		pte_t pte = uvpt[PGNUM(addr)];
		if ((pte & PTE_P) == 0)
			continue;

		if (pte & PTE_W)
			perm |= PTE_W;

		dup_or_share(envid, addr, perm);
	}

	if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
		panic("sys_env_set_status: %e", r);

	return envid;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	envid_t envid;
	uint8_t *addr;
	int r;

	set_pgfault_handler(pgfault);

	envid = sys_exofork();
	if (envid < 0)
		panic("sys_exofork: %e", envid);
	if (envid == 0) {
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}

	for (addr = 0; (uint32_t) addr < UXSTACKTOP - PGSIZE; addr += PGSIZE) {
		pde_t pde = uvpd[PDX(addr)];

		if ((pde & PTE_P) == 0) {
			addr += PTSIZE - PGSIZE;
			continue;
		}

		pte_t pte = uvpt[PGNUM(addr)];
		if ((pte & PTE_P) == 0)
			continue;

		if (duppage(envid, (unsigned) addr / PGSIZE) < 0)
			panic("duppage");
	}

	sys_page_alloc(envid, (void *) (UXSTACKTOP - PGSIZE), PTE_P | PTE_U | PTE_W);
	sys_env_set_pgfault_upcall(envid, _pgfault_upcall);

	if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
		panic("sys_env_set_status: %e", r);

	return envid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
