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
#include <linux/namei.h>
#include "record.h"

static int trfs_create(struct inode *dir, struct dentry *dentry,
			 umode_t mode, bool want_excl)
{
	int err;
	struct dentry *lower_dentry;
	struct dentry *lower_parent_dentry = NULL;
	struct path lower_path;
	printk("Trfs_Create called\n");

	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	lower_parent_dentry = lock_parent(lower_dentry);

	err = vfs_create(d_inode(lower_parent_dentry), lower_dentry, mode,
			 want_excl);
	if (err)
		goto out;
	err = trfs_interpose(dentry, dir->i_sb, &lower_path);
	if (err)
		goto out;
	fsstack_copy_attr_times(dir, trfs_lower_inode(dir));
	fsstack_copy_inode_size(dir, d_inode(lower_parent_dentry));

out:
	unlock_dir(lower_parent_dentry);
	trfs_put_lower_path(dentry, &lower_path);
	return err;
}

static int trfs_link(struct dentry *old_dentry, struct inode *dir,
		       struct dentry *new_dentry)
{
	//printk("Trfs_Link called");
	
	struct dentry *lower_old_dentry;
	struct dentry *lower_new_dentry;
	struct dentry *lower_dir_dentry;
	u64 file_size_save;
	int err;
	struct path lower_old_path, lower_new_path;

	file_size_save = i_size_read(d_inode(old_dentry));
	trfs_get_lower_path(old_dentry, &lower_old_path);
	trfs_get_lower_path(new_dentry, &lower_new_path);
	lower_old_dentry = lower_old_path.dentry;
	lower_new_dentry = lower_new_path.dentry;
	lower_dir_dentry = lock_parent(lower_new_dentry);

	err = vfs_link(lower_old_dentry, d_inode(lower_dir_dentry),
		       lower_new_dentry, NULL);
	if (err || !d_inode(lower_new_dentry))
		goto out;

	err = trfs_interpose(new_dentry, dir->i_sb, &lower_new_path);
	if (err)
		goto out;
	fsstack_copy_attr_times(dir, d_inode(lower_new_dentry));
	fsstack_copy_inode_size(dir, d_inode(lower_new_dentry));
	set_nlink(d_inode(old_dentry),
		  trfs_lower_inode(d_inode(old_dentry))->i_nlink);
	i_size_write(d_inode(new_dentry), file_size_save);
out:
	unlock_dir(lower_dir_dentry);
	trfs_put_lower_path(old_dentry, &lower_old_path);
	trfs_put_lower_path(new_dentry, &lower_new_path);
	return err;
}

static int trfs_unlink(struct inode *dir, struct dentry *dentry)
{
	printk("Trfs_Unlink called\n");

	int err;
	struct dentry *lower_dentry;
	struct inode *lower_dir_inode = trfs_lower_inode(dir);
	struct dentry *lower_dir_dentry;
	struct path lower_path;

	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	dget(lower_dentry);
	lower_dir_dentry = lock_parent(lower_dentry);

	err = vfs_unlink(lower_dir_inode, lower_dentry, NULL);

	/*
	 * Note: unlinking on top of NFS can cause silly-renamed files.
	 * Trying to delete such files results in EBUSY from NFS
	 * below.  Silly-renamed files will get deleted by NFS later on, so
	 * we just need to detect them here and treat such EBUSY errors as
	 * if the upper file was successfully deleted.
	 */
	if (err == -EBUSY && lower_dentry->d_flags & DCACHE_NFSFS_RENAMED)
		err = 0;
	if (err)
		goto out;
	fsstack_copy_attr_times(dir, lower_dir_inode);
	fsstack_copy_inode_size(dir, lower_dir_inode);
	set_nlink(d_inode(dentry),
		  trfs_lower_inode(d_inode(dentry))->i_nlink);
	d_inode(dentry)->i_ctime = dir->i_ctime;
	d_drop(dentry); /* this is needed, else LTP fails (VFS won't do it) */
out:
	unlock_dir(lower_dir_dentry);
	dput(lower_dentry);
	trfs_put_lower_path(dentry, &lower_path);
	return err;
}

static int trfs_symlink(struct inode *dir, struct dentry *dentry,
			  const char *symname)
{
	int err;
	struct dentry *lower_dentry;
	struct dentry *lower_parent_dentry = NULL;
	struct path lower_path;

	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	lower_parent_dentry = lock_parent(lower_dentry);

	err = vfs_symlink(d_inode(lower_parent_dentry), lower_dentry, symname);
	if (err)
		goto out;
	err = trfs_interpose(dentry, dir->i_sb, &lower_path);
	if (err)
		goto out;
	fsstack_copy_attr_times(dir, trfs_lower_inode(dir));
	fsstack_copy_inode_size(dir, d_inode(lower_parent_dentry));

out:
	unlock_dir(lower_parent_dentry);
	trfs_put_lower_path(dentry, &lower_path);
	return err;
}

static int trfs_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode)
{
	
	int err;
	struct dentry *lower_dentry;
	struct dentry *lower_parent_dentry = NULL;
	int offset_d;
	struct path lower_path;
	//struct inode *inode;
	struct super_block *sb;
	struct trfs_sb_info *trfs_sb_info;
	unsigned long long offset;
	struct file *filename=NULL;
	char *data=NULL;
	char *temp=NULL;
	
	mm_segment_t oldfs;
	int retVal;
	struct trfs_record *sample_record = NULL;

	printk("trfs_mkdir called\n");

	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	lower_parent_dentry = lock_parent(lower_dentry);


	sb=dir->i_sb;
	//trfs_sb_info=(struct trfs_sb_info*)kzalloc(sizeof(struct trfs_sb_info),GFP_KERNEL);
	trfs_sb_info=(struct trfs_sb_info*)sb->s_fs_info;
	printk("Sb passed\n");
	offset=10;
	filename=trfs_sb_info->tracefile->filename;
	offset=trfs_sb_info->tracefile->offset;
	int bitmap=trfs_sb_info->tracefile->bitmap;
	printk("Bitmap value is:%d",trfs_sb_info->tracefile->bitmap);	
	printk("Offset is:%llu\n",offset);
	if(bitmap & MK_DIR){ 
					if(filename != NULL){

   						oldfs = get_fs();
   						set_fs(get_ds());
						
						sample_record= kzalloc(sizeof(struct trfs_record), GFP_KERNEL);
						if(sample_record == NULL){
							err = -ENOMEM;
							goto out;
						}

						temp = kmalloc(PAGE_SIZE, GFP_KERNEL);
	                    //char *path = d_path(&file->f_path, temp, PAGE_SIZE);
						char *path = dentry_path_raw(dentry, temp, PAGE_SIZE);

	                    printk("Full Path is %s\n", path);

						sample_record->pathname_size = strlen(path);
						printk("pathname size is %d and path name is %s\n", sample_record->pathname_size, path);
						
						sample_record->pathname = kmalloc(sizeof(char)*sample_record->pathname_size, GFP_KERNEL);
						if(sample_record->pathname == NULL){
							err = -ENOMEM;
							goto out;
						}
						memcpy((void *)sample_record->pathname, (void *)dentry->d_name.name, sample_record->pathname_size);
						//printk("Path name in record after memcpy is %s\n", sample_record->pathname);
						sample_record->record_size = sizeof(sample_record->record_id) + sizeof(sample_record->record_size) + sizeof(sample_record->record_type)
                                               + sizeof(sample_record->open_flags) + sizeof(sample_record->permission_mode)
                                               + sizeof(sample_record->pathname_size) + sample_record->pathname_size 
                                               + sizeof(sample_record->size)
                                               + sizeof(sample_record->return_value) + sizeof(sample_record->file_address);
						
						sample_record->record_type = MK_DIR; //char 0 will represent 
						sample_record->open_flags = 10;
						sample_record->permission_mode = 11;
						sample_record->return_value = 1;
	//					sample_record->mybitmap = 1 << MKDIR;
					
	 					mutex_lock(&trfs_sb_info->tracefile->record_lock);
						sample_record->record_id = trfs_sb_info->tracefile->record_id++;
						
						printk("Sample Record -\n");
						printk("record_id is %d\n", sample_record->record_id);
						printk("record_size is %d\n", sample_record->record_size);
						printk("record_type is %c\n", sample_record->record_type);
						printk("return_value is %d\n", sample_record->return_value);
	//					printk("my bitmap value is %d\n", sample_record->mybitmap);

						data= kzalloc(sample_record->record_size, GFP_KERNEL);
					
						offset_d = 0;

						memcpy((void *)(data + offset_d), (void *)&sample_record->record_size, sizeof(short));
						offset_d = offset_d + sizeof(short);

						memcpy((void *)(data + offset_d), (void *)&sample_record->record_id, sizeof(int));
						offset_d = offset_d + sizeof(int);

						memcpy((void *)(data + offset_d), (void *)&sample_record->record_type, sizeof(char));
						offset_d = offset_d + sizeof(char);

						memcpy((void *)(data + offset_d), (void *)&sample_record->open_flags, sizeof(int));
						offset_d = offset_d + sizeof(int);

						memcpy((void *)(data + offset_d), (void *)&sample_record->permission_mode, sizeof(int));
						offset_d = offset_d + sizeof(int);

						memcpy((void *)(data + offset_d), (void *)&sample_record->pathname_size, sizeof(short));
						offset_d = offset_d + sizeof(short);

						memcpy((void *)(data + offset_d), (void *)sample_record->pathname, sample_record->pathname_size);
						offset_d = offset_d + sample_record->pathname_size;

						memcpy((void *)(data + offset_d), (void *)&sample_record->size, sizeof(unsigned long long));
                        offset_d = offset_d + sizeof(unsigned long long);

                        memcpy((void *)(data + offset_d), (void *)&sample_record->return_value, sizeof(int));
                        offset_d = offset_d + sizeof(int);

                        memcpy((void *)(data + offset_d), (void *)&sample_record->file_address, sizeof(unsigned long long));
                        offset_d = offset_d + sizeof(unsigned long long);


						printk("data is %s\n", data);

						retVal = vfs_write(filename, data, sample_record->record_size,&offset);
						printk("number of bytes written %d\n", retVal);
    					set_fs(oldfs);
    					mutex_unlock(&trfs_sb_info->tracefile->record_lock);
					}
	}
	err = vfs_mkdir(d_inode(lower_parent_dentry), lower_dentry, mode);
	if (err)
		goto out;

	err = trfs_interpose(dentry, dir->i_sb, &lower_path);
	if (err)
		goto out;

	fsstack_copy_attr_times(dir, trfs_lower_inode(dir));
	fsstack_copy_inode_size(dir, d_inode(lower_parent_dentry));
	/* update number of links on parent directory */
	set_nlink(dir, trfs_lower_inode(dir)->i_nlink);

out:
	unlock_dir(lower_parent_dentry);
	trfs_put_lower_path(dentry, &lower_path);
	//if(data)
	//	kfree(data);
	if(sample_record){
		if(sample_record->pathname){
			kfree(sample_record->pathname);
		}
		kfree(sample_record);
	}
	if(data)
		kfree(data);
	if(temp)
		kfree(temp);
	return err;
}

static int trfs_rmdir(struct inode *dir, struct dentry *dentry)
{
	//printk("Trfs_Rmdir called");

	struct dentry *lower_dentry;
	struct dentry *lower_dir_dentry;
	int err;
	struct path lower_path;

	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	lower_dir_dentry = lock_parent(lower_dentry);

	err = vfs_rmdir(d_inode(lower_dir_dentry), lower_dentry);
	if (err)
		goto out;

	d_drop(dentry);	/* drop our dentry on success (why not VFS's job?) */
	if (d_inode(dentry))
		clear_nlink(d_inode(dentry));
	fsstack_copy_attr_times(dir, d_inode(lower_dir_dentry));
	fsstack_copy_inode_size(dir, d_inode(lower_dir_dentry));
	set_nlink(dir, d_inode(lower_dir_dentry)->i_nlink);

out:
	unlock_dir(lower_dir_dentry);
	trfs_put_lower_path(dentry, &lower_path);
	return err;
}

static int trfs_mknod(struct inode *dir, struct dentry *dentry, umode_t mode,
			dev_t dev)
{
	//printk("Trfs_Mknod called");
	
	int err;
	struct dentry *lower_dentry;
	struct dentry *lower_parent_dentry = NULL;
	struct path lower_path;

	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	lower_parent_dentry = lock_parent(lower_dentry);

	err = vfs_mknod(d_inode(lower_parent_dentry), lower_dentry, mode, dev);
	if (err)
		goto out;

	err = trfs_interpose(dentry, dir->i_sb, &lower_path);
	if (err)
		goto out;
	fsstack_copy_attr_times(dir, trfs_lower_inode(dir));
	fsstack_copy_inode_size(dir, d_inode(lower_parent_dentry));

out:
	unlock_dir(lower_parent_dentry);
	trfs_put_lower_path(dentry, &lower_path);
	return err;
}

/*
 * The locking rules in trfs_rename are complex.  We could use a simpler
 * superblock-level name-space lock for renames and copy-ups.
 */
static int trfs_rename(struct inode *old_dir, struct dentry *old_dentry,
			 struct inode *new_dir, struct dentry *new_dentry)
{
	//printk("Trfs_rename called");

	int err = 0;
	struct dentry *lower_old_dentry = NULL;
	struct dentry *lower_new_dentry = NULL;
	struct dentry *lower_old_dir_dentry = NULL;
	struct dentry *lower_new_dir_dentry = NULL;
	struct dentry *trap = NULL;
	struct path lower_old_path, lower_new_path;

	trfs_get_lower_path(old_dentry, &lower_old_path);
	trfs_get_lower_path(new_dentry, &lower_new_path);
	lower_old_dentry = lower_old_path.dentry;
	lower_new_dentry = lower_new_path.dentry;
	lower_old_dir_dentry = dget_parent(lower_old_dentry);
	lower_new_dir_dentry = dget_parent(lower_new_dentry);

	trap = lock_rename(lower_old_dir_dentry, lower_new_dir_dentry);
	/* source should not be ancestor of target */
	if (trap == lower_old_dentry) {
		err = -EINVAL;
		goto out;
	}
	/* target should not be ancestor of source */
	if (trap == lower_new_dentry) {
		err = -ENOTEMPTY;
		goto out;
	}

	err = vfs_rename(d_inode(lower_old_dir_dentry), lower_old_dentry,
			 d_inode(lower_new_dir_dentry), lower_new_dentry,
			 NULL, 0);
	if (err)
		goto out;

	fsstack_copy_attr_all(new_dir, d_inode(lower_new_dir_dentry));
	fsstack_copy_inode_size(new_dir, d_inode(lower_new_dir_dentry));
	if (new_dir != old_dir) {
		fsstack_copy_attr_all(old_dir,
				      d_inode(lower_old_dir_dentry));
		fsstack_copy_inode_size(old_dir,
					d_inode(lower_old_dir_dentry));
	}

out:
	unlock_rename(lower_old_dir_dentry, lower_new_dir_dentry);
	dput(lower_old_dir_dentry);
	dput(lower_new_dir_dentry);
	trfs_put_lower_path(old_dentry, &lower_old_path);
	trfs_put_lower_path(new_dentry, &lower_new_path);
	return err;
}

static int trfs_readlink(struct dentry *dentry, char __user *buf, int bufsiz)
{
	//printk("Trfs_Readlink called");

	int err;
	struct dentry *lower_dentry;
	struct path lower_path;

	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	if (!d_inode(lower_dentry)->i_op ||
	    !d_inode(lower_dentry)->i_op->readlink) {
		err = -EINVAL;
		goto out;
	}

	err = d_inode(lower_dentry)->i_op->readlink(lower_dentry,
						    buf, bufsiz);
	if (err < 0)
		goto out;
	fsstack_copy_attr_atime(d_inode(dentry), d_inode(lower_dentry));

out:
	trfs_put_lower_path(dentry, &lower_path);
	return err;
}

static const char *trfs_get_link(struct dentry *dentry, struct inode *inode,
				   struct delayed_call *done)
{
	//printk("Trfs_Getlink called");

	char *buf;
	int len = PAGE_SIZE, err;
	mm_segment_t old_fs;

	if (!dentry)
		return ERR_PTR(-ECHILD);

	/* This is freed by the put_link method assuming a successful call. */
	buf = kmalloc(len, GFP_KERNEL);
	if (!buf) {
		buf = ERR_PTR(-ENOMEM);
		return buf;
	}

	/* read the symlink, and then we will follow it */
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	err = trfs_readlink(dentry, buf, len);
	set_fs(old_fs);
	if (err < 0) {
		kfree(buf);
		buf = ERR_PTR(err);
	} else {
		buf[err] = '\0';
	}
	set_delayed_call(done, kfree_link, buf);
	return buf;
}

static int trfs_permission(struct inode *inode, int mask)
{
	//printk("Trfs_Permission called");

	struct inode *lower_inode;
	int err;

	lower_inode = trfs_lower_inode(inode);
	err = inode_permission(lower_inode, mask);
	return err;
}

static int trfs_setattr(struct dentry *dentry, struct iattr *ia)
{
	//printk("Trfs_Setattr called");

	int err;
	struct dentry *lower_dentry;
	struct inode *inode;
	struct inode *lower_inode;
	struct path lower_path;
	struct iattr lower_ia;

	inode = d_inode(dentry);

	/*
	 * Check if user has permission to change inode.  We don't check if
	 * this user can change the lower inode: that should happen when
	 * calling notify_change on the lower inode.
	 */
	err = inode_change_ok(inode, ia);
	if (err)
		goto out_err;

	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	lower_inode = trfs_lower_inode(inode);

	/* prepare our own lower struct iattr (with the lower file) */
	memcpy(&lower_ia, ia, sizeof(lower_ia));
	if (ia->ia_valid & ATTR_FILE)
		lower_ia.ia_file = trfs_lower_file(ia->ia_file);

	/*
	 * If shrinking, first truncate upper level to cancel writing dirty
	 * pages beyond the new eof; and also if its' maxbytes is more
	 * limiting (fail with -EFBIG before making any change to the lower
	 * level).  There is no need to vmtruncate the upper level
	 * afterwards in the other cases: we fsstack_copy_inode_size from
	 * the lower level.
	 */
	if (ia->ia_valid & ATTR_SIZE) {
		err = inode_newsize_ok(inode, ia->ia_size);
		if (err)
			goto out;
		truncate_setsize(inode, ia->ia_size);
	}

	/*
	 * mode change is for clearing setuid/setgid bits. Allow lower fs
	 * to interpret this in its own way.
	 */
	if (lower_ia.ia_valid & (ATTR_KILL_SUID | ATTR_KILL_SGID))
		lower_ia.ia_valid &= ~ATTR_MODE;

	/* notify the (possibly copied-up) lower inode */
	/*
	 * Note: we use d_inode(lower_dentry), because lower_inode may be
	 * unlinked (no inode->i_sb and i_ino==0.  This happens if someone
	 * tries to open(), unlink(), then ftruncate() a file.
	 */
	inode_lock(d_inode(lower_dentry));
	err = notify_change(lower_dentry, &lower_ia, /* note: lower_ia */
			    NULL);
	inode_unlock(d_inode(lower_dentry));
	if (err)
		goto out;

	/* get attributes from the lower inode */
	fsstack_copy_attr_all(inode, lower_inode);
	/*
	 * Not running fsstack_copy_inode_size(inode, lower_inode), because
	 * VFS should update our inode size, and notify_change on
	 * lower_inode should update its size.
	 */

out:
	trfs_put_lower_path(dentry, &lower_path);
out_err:
	return err;
}

static int trfs_getattr(struct vfsmount *mnt, struct dentry *dentry,
			  struct kstat *stat)
{
	//printk("Trfs_Getattr called");

	int err;
	struct kstat lower_stat;
	struct path lower_path;

	trfs_get_lower_path(dentry, &lower_path);
	err = vfs_getattr(&lower_path, &lower_stat);
	if (err)
		goto out;
	fsstack_copy_attr_all(d_inode(dentry),
			      d_inode(lower_path.dentry));
	generic_fillattr(d_inode(dentry), stat);
	stat->blocks = lower_stat.blocks;
out:
	trfs_put_lower_path(dentry, &lower_path);
	return err;
}

static int
trfs_setxattr(struct dentry *dentry, const char *name, const void *value,
		size_t size, int flags)
{
	//printk("Trfs_Setxattr called");

	int err; struct dentry *lower_dentry;
	struct path lower_path;

	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	if (!d_inode(lower_dentry)->i_op->setxattr) {
		err = -EOPNOTSUPP;
		goto out;
	}
	err = vfs_setxattr(lower_dentry, name, value, size, flags);
	if (err)
		goto out;
	fsstack_copy_attr_all(d_inode(dentry),
			      d_inode(lower_path.dentry));
out:
	trfs_put_lower_path(dentry, &lower_path);
	return err;
}

static ssize_t
trfs_getxattr(struct dentry *dentry, const char *name, void *buffer,
		size_t size)
{
	//printk("Trfs_getxattr called");

	int err;
	struct dentry *lower_dentry;
	struct path lower_path;

	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	if (!d_inode(lower_dentry)->i_op->getxattr) {
		err = -EOPNOTSUPP;
		goto out;
	}
	err = vfs_getxattr(lower_dentry, name, buffer, size);
	if (err)
		goto out;
	fsstack_copy_attr_atime(d_inode(dentry),
				d_inode(lower_path.dentry));
out:
	trfs_put_lower_path(dentry, &lower_path);
	return err;
}

static ssize_t trfs_listxattr(struct dentry *dentry, char *buffer, size_t buffer_size)
{
	//printk("Trfs_listxattr called");
	
	int err;
	struct dentry *lower_dentry;
	struct path lower_path;

	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	if (!d_inode(lower_dentry)->i_op->listxattr) {
		err = -EOPNOTSUPP;
		goto out;
	}
	err = vfs_listxattr(lower_dentry, buffer, buffer_size);
	if (err)
		goto out;
	fsstack_copy_attr_atime(d_inode(dentry),
				d_inode(lower_path.dentry));
out:
	trfs_put_lower_path(dentry, &lower_path);
	return err;
}

static int
trfs_removexattr(struct dentry *dentry, const char *name)
{
	//printk("Trfs_Removexattr called");

	int err;
	struct dentry *lower_dentry;
	struct path lower_path;

	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	if (!d_inode(lower_dentry)->i_op ||
	    !d_inode(lower_dentry)->i_op->removexattr) {
		err = -EINVAL;
		goto out;
	}
	err = vfs_removexattr(lower_dentry, name);
	if (err)
		goto out;
	fsstack_copy_attr_all(d_inode(dentry),
			      d_inode(lower_path.dentry));
out:
	trfs_put_lower_path(dentry, &lower_path);
	return err;
}
const struct inode_operations trfs_symlink_iops = {
	.readlink	= trfs_readlink,
	.permission	= trfs_permission,
	.setattr	= trfs_setattr,
	.getattr	= trfs_getattr,
	.get_link	= trfs_get_link,
	.setxattr	= trfs_setxattr,
	.getxattr	= trfs_getxattr,
	.listxattr	= trfs_listxattr,
	.removexattr	= trfs_removexattr,
};

const struct inode_operations trfs_dir_iops = {
	.create		= trfs_create,
	.lookup		= trfs_lookup,
	.link		= trfs_link,
	.unlink		= trfs_unlink,
	.symlink	= trfs_symlink,
	.mkdir		= trfs_mkdir,
	.rmdir		= trfs_rmdir,
	.mknod		= trfs_mknod,
	.rename		= trfs_rename,
	.permission	= trfs_permission,
	.setattr	= trfs_setattr,
	.getattr	= trfs_getattr,
	.setxattr	= trfs_setxattr,
	.getxattr	= trfs_getxattr,
	.listxattr	= trfs_listxattr,
	.removexattr	= trfs_removexattr,
};

const struct inode_operations trfs_main_iops = {
	.permission	= trfs_permission,
	.setattr	= trfs_setattr,
	.getattr	= trfs_getattr,
	.setxattr	= trfs_setxattr,
	.getxattr	= trfs_getxattr,
	.listxattr	= trfs_listxattr,
	.removexattr	= trfs_removexattr,
};
