/*
 * Copyright (c) 1998-2015 Erez Zadok
 * Copyright (c) 2009	   Shrikar Archak
 * Copyright (c) 2003-2015 Stony Brook University
 * Copyright (c) 2003-2015 The Research Foundation of SUNY
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "trfs.h"
#include <linux/module.h>

int process_raw_data_and_create_the_file(char *temp_raw_data, struct super_block *sb)
{
	int ret=0;
	struct trfs_sb_info *sb_info;
	//char* temp_raw_data = (char* )raw_data;
	mm_segment_t old_fs;
	struct file *trace_file=NULL;
	unsigned long long offset;
	int record_id;
	old_fs = get_fs();
	set_fs(get_ds());
	if (temp_raw_data==NULL)
	{
		printk("\nMissing arguments");
		ret=-EINVAL;
		goto out;
	}	
	
	trace_file = filp_open(temp_raw_data, O_CREAT | O_WRONLY | O_TRUNC , 0644);
	set_fs(old_fs);
	if (IS_ERR(trace_file)) {
		printk("Count't create the file\n");
		ret= PTR_ERR(trace_file);
	}

	offset=0;
	record_id=0;
	
	sb_info = (struct trfs_sb_info *)kzalloc(sizeof(struct trfs_sb_info), GFP_KERNEL);
	if(sb_info == NULL){
		ret = -ENOMEM;
		goto out;
	}
	
	printk("Before sb info\n");
	sb_info = (struct trfs_sb_info *)sb->s_fs_info;
	if(sb_info == NULL){
		printk("Somehow sb_info is NULL\n");
		goto out;
	}
	sb_info->tracefile = (struct trfs_tracefile_info*)kzalloc(sizeof(struct trfs_tracefile_info) , GFP_KERNEL);
	sb_info->tracefile->filename = trace_file;
	sb_info->tracefile->offset = offset;
	sb_info->tracefile->record_id = record_id;
	mutex_init(&sb_info->tracefile->record_lock);
	
	printk("Reached successfully at the end of create trace file function\n");
	out:
		return ret;
}


/*
 * There is no need to lock the wrapfs_super_info's rwsem as there is no
 * way anyone can have a reference to the superblock at this point in time.
 */
static int trfs_read_super(struct super_block *sb, void *raw_data, int silent)
{
	int err = 0;
	struct super_block *lower_sb;
	struct path lower_path;
	//char *dev_name = (char *) raw_data;
	struct inode *inode;
	struct trfs_options_data *options_data;
	char *dev_name;
	
	options_data = (struct trfs_options_data *)raw_data;
	dev_name = options_data->lower_fs_path;
	if (!dev_name) {
		printk(KERN_ERR
		       "trfs: read_super: missing dev_name argument\n");
		err = -EINVAL;
		goto out;
	}

	/* parse lower path */
	err = kern_path(dev_name, LOOKUP_FOLLOW | LOOKUP_DIRECTORY,
			&lower_path);
	if (err) {
		printk(KERN_ERR	"trfs: error accessing "
		       "lower directory '%s'\n", dev_name);
		goto out;
	}

	/* allocate superblock private data */
	sb->s_fs_info = kzalloc(sizeof(struct trfs_sb_info), GFP_KERNEL);
	if (!TRFS_SB(sb)) {
		printk(KERN_CRIT "trfs: read_super: out of memory\n");
		err = -ENOMEM;
		goto out_free;
	}

	err = process_raw_data_and_create_the_file(options_data->tracefile_path, sb);
	if (err) {
		printk(KERN_ERR	"trfs: error creating trace file "
		       "at location '%s'\n", options_data->tracefile_path);
		goto out;
	}
	

	/* set the lower superblock field of upper superblock */
	lower_sb = lower_path.dentry->d_sb;
	atomic_inc(&lower_sb->s_active);
	trfs_set_lower_super(sb, lower_sb);

	/* inherit maxbytes from lower file system */
	sb->s_maxbytes = lower_sb->s_maxbytes;

	/*
	 * Our c/m/atime granularity is 1 ns because we may stack on file
	 * systems whose granularity is as good.
	 */
	sb->s_time_gran = 1;

	sb->s_op = &trfs_sops;

	sb->s_export_op = &trfs_export_ops; /* adding NFS support */

	/* get a new inode and allocate our root dentry */
	inode = trfs_iget(sb, d_inode(lower_path.dentry));
	if (IS_ERR(inode)) {
		err = PTR_ERR(inode);
		goto out_sput;
	}
	sb->s_root = d_make_root(inode);
	if (!sb->s_root) {
		err = -ENOMEM;
		goto out_iput;
	}
	d_set_d_op(sb->s_root, &trfs_dops);

	/* link the upper and lower dentries */
	sb->s_root->d_fsdata = NULL;
	err = new_dentry_private_data(sb->s_root);
	if (err)
		goto out_freeroot;

	/* if get here: cannot have error */

	/* set the lower dentries for s_root */
	trfs_set_lower_path(sb->s_root, &lower_path);

	/*
	 * No need to call interpose because we already have a positive
	 * dentry, which was instantiated by d_make_root.  Just need to
	 * d_rehash it.
	 */
	d_rehash(sb->s_root);
	if (!silent)
		printk(KERN_INFO
		       "trfs: mounted on top of %s type %s\n",
		       dev_name, lower_sb->s_type->name);
	goto out; /* all is well */

	/* no longer needed: free_dentry_private_data(sb->s_root); */
out_freeroot:
	dput(sb->s_root);
out_iput:
	iput(inode);
out_sput:
	/* drop refs we took earlier */
	atomic_dec(&lower_sb->s_active);
	kfree(TRFS_SB(sb));
	sb->s_fs_info = NULL;
out_free:
	path_put(&lower_path);

out:
	return err;
}

struct dentry *trfs_mount(struct file_system_type *fs_type, int flags,
			    const char *dev_name, void *raw_data)
{
	//void *lower_path_name = (void *) dev_name;
	struct trfs_options_data *options_data;
	void *raw_options_data;
	char *temp_raw_data;

 	temp_raw_data = (char *)raw_data;
	options_data = (struct trfs_options_data*)kmalloc(sizeof(struct trfs_options_data), GFP_KERNEL);	
	options_data->lower_fs_path = (char *)dev_name;
	options_data->tracefile_path = (char *)raw_data;
  
	//int return_function=0;

	//return_function = process_raw_data_and_create_the_file(raw_data);	
	//return_function = process_raw_data_and_create_the_file(temp_raw_data);

	printk("######dev_name is - %s\n", dev_name);	
	printk("######Raw data is - %s\n", temp_raw_data);
	
	raw_options_data = (void *)options_data;		
	return mount_nodev(fs_type, flags, raw_options_data,
			   trfs_read_super);
} 

static struct file_system_type trfs_fs_type = {
	.owner		= THIS_MODULE,
	.name		= TRFS_NAME,
	.mount		= trfs_mount,
	.kill_sb	= generic_shutdown_super,
	.fs_flags	= 0,
};
MODULE_ALIAS_FS(TRFS_NAME);

static int __init init_trfs_fs(void)
{
	int err;

	pr_info("Registering trfs " TRFS_VERSION "\n");

	err = trfs_init_inode_cache();
	if (err)
		goto out;
	err = trfs_init_dentry_cache();
	if (err)
		goto out;
	err = register_filesystem(&trfs_fs_type);
out:
	if (err) {
		trfs_destroy_inode_cache();
		trfs_destroy_dentry_cache();
	}
	return err;
}

static void __exit exit_trfs_fs(void)
{
	trfs_destroy_inode_cache();
	trfs_destroy_dentry_cache();
	unregister_filesystem(&trfs_fs_type);
	pr_info("Completed trfs module unload\n");
}

MODULE_AUTHOR("Erez Zadok, Filesystems and Storage Lab, Stony Brook University"
	      " (http://www.fsl.cs.sunysb.edu/)");
MODULE_DESCRIPTION("Trfs " TRFS_VERSION
		   " (http://trfs.filesystems.org/)");
MODULE_LICENSE("GPL");

module_init(init_trfs_fs);
module_exit(exit_trfs_fs);
