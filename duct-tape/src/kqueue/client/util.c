#include <sys/systm.h>
#include <elfcalls.h>

#undef kheap_alloc
#undef kheap_free

extern struct elf_calls* _elfcalls;

void* kheap_alloc(kalloc_heap_t kheap, vm_size_t size, zalloc_flags_t flags) {
	void* ptr = _elfcalls->malloc(size);
	if (!ptr) {
		return ptr;
	}

	if (flags & Z_ZERO) {
		memset(ptr, 0, size);
	}

	return ptr;
};

void kheap_free(kalloc_heap_t kheap, void* addr, vm_size_t size) {
	_elfcalls->free(addr);
};

// <copied from="xnu://7195.141.2/osfmk/x86_64/loose_ends.c">

/*
 * Find last bit set in bit string.
 */
int
fls(unsigned int mask)
{
	if (mask == 0) {
		return 0;
	}

	return (sizeof(mask) << 3) - __builtin_clz(mask);
}

// </copied>

// <copied from="xnu://7195.141.2/bsd/kern/kern_subr.c">

LIST_HEAD(generic_hash_head, generic);

/*
 * General routine to allocate a hash table.
 */
void *
hashinit(int elements, int type __unused, u_long *hashmask)
{
	struct generic_hash_head *hashtbl;
	vm_size_t hashsize;

	if (elements <= 0) {
		panic("hashinit: bad cnt");
	}

	hashsize = 1UL << (fls(elements) - 1);
	hashtbl = kheap_alloc(KHEAP_DEFAULT, hashsize * sizeof(*hashtbl),
	    Z_WAITOK | Z_ZERO);
	if (hashtbl != NULL) {
		*hashmask = hashsize - 1;
	}
	return hashtbl;
}

void
hashdestroy(void *hash, int type __unused, u_long hashmask)
{
	struct generic_hash_head *hashtbl = hash;
	assert(powerof2(hashmask + 1));
	kheap_free(KHEAP_DEFAULT, hashtbl, (hashmask + 1) * sizeof(*hashtbl));
}

// </copied>
