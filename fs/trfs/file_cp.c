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
#include "record.h"
#include "trfs.h"

static ssize_t trfs_read(struct file *file, char __user *buf,
			   size_t count, loff_t *ppos)
{
	int err;
	struct file *lower_file;
	struct dentry *dentry = file->f_path.dentry;
	struct super_block *sb;
        struct trfs_sb_info *trfs_sb_info;
        //unsigned long long offset;
        struct file *filename=NULL;
        char *data=NULL;
        char *temp =NULL;
	int offset_d=0;
        mm_segment_t oldfs;
        int retVal;
        struct trfs_record *sample_record = NULL;

	printk("Trfs_read called\n");
	printk("Pointer address is %d\n", file);
	sb=file->f_inode->i_sb;
	
	trfs_sb_info=(struct trfs_sb_info*)sb->s_fs_info;
        printk("Sb passed\n");
        filename=trfs_sb_info->tracefile->filename;
        //offset=trfs_sb_info->tracefile->offset;
        int bitmap=trfs_sb_info->tracefile->bitmap;
        printk("Bitmap value is:%d\n",trfs_sb_info->tracefile->bitmap);
        //printk("Offset is:%llu\n",offset);


	lower_file = trfs_lower_file(file);
	err = vfs_read(lower_file, buf, count, ppos);
	/* update our inode atime upon a successful lower read */
	if (err >= 0)
		{
		fsstack_copy_attr_atime(d_inode(dentry),
					file_inode(lower_file));
		if(bitmap & READ_TR){
			
				if(filename != NULL){

                    

                    sample_record= kzalloc(sizeof(struct trfs_record), GFP_KERNEL);
                    if(sample_record == NULL){
                            err = -ENOMEM;
                            goto out;
                    }

                    temp = kmalloc(PAGE_SIZE, GFP_KERNEL);
                    char *path = d_path(&file->f_path, temp, PAGE_SIZE);

                    printk("Full Path is %s\n", path);

                    sample_record->pathname_size = strlen(path);
                    printk("pathname size is %d and path name is %s\n", sample_record->pathname_size, path);

                    sample_record->pathname = kmalloc(sizeof(char)*sample_record->pathname_size, GFP_KERNEL);
                    if(sample_record->pathname == NULL){
                            err = -ENOMEM;
                            goto out;
                    }
                    memcpy((void *)sample_record->pathname, (void *)path, sample_record->pathname_size);


				  sample_record->record_size = sizeof(sample_record->record_id) + sizeof(sample_record->record_size) + sizeof(sample_record->record_type)
                                               + sizeof(sample_record->open_flags) + sizeof(sample_record->permission_mode)
                                               + sizeof(sample_record->pathname_size) + sample_record->pathname_size 
                                               + sizeof(sample_record->size)
                                               + sizeof(sample_record->return_value) + sizeof(sample_record->file_address);

						sample_record->record_type=READ_TR;
						sample_record->open_flags = 0;
                        sample_record->permission_mode = 0;
                        sample_record->return_value = err;
						//sample_record->offset=kmalloc(sizeof(unsigned long),GFP_KERNEL);
						//sample_record->offset=ppos;
						sample_record->size=count;
						sample_record->file_address = file;

						mutex_lock(&trfs_sb_info->tracefile->record_lock);
						sample_record->record_id = trfs_sb_info->tracefile->record_id++;

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

                            // memcpy((void *)(data + offset_d), (void *)sample_record->offset, sizeof(int));
                            // offset_d = offset_d + sizeof(int);

                            memcpy((void *)(data + offset_d), (void *)&sample_record->size, sizeof(unsigned long long));
                            offset_d = offset_d + sizeof(unsigned long long);

                            memcpy((void *)(data + offset_d), (void *)&sample_record->return_value, sizeof(int));
                            offset_d = offset_d + sizeof(int);

                            memcpy((void *)(data + offset_d), (void *)&sample_record->file_address, sizeof(unsigned long long));
                            offset_d = offset_d + sizeof(unsigned long long);


                            //printk("data is %s\n", data);

                            oldfs = get_fs();
                    		set_fs(get_ds());

                            retVal = vfs_write(filename, data, sample_record->record_size,&trfs_sb_info->tracefile->offset);
                            printk("number of bytes written %d\n", retVal);

                            set_fs(oldfs);
                            mutex_unlock(&trfs_sb_info->tracefile->record_lock);


				}
		}
	}
out:
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

static ssize_t trfs_write(struct file *file, const char __user *buf,
			    size_t count, loff_t *ppos)
{
	int err;

	struct file *lower_file;
	struct dentry *dentry = file->f_path.dentry;

	lower_file = trfs_lower_file(file);
	err = vfs_write(lower_file, buf, count, ppos);
	/* update our inode times+sizes upon a successful lower write */
	if (err >= 0) {
		fsstack_copy_inode_size(d_inode(dentry),
					file_inode(lower_file));
		fsstack_copy_attr_times(d_inode(dentry),
					file_inode(lower_file));
	}

	return err;
}

static int trfs_readdir(struct file *file, struct dir_context *ctx)
{
	int err;
	struct file *lower_file = NULL;
	struct dentry *dentry = file->f_path.dentry;

	lower_file = trfs_lower_file(file);
	err = iterate_dir(lower_file, ctx);
	file->f_pos = lower_file->f_pos;
	if (err >= 0)		/* copy the atime */
		fsstack_copy_attr_atime(d_inode(dentry),
					file_inode(lower_file));
	return err;
}

static long trfs_unlocked_ioctl(struct file *file, unsigned int cmd,
				  unsigned long arg)
{
	long err = -ENOTTY;
	struct file *lower_file;
	struct super_block *sb;
	unsigned long *val;
	int gs_value;
	struct trfs_sb_info *sb_info;
	val=kmalloc(sizeof(unsigned long),GFP_KERNEL);
	sb=file->f_inode->i_sb;

	//printk("Super block accessed\n");

	//val=arg;
	printk("trfs_ioctl called\n");
	printk("Cmd is :%d\n",cmd);
	//printk("address is:%d\n",arg);

	copy_from_user(val, (void *)arg, sizeof(val));
	gs_value=*val;
	sb_info=(struct trfs_sb_info*)sb->s_fs_info;
	//printk("SB_info accessed\n");

	printk("value is:%d\n",gs_value);
	if(cmd==1)
	{
	 sb_info->tracefile->bitmap=gs_value;		
	}
	else
	{	
		arg=sb_info->tracefile->bitmap;
	}
//	copy_from_user(val,arg,sizeof(unsigned long));

	lower_file = trfs_lower_file(file);

/* XXX: use vfs_ioctl if/when VFS exports it */
	if (!lower_file || !lower_file->f_op)
		goto out;
	if (lower_file->f_op->unlocked_ioctl)
		err = lower_file->f_op->unlocked_ioctl(lower_file, cmd, arg);

	/* some ioctls can change inode attributes (EXT2_IOC_SETFLAGS) */
	if (!err)
		fsstack_copy_attr_all(file_inode(file),
				      file_inode(lower_file));
out:
	kfree(val);
	return err;
}

#ifdef CONFIG_COMPAT
static long trfs_compat_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg)
{
	long err = -ENOTTY;
	struct file *lower_file;

	lower_file = trfs_lower_file(file);

	/* XXX: use vfs_ioctl if/when VFS exports it */
	if (!lower_file || !lower_file->f_op)
		goto out;
	if (lower_file->f_op->compat_ioctl)
		err = lower_file->f_op->compat_ioctl(lower_file, cmd, arg);

out:
	return err;
}
#endif

static int trfs_mmap(struct file *file, struct vm_area_struct *vma)
{
	int err = 0;
	bool willwrite;
	struct file *lower_file;
	const struct vm_operations_struct *saved_vm_ops = NULL;

	/* this might be deferred to mmap's writepage */
	willwrite = ((vma->vm_flags | VM_SHARED | VM_WRITE) == vma->vm_flags);

	/*
	 * File systems which do not implement ->writepage may use
	 * generic_file_readonly_mmap as their ->mmap op.  If you call
	 * generic_file_readonly_mmap with VM_WRITE, you'd get an -EINVAL.
	 * But we cannot call the lower ->mmap op, so we can't tell that
	 * writeable mappings won't work.  Therefore, our only choice is to
	 * check if the lower file system supports the ->writepage, and if
	 * not, return EINVAL (the same error that
	 * generic_file_readonly_mmap returns in that case).
	 */
	lower_file = trfs_lower_file(file);
	if (willwrite && !lower_file->f_mapping->a_ops->writepage) {
		err = -EINVAL;
		printk(KERN_ERR "trfs: lower file system does not "
		       "support writeable mmap\n");
		goto out;
	}

	/*
	 * find and save lower vm_ops.
	 *
	 * XXX: the VFS should have a cleaner way of finding the lower vm_ops
	 */
	if (!TRFS_F(file)->lower_vm_ops) {
		err = lower_file->f_op->mmap(lower_file, vma);
		if (err) {
			printk(KERN_ERR "trfs: lower mmap failed %d\n", err);
			goto out;
		}
		saved_vm_ops = vma->vm_ops; /* save: came from lower ->mmap */
	}

	/*
	 * Next 3 lines are all I need from generic_file_mmap.  I definitely
	 * don't want its test for ->readpage which returns -ENOEXEC.
	 */
	file_accessed(file);
	vma->vm_ops = &trfs_vm_ops;

	file->f_mapping->a_ops = &trfs_aops; /* set our aops */
	if (!TRFS_F(file)->lower_vm_ops) /* save for our ->fault */
		TRFS_F(file)->lower_vm_ops = saved_vm_ops;

out:
	return err;
}

static int trfs_open(struct inode *inode, struct file *file)
{
	int err = 0;
	struct file *lower_file = NULL;
	struct path lower_path;

	printk("In Trfs_open\n");
	//printk("file path is %s\n", file->f_path.dentry->d_path);
	printk("Pointer address is %d\n", file);

	/* don't open unhashed/deleted files */
	if (d_unhashed(file->f_path.dentry)) {
		err = -ENOENT;
		goto out_err;
	}

	file->private_data =
		kzalloc(sizeof(struct trfs_file_info), GFP_KERNEL);
	if (!TRFS_F(file)) {
		err = -ENOMEM;
		goto out_err;
	}

	/* open lower object and link trfs's file struct to lower's */
	trfs_get_lower_path(file->f_path.dentry, &lower_path);
	lower_file = dentry_open(&lower_path, file->f_flags, current_cred());
	path_put(&lower_path);
	if (IS_ERR(lower_file)) {
		err = PTR_ERR(lower_file);
		lower_file = trfs_lower_file(file);
		if (lower_file) {
			trfs_set_lower_file(file, NULL);
			fput(lower_file); /* fput calls dput for lower_dentry */
		}
	} else {
		trfs_set_lower_file(file, lower_file);
	}

	if (err)
		kfree(TRFS_F(file));
	else
		fsstack_copy_attr_all(inode, trfs_lower_inode(inode));
out_err:
	return err;
}

static int trfs_flush(struct file *file, fl_owner_t id)
{
	int err = 0;
	struct file *lower_file = NULL;

	lower_file = trfs_lower_file(file);
	if (lower_file && lower_file->f_op && lower_file->f_op->flush) {
		filemap_write_and_wait(file->f_mapping);
		err = lower_file->f_op->flush(lower_file, id);
	}

	return err;
}

/* release all lower object references & free the file info structure */
static int trfs_file_release(struct inode *inode, struct file *file)
{
	struct file *lower_file;

	lower_file = trfs_lower_file(file);
	if (lower_file) {
		trfs_set_lower_file(file, NULL);
		fput(lower_file);
	}

	kfree(TRFS_F(file));
	return 0;
}

static int trfs_fsync(struct file *file, loff_t start, loff_t end,
			int datasync)
{
	int err;
	struct file *lower_file;
	struct path lower_path;
	struct dentry *dentry = file->f_path.dentry;

	err = __generic_file_fsync(file, start, end, datasync);
	if (err)
		goto out;
	lower_file = trfs_lower_file(file);
	trfs_get_lower_path(dentry, &lower_path);
	err = vfs_fsync_range(lower_file, start, end, datasync);
	trfs_put_lower_path(dentry, &lower_path);
out:
	return err;
}

static int trfs_fasync(int fd, struct file *file, int flag)
{
	int err = 0;
	struct file *lower_file = NULL;

	lower_file = trfs_lower_file(file);
	if (lower_file->f_op && lower_file->f_op->fasync)
		err = lower_file->f_op->fasync(fd, lower_file, flag);

	return err;
}

/*
 * Trfs cannot use generic_file_llseek as ->llseek, because it would
 * only set the offset of the upper file.  So we have to implement our
 * own method to set both the upper and lower file offsets
 * consistently.
 */
static loff_t trfs_file_llseek(struct file *file, loff_t offset, int whence)
{
	int err;
	struct file *lower_file;

	err = generic_file_llseek(file, offset, whence);
	if (err < 0)
		goto out;

	lower_file = trfs_lower_file(file);
	err = generic_file_llseek(lower_file, offset, whence);

out:
	return err;
}

/*
 * Trfs read_iter, redirect modified iocb to lower read_iter
 */
ssize_t
trfs_read_iter(struct kiocb *iocb, struct iov_iter *iter)
{
	int err;
	struct file *file = iocb->ki_filp, *lower_file;

	lower_file = trfs_lower_file(file);
	if (!lower_file->f_op->read_iter) {
		err = -EINVAL;
		goto out;
	}

	get_file(lower_file); /* prevent lower_file from being released */
	iocb->ki_filp = lower_file;
	err = lower_file->f_op->read_iter(iocb, iter);
	iocb->ki_filp = file;
	fput(lower_file);
	/* update upper inode atime as needed */
	if (err >= 0 || err == -EIOCBQUEUED)
		fsstack_copy_attr_atime(d_inode(file->f_path.dentry),
					file_inode(lower_file));
out:
	return err;
}

/*
 * Trfs write_iter, redirect modified iocb to lower write_iter
 */
ssize_t
trfs_write_iter(struct kiocb *iocb, struct iov_iter *iter)
{
	int err;
	struct file *file = iocb->ki_filp, *lower_file;

	lower_file = trfs_lower_file(file);
	if (!lower_file->f_op->write_iter) {
		err = -EINVAL;
		goto out;
	}

	get_file(lower_file); /* prevent lower_file from being released */
	iocb->ki_filp = lower_file;
	err = lower_file->f_op->write_iter(iocb, iter);
	iocb->ki_filp = file;
	fput(lower_file);
	/* update upper inode times/sizes as needed */
	if (err >= 0 || err == -EIOCBQUEUED) {
		fsstack_copy_inode_size(d_inode(file->f_path.dentry),
					file_inode(lower_file));
		fsstack_copy_attr_times(d_inode(file->f_path.dentry),
					file_inode(lower_file));
	}
out:
	return err;
}

const struct file_operations trfs_main_fops = {
	.llseek		= generic_file_llseek,
	.read		= trfs_read,
	.write		= trfs_write,
	.unlocked_ioctl	= trfs_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= trfs_compat_ioctl,
#endif
	.mmap		= trfs_mmap,
	.open		= trfs_open,
	.flush		= trfs_flush,
	.release	= trfs_file_release,
	.fsync		= trfs_fsync,
	.fasync		= trfs_fasync,
	.read_iter	= trfs_read_iter,
	.write_iter	= trfs_write_iter,
};

/* trimmed directory options */
const struct file_operations trfs_dir_fops = {
	.llseek		= trfs_file_llseek,
	.read		= generic_read_dir,
	.iterate	= trfs_readdir,
	.unlocked_ioctl	= trfs_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= trfs_compat_ioctl,
#endif
	.open		= trfs_open,
	.release	= trfs_file_release,
	.flush		= trfs_flush,
	.fsync		= trfs_fsync,
	.fasync		= trfs_fasync,
};
