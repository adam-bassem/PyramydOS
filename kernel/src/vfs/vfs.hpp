#pragma once

#include <utils.hpp> // for UUID only :>

#define VFS_NO_DISK ((uint32_t)-1)
#define VFS_NO_PART ((uint32_t)-1)

enum filesystem_type
{
	FS_RAM,
};

struct namespace_node
{
	char name[16];

	filesystem_type fs_type;

	UUID uuid;
	uint32_t disk_number; // if applicable
	uint32_t partition_number; // if applicable
};

namespace vfs
{
	void init();
	namespace_node* create_node(const char* node_name);
	void destroy_node(namespace_node* node);
	void list();
}
