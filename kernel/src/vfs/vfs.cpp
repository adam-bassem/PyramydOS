#include "vfs.hpp"
#include <allocator/allocator.hpp>
#include <console/console.hpp>
#include <utils.hpp>

namespace
{
	struct vfs_node_entry
	{
		namespace_node* node;
		vfs_node_entry* prev;
		vfs_node_entry* next;
	} *first_vfs_node;
	int n_nodes = 0;

	vfs_node_entry* last_vfs_node()
	{
		vfs_node_entry* last = first_vfs_node;

		if (!last) return last;

		while (last->next)
		{
			last = last->next;
		}

		return last;
	}

	void add_vfs_node(namespace_node* node)
	{
		vfs_node_entry* last = last_vfs_node();

		if (!last)
		{
			last = reinterpret_cast<vfs_node_entry*>(alloc::malloc(sizeof(vfs_node_entry)));
			if (!last)
			{
				console::kprintf(ANSI_BOLD ANSI_RED "Out of memory... failed to allocate first VFS node");
				console::kprintf("Halting...");
				hcf();
			}

			last->prev = nullptr;
			last->next = nullptr;
			last->node = node;

			first_vfs_node = last;

			n_nodes++;

			return;
		}

		last->next = reinterpret_cast<vfs_node_entry*>(alloc::malloc(sizeof(vfs_node_entry)));
		if (!last->next)
		{
			console::kprintf(ANSI_BOLD ANSI_RED "Out of memory... failed to allocate VFS node");
			console::kprintf("Halting...");
			hcf();
		}

		last->next->prev = last;
		last->next->next = nullptr;
		last->next->node = node;

		n_nodes++;
	}

	void delete_node(vfs_node_entry* entry)
	{
		// assume the namespace node was already cleared
		if (entry)
		{
			if (entry->prev)
				entry->prev->next = entry->next;
			else
				first_vfs_node = entry->next;
			if (entry->next)
				entry->next->prev = entry->prev;

			alloc::free(entry);

			n_nodes--;
		}
	}

	vfs_node_entry* node_at(int at)
	{
		vfs_node_entry* curr = first_vfs_node;

		for (int i = 0; i < at; i++)
		{
			if (!curr->next) return curr;
			curr = curr->next;
		}

		return curr;
	}

	void set_node_name(namespace_node* node, const char* name)
	{
		int len = strlen(name);
		if (len > (int)sizeof(node->name) - 1) len = sizeof(node->name) - 1;

		memcpy(node->name, name, len);
		node->name[len] = 0;
	}
}

void vfs::init()
{
	// init and create ramfs

	namespace_node* node = reinterpret_cast<namespace_node*>(alloc::malloc(sizeof(namespace_node)));
	if (!node)
	{
		console::kprintf(ANSI_BOLD ANSI_RED "Out of memory... failed to allocate first namespace node for VFS (ramfs:/)");
		console::kprintf("Halting...");
		hcf();
	}
	set_node_name(node, "ramfs");
	node->fs_type = FS_RAM;
	node->uuid = uuid_gen();
	node->disk_number = VFS_NO_DISK;
	node->partition_number = VFS_NO_PART;

	add_vfs_node(node);

	console::kprintf("Created ramfs:/ namespace in VFS");
}

namespace_node* vfs::create_node(const char* node_name)
{
	namespace_node* node = reinterpret_cast<namespace_node*>(alloc::malloc(sizeof(namespace_node)));
	if (!node)
	{
		console::kprintf(ANSI_BOLD ANSI_RED "Out of memory... failed to allocate namespace node for VFS");
		console::kprintf("Halting...");
		hcf();
	}
	set_node_name(node, node_name);
	node->fs_type = FS_RAM;
	node->uuid = uuid_gen();
	node->disk_number = VFS_NO_DISK;
	node->partition_number = VFS_NO_PART;

	add_vfs_node(node);

	return node;
}

void vfs::destroy_node(namespace_node* node)
{
	for (int i = 0; i < n_nodes; i++)
	{
		vfs_node_entry* entry = node_at(i);

		if (entry->node == node)
		{
			delete_node(entry);
			alloc::free(node);
			return;
		}
	}
}

void vfs::list()
{
	for (int i = 0; i < n_nodes; i++)
	{
		namespace_node* node = node_at(i)->node;
	
		console::print_timestamp();
		printf("%s:/ ", node->name);
		print_uuid(node->uuid);
	}
}
