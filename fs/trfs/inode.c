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
	/*Adding data structures for trfs*/
	struct super_block *sb;
    struct trfs_sb_info *trfs_sb_info;
    struct file *filename;
  
    char *temp;
    int bitmap;
    mm_segment_t oldfs;
    int retVal;
    struct trfs_link_record *sample_record;
	char *old_path;
	char *new_path;

	sample_record=NULL;
	temp=NULL;
	filename=NULL;

   printk("trfs_link called\n");



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
	/*traceable code starts now*/
		sb=dir->i_sb;
        trfs_sb_info=(struct trfs_sb_info*)sb->s_fs_info;
        filename=trfs_sb_info->tracefile->filename;
        bitmap=trfs_sb_info->tracefile->bitmap;
     
					
                if(bitmap & LINK_TR)
                {

                        if(filename != NULL){

                               
                                sample_record= kzalloc(sizeof(struct trfs_link_record), GFP_KERNEL);
                                if(sample_record == NULL){
                                        err = -ENOMEM;
                                        goto out_err;
                                        }

                                temp = kmalloc(BUFFER_SIZE, GFP_KERNEL);
								old_path = dentry_path_raw(old_dentry, temp, BUFFER_SIZE);

								printk("Old Path is %s\n", old_path);

                                sample_record->oldpathsize = strlen(old_path);
                                printk("pathname size is %d and path name is %s\n", sample_record->oldpathsize, old_path);

                                sample_record->oldpath = kmalloc(sizeof(char)*sample_record->oldpathsize, GFP_KERNEL);
                                if(sample_record->oldpath == NULL){
                                        err = -ENOMEM;
                                        goto out_err;
                                                }
                                strcpy(sample_record->oldpath,old_path);
								new_path = dentry_path_raw(new_dentry, temp, PAGE_SIZE);

								printk("new Path is %s\n", new_path);

                                sample_record->newpathsize = strlen(new_path);
                                printk("pathname size is %d and path name is %s\n", sample_record->newpathsize, new_path);
                                
                                
                                sample_record->record_size = sizeof(sample_record->record_id) + sizeof(sample_record->record_size) + sizeof(sample_record->record_type)
				    										+ sizeof(sample_record->oldpathsize) + sample_record->oldpathsize
                                               				+ sizeof(sample_record->newpathsize)  + sample_record->newpathsize
                                               				+ sizeof(sample_record->return_value) ;

                                sample_record->record_type = LINK_TR; //char 0 will represent
                                sample_record->return_value = err;

                                if(sample_record->newpathsize > BUFFER_SIZE/2){
					                //Write count is greater than page size...so flush the buffer
					                printk("count > BUFFER_SIZE/2...BUFFER overflow in write\n");
					                mutex_lock(&trfs_sb_info->tracefile->record_lock);

					                oldfs = get_fs();
					                set_fs(get_ds());

					                //Flush the buffer to file and reset the offset to 0
					                retVal = vfs_write(filename, trfs_sb_info->tracefile->buffer, trfs_sb_info->tracefile->buffer_offset, &trfs_sb_info->tracefile->offset);
					                printk("number of bytes written %d\n", retVal);
					                trfs_sb_info->tracefile->buffer_offset = 0;

					                //Get the record ID for write record
					                sample_record->record_id = trfs_sb_info->tracefile->record_id++;

					                //Push rest of write record details
					                memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->record_size, sizeof(short));
	                                trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(short);

	                                memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->record_type, sizeof(int));
	                                trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(int);


	                                memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->oldpathsize, sizeof(short));
	                                trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(short);

	                                memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)sample_record->oldpath, sample_record->oldpathsize);
	                                trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sample_record->oldpathsize;

	                                memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->return_value, sizeof(int));
	                                trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(int);
								

	                                memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->record_id, sizeof(int));
	                                trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(int);
	              
	                               
					  				memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->newpathsize, sizeof(short));
	                                trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(short);

					                retVal = vfs_write(filename, trfs_sb_info->tracefile->buffer, trfs_sb_info->tracefile->buffer_offset, &trfs_sb_info->tracefile->offset);
					                printk("number of bytes written %d\n", retVal);
					                trfs_sb_info->tracefile->buffer_offset = 0;

					                //Write the entire write-buf to the file 
					                retVal = vfs_write(filename, new_path, sample_record->newpathsize, &trfs_sb_info->tracefile->offset);
					                printk("writing the new path directly to log file\n");

					                set_fs(oldfs);
					                mutex_unlock(&trfs_sb_info->tracefile->record_lock);

            					}
            					else{

            						sample_record->newpath = kmalloc(sizeof(char)*sample_record->newpathsize, GFP_KERNEL);
	                                if(sample_record->newpath == NULL){
	                                    err = -ENOMEM;
	                                    goto out_err;
	                                }
	                                strcpy(sample_record->newpath,new_path);

            						if(trfs_sb_info->tracefile->buffer_offset + sample_record->record_size >= 2*BUFFER_SIZE){
						                mutex_lock(&trfs_sb_info->tracefile->record_lock);

		                				oldfs = get_fs();
						                set_fs(get_ds());

						                //Flush the buffer to file and reset the offset to 0
						                retVal = vfs_write(filename, trfs_sb_info->tracefile->buffer, trfs_sb_info->tracefile->buffer_offset, &trfs_sb_info->tracefile->offset);
						                printk("number of bytes written %d\n", retVal);
						                trfs_sb_info->tracefile->buffer_offset = 0;

						                set_fs(oldfs);
						                mutex_unlock(&trfs_sb_info->tracefile->record_lock);
            						}


	                                mutex_lock(&trfs_sb_info->tracefile->record_lock);
	                                sample_record->record_id = trfs_sb_info->tracefile->record_id++;

	                                printk("Sample Record -\n");
	                                printk("record_id is %d\n", sample_record->record_id);
	                                printk("record_size is %d\n", sample_record->record_size);
	                                printk("record_type is %d\n", sample_record->record_type);
	                                printk("return_value is %d\n", sample_record->return_value);



                              
	                                memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->record_size, sizeof(short));
	                                trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(short);

	                                memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->record_type, sizeof(int));
	                                trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(int);


	                                memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->oldpathsize, sizeof(short));
	                                trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(short);

	                                memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)sample_record->oldpath, sample_record->oldpathsize);
	                                trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sample_record->oldpathsize;

	                                memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->return_value, sizeof(int));
	                                trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(int);
								

	                                memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->record_id, sizeof(int));
	                                trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(int);
	              
	                               
					  				memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->newpathsize, sizeof(short));
	                                trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(short);

					  				memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)sample_record->newpath, sample_record->newpathsize);
	                                trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sample_record->newpathsize;

	                                mutex_unlock(&trfs_sb_info->tracefile->record_lock);

            					}
            }
        }
out_err:
	if(sample_record){
                if(sample_record->oldpath){
                        kfree(sample_record->oldpath);
                }
		 if(sample_record->newpath){
                        kfree(sample_record->newpath);
                }

                kfree(sample_record);
        }
        if(temp)
                kfree(temp);

 
	return err;
}

static int trfs_unlink(struct inode *dir, struct dentry *dentry)
{

	int err;
	struct dentry *lower_dentry;
	struct inode *lower_dir_inode = trfs_lower_inode(dir);
	struct dentry *lower_dir_dentry;
	struct path lower_path;
   	 struct super_block *sb;
	struct trfs_sb_info *trfs_sb_info;
	//unsigned long long offset;
	struct file *filename=NULL;
	char *temp=NULL;
	int bitmap;
	char *path;
	mm_segment_t oldfs;
	int retVal;
	struct trfs_unlink_record *sample_record = NULL;

	printk("Trfs_Unlink called\n");
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
	sb=dir->i_sb;
	trfs_sb_info=(struct trfs_sb_info*)sb->s_fs_info;
	filename=trfs_sb_info->tracefile->filename;

	bitmap=trfs_sb_info->tracefile->bitmap;
	

 	if(bitmap & UNLINK_TR)
	{
	
		if(filename != NULL){
					
			sample_record= kzalloc(sizeof(struct trfs_unlink_record), GFP_KERNEL);
			if(sample_record == NULL){
				err = -ENOMEM;
				goto out_err;
				}

			temp = kmalloc(BUFFER_SIZE, GFP_KERNEL);
                    	//ar *path = d_path(dentry->dna, temp, PAGE_SIZE);
			path = dentry_path_raw(dentry, temp, BUFFER_SIZE);

            printk("Full Path is %s\n", path);

			sample_record->pathname_size = strlen(path);
			printk("pathname size is %d and path name is %s\n", sample_record->pathname_size, path);
					
			sample_record->pathname = kmalloc(sizeof(char)*sample_record->pathname_size, GFP_KERNEL);
			if(sample_record->pathname == NULL){
				err = -ENOMEM;
				goto out_err;
					}
			strcpy(sample_record->pathname,path);
			
			//memcpy((void *)sample_record->pathname, (void *)dentry->d_name.name, sample_record->pathname_size);
					//printk("Path name in record after memcpy is %s\n", sample_record->pathname);
			sample_record->record_size = sizeof(sample_record->record_id) + sizeof(sample_record->record_size) + sizeof(sample_record->record_type) + sizeof(sample_record->return_value)
                                           + sizeof(sample_record->pathname_size) + sample_record->pathname_size; 

			sample_record->record_type = UNLINK_TR; //char 0 will represent 
			sample_record->return_value = err;
                                        
				printk("Sample Record -\n");
//				printk("record_id is %d\n", sample_record->record_id);
				printk("record_size is %d\n", sample_record->record_size);
				printk("record_type is %d\n", sample_record->record_type);
				printk("return_value is %d\n", sample_record->return_value);
				printk("pathname_size is %d\n", sample_record->pathname_size);
				printk("pathname is %s\n", sample_record->pathname);
				//printk("pathname is %d\n", sample_record->permission_mode);
				if(trfs_sb_info->tracefile->buffer_offset + sample_record->record_size >= 2*BUFFER_SIZE){
				mutex_lock(&trfs_sb_info->tracefile->record_lock);

                oldfs = get_fs();
        		set_fs(get_ds());

        		//Flush the buffer to file and reset the offset to 0
                retVal = vfs_write(filename, trfs_sb_info->tracefile->buffer, trfs_sb_info->tracefile->buffer_offset, &trfs_sb_info->tracefile->offset);
             	printk("number of bytes written %d\n", retVal);
             	trfs_sb_info->tracefile->buffer_offset = 0;

                set_fs(oldfs);
                mutex_unlock(&trfs_sb_info->tracefile->record_lock);
			}
				mutex_lock(&trfs_sb_info->tracefile->record_lock);
				sample_record->record_id = trfs_sb_info->tracefile->record_id++;				
					
			
				memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->record_size, sizeof(short));
				trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(short);
				memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->record_type, sizeof(int));
				trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(int);

			

                memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->return_value, sizeof(int));
                trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(int);
				memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->record_id, sizeof(int));
				trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(int);	

				memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->pathname_size, sizeof(short));
				trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(short);

				memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)sample_record->pathname, sample_record->pathname_size);
				trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sample_record->pathname_size;
    			mutex_unlock(&trfs_sb_info->tracefile->record_lock);
					}
	}


out_err:

	if(sample_record){
		if(sample_record->pathname){
			kfree(sample_record->pathname);
		}
		kfree(sample_record);
	}

	if(temp)
		kfree(temp);



	return err;
}

static int trfs_symlink(struct inode *dir, struct dentry *dentry,
			  const char *symname)
{
	int err, retVal, bitmap;
	struct dentry *lower_dentry;
	struct dentry *lower_parent_dentry = NULL;
	struct path lower_path;

	struct trfs_sb_info *trfs_sb_info;
	struct super_block *sb;
	struct trfs_symlink_record *sample_record;
	char *temp, *path;
	struct file *filename;
    mm_segment_t oldfs;

	temp = NULL;
	sample_record = NULL;
	sb = NULL;

	printk("trfs_symlink called\n");
	printk("symname is %s\n", symname);
	sb = dir->i_sb;

	trfs_sb_info=(struct trfs_sb_info*)sb->s_fs_info;
    filename=trfs_sb_info->tracefile->filename;
    bitmap=trfs_sb_info->tracefile->bitmap;

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

	if(bitmap & SYMLINK_TR)
	{
		if(filename != NULL){
					
			sample_record= kzalloc(sizeof(struct trfs_symlink_record), GFP_KERNEL);
			if(sample_record == NULL){
				err = -ENOMEM;
				goto out_err;
			}
			sample_record->targetpath = NULL;

			temp = kmalloc(BUFFER_SIZE, GFP_KERNEL);
			if(temp == NULL){
				err = -ENOMEM;
				goto out_err;
			}
                    	//ar *path = d_path(dentry->dna, temp, PAGE_SIZE);
			path = dentry_path_raw(dentry, temp, BUFFER_SIZE);

            printk("Full Path is %s\n", path);

			sample_record->linkpath_size = strlen(path);
			printk("pathname size is %d and path name is %s\n", sample_record->linkpath_size, path);
					
			sample_record->linkpath = kzalloc(sizeof(char)*sample_record->linkpath_size, GFP_KERNEL);
			if(sample_record->linkpath == NULL){
				err = -ENOMEM;
				goto out_err;
			}
			strcpy(sample_record->linkpath ,path);

			sample_record->targetpath_size = strlen(symname);
			
			//memcpy((void *)sample_record->pathname, (void *)dentry->d_name.name, sample_record->pathname_size);
					//printk("Path name in record after memcpy is %s\n", sample_record->pathname);
			sample_record->record_size = sizeof(sample_record->record_id) + sizeof(sample_record->record_size) + sizeof(sample_record->record_type) + sizeof(sample_record->return_value)
                                           + sizeof(sample_record->targetpath_size) + sample_record->targetpath_size
                                           + sizeof(sample_record->linkpath_size) + sample_record->linkpath_size; 

			sample_record->record_type = SYMLINK_TR; //char 0 will represent 
			sample_record->return_value = err;                            
			
			//printk("link_path is %s\n", sample_record->linkpath);

			if(sample_record->targetpath_size > BUFFER_SIZE/2){
				printk("count > BUFFER_SIZE/2...BUFFER overflow in write\n");
                mutex_lock(&trfs_sb_info->tracefile->record_lock);

                oldfs = get_fs();
                set_fs(get_ds());

                //Flush the buffer to file and reset the offset to 0
                retVal = vfs_write(filename, trfs_sb_info->tracefile->buffer, trfs_sb_info->tracefile->buffer_offset, &trfs_sb_info->tracefile->offset);
                printk("number of bytes written %d\n", retVal);
                trfs_sb_info->tracefile->buffer_offset = 0;

                //Get the record ID for write record
                sample_record->record_id = trfs_sb_info->tracefile->record_id++;

                memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->record_size, sizeof(short));
				trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(short);

				memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->record_type, sizeof(int));
				trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(int);

                memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->return_value, sizeof(int));
                trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(int);

				memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->record_id, sizeof(int));
				trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(int);	

				memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->linkpath_size, sizeof(short));
				trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(short);

				memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)sample_record->linkpath, sample_record->linkpath_size);
				trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sample_record->linkpath_size;

				memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->targetpath_size, sizeof(short));
				trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(short);

				retVal = vfs_write(filename, trfs_sb_info->tracefile->buffer, trfs_sb_info->tracefile->buffer_offset, &trfs_sb_info->tracefile->offset);
                printk("number of bytes written %d\n", retVal);
                trfs_sb_info->tracefile->buffer_offset = 0;

                //Write the entire write-buf to the file 
                retVal = vfs_write(filename, symname, strlen(symname), &trfs_sb_info->tracefile->offset);
                printk("writing the symlink name path directly to log file\n");

                set_fs(oldfs);
                mutex_unlock(&trfs_sb_info->tracefile->record_lock);
			}
			else{
				sample_record->targetpath = kzalloc(strlen(symname), GFP_KERNEL);
				if(sample_record->targetpath == NULL){
                    err = -ENOMEM;
                    goto out;
                }
				strcpy(sample_record->targetpath ,symname);
                

                //Check if this record size can fit inside the buffer with current offset
                if(trfs_sb_info->tracefile->buffer_offset + sample_record->record_size >= 2*BUFFER_SIZE){
                    printk("count + BUFFER_offset > 2*BUFFER_SIZE...BUFFER overflow in write\n");
                    mutex_lock(&trfs_sb_info->tracefile->record_lock);

                    oldfs = get_fs();
                    set_fs(get_ds());

                    //Flush the buffer to file and reset the offset to 0
                    retVal = vfs_write(filename, trfs_sb_info->tracefile->buffer, trfs_sb_info->tracefile->buffer_offset, &trfs_sb_info->tracefile->offset);
                    printk("number of bytes written %d\n", retVal);
                    trfs_sb_info->tracefile->buffer_offset = 0;

                    set_fs(oldfs);
                    mutex_unlock(&trfs_sb_info->tracefile->record_lock);
                }

                printk("Sample symlink Record -\n");
//				printk("record_id is %d\n", sample_record->record_id);
				printk("record_size is %d\n", sample_record->record_size);
				printk("record_type is %d\n", sample_record->record_type);
				printk("return_value is %d\n", sample_record->return_value);
				printk("linkpath_size is %d\n", sample_record->linkpath_size);
				printk("link path is %s\n", sample_record->linkpath);
				printk("targetpath_size is %d\n", sample_record->targetpath_size);
				printk("target path is %s\n", sample_record->targetpath);

                mutex_lock(&trfs_sb_info->tracefile->record_lock);
                sample_record->record_id = trfs_sb_info->tracefile->record_id++;

                memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->record_size, sizeof(short));
				trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(short);

				memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->record_type, sizeof(int));
				trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(int);

                memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->return_value, sizeof(int));
                trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(int);

				memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->record_id, sizeof(int));
				trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(int);	
				

				memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->linkpath_size, sizeof(short));
				trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(short);

				memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)sample_record->linkpath, sample_record->linkpath_size);
				trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sample_record->linkpath_size;

				memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->targetpath_size, sizeof(short));
				trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(short);

				memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)sample_record->targetpath, sample_record->targetpath_size);
				trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sample_record->targetpath_size;


				mutex_unlock(&trfs_sb_info->tracefile->record_lock);
			}
		}
	}


out_err:

	if(sample_record){
		if(sample_record->targetpath){
			kfree(sample_record->targetpath);
		}
		if(sample_record->linkpath){
			kfree(sample_record->linkpath);
		}
		kfree(sample_record);
	}

	if(temp)
		kfree(temp);

	return err;
}

static int trfs_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode)
{
	
	int err;
	struct dentry *lower_dentry;
	struct dentry *lower_parent_dentry = NULL;
	
	struct path lower_path;
	struct super_block *sb;
	struct trfs_sb_info *trfs_sb_info;
	//unsigned long long offset;
	struct file *filename=NULL;
	//char *data=NULL;
	char *temp=NULL;
	int bitmap;
	char *path;
	mm_segment_t oldfs;
	int retVal;
	struct trfs_mkdir_record *sample_record = NULL;

	printk("trfs_mkdir called\n");

	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	lower_parent_dentry = lock_parent(lower_dentry);
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

out:	unlock_dir(lower_parent_dentry);
        trfs_put_lower_path(dentry, &lower_path);
	/*Get Superblock info*/

		sb=dir->i_sb;
		trfs_sb_info=(struct trfs_sb_info*)sb->s_fs_info;
		filename=trfs_sb_info->tracefile->filename;
	
		bitmap=trfs_sb_info->tracefile->bitmap;
	
	 	if(bitmap & MKDIR_TR)
		{
 	
			if(filename != NULL){

   			
						
				sample_record= kzalloc(sizeof(struct trfs_mkdir_record), GFP_KERNEL);
				if(sample_record == NULL){
					err = -ENOMEM;
					goto out_err;
					}

				temp = kmalloc(BUFFER_SIZE, GFP_KERNEL);
	                    	//ar *path = d_path(dentry->dna, temp, PAGE_SIZE);
				path = dentry_path_raw(dentry, temp, BUFFER_SIZE);

	            printk("Full Path is %s\n", path);

				sample_record->pathname_size = strlen(path);
				printk("pathname size is %d and path name is %s\n", sample_record->pathname_size, path);
						
				sample_record->pathname = kmalloc(sizeof(char)*sample_record->pathname_size, GFP_KERNEL);
				if(sample_record->pathname == NULL){
					err = -ENOMEM;
					goto out_err;
						}
				strcpy(sample_record->pathname,path);
				
				//memcpy((void *)sample_record->pathname, (void *)dentry->d_name.name, sample_record->pathname_size);
						//printk("Path name in record after memcpy is %s\n", sample_record->pathname);
				sample_record->record_size = sizeof(sample_record->record_id) + sizeof(sample_record->record_size) + sizeof(sample_record->record_type)
                                               + sizeof(sample_record->permission_mode)+ sizeof(sample_record->return_value)
                                               + sizeof(sample_record->pathname_size) + sample_record->pathname_size; 
                                            
						
				sample_record->record_type = MKDIR_TR; //char 0 will represent 
				sample_record->permission_mode = mode;
				sample_record->return_value = err;
//--------------------------Set value of mkdir records----------------------------------------------------------------------------------------------------------------------

						
				printk("Sample Record -\n");
//				printk("record_id is %d\n", sample_record->record_id);
				printk("record_size is %d\n", sample_record->record_size);
				printk("record_type is %d\n", sample_record->record_type);
				printk("return_value is %d\n", sample_record->return_value);
				printk("pathname_size is %d\n", sample_record->pathname_size);
				printk("pathname is %s\n", sample_record->pathname);
				printk("pathname is %d\n", sample_record->permission_mode);
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------


	//					printk("my bitmap value is %d\n", sample_record->mybitmap);

		if(trfs_sb_info->tracefile->buffer_offset + sample_record->record_size >= 2*BUFFER_SIZE){
				mutex_lock(&trfs_sb_info->tracefile->record_lock);

                oldfs = get_fs();
        		set_fs(get_ds());

        		//Flush the buffer to file and reset the offset to 0
                retVal = vfs_write(filename, trfs_sb_info->tracefile->buffer, trfs_sb_info->tracefile->buffer_offset, &trfs_sb_info->tracefile->offset);
             	printk("number of bytes written %d\n", retVal);
             	trfs_sb_info->tracefile->buffer_offset = 0;

                set_fs(oldfs);
                mutex_unlock(&trfs_sb_info->tracefile->record_lock);
			}


				
	 			mutex_lock(&trfs_sb_info->tracefile->record_lock);
				sample_record->record_id = trfs_sb_info->tracefile->record_id++;				
					
			
				memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->record_size, sizeof(short));
				trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(short);
				memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->record_type, sizeof(int));
				trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(int);

				memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->permission_mode, sizeof(int));
				trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(int);

                memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->return_value, sizeof(int));
                trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(int);
				memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->record_id, sizeof(int));
				trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(int);	

				memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->pathname_size, sizeof(short));
				trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(short);

				memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)sample_record->pathname, sample_record->pathname_size);
				trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sample_record->pathname_size;
    			mutex_unlock(&trfs_sb_info->tracefile->record_lock);
					}
	}


out_err:

	if(sample_record){
		if(sample_record->pathname){
			kfree(sample_record->pathname);
		}
		kfree(sample_record);
	}

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
	//----Declarations-------
 	struct super_block *sb;
	struct trfs_sb_info *trfs_sb_info;
	//unsigned long long offset;
	struct file *filename=NULL;
	char *temp=NULL;
	int bitmap;
	char *path;
	mm_segment_t oldfs;
	int retVal;
	struct trfs_rmdir_record *sample_record = NULL;

	printk("Trfs_Rmdir called\n");





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
	sb=dir->i_sb;
	trfs_sb_info=(struct trfs_sb_info*)sb->s_fs_info;
	filename=trfs_sb_info->tracefile->filename;

	bitmap=trfs_sb_info->tracefile->bitmap;
		

 	if(bitmap & RMDIR_TR)
	{
	 
		if(filename != NULL){
					
			sample_record= kzalloc(sizeof(struct trfs_rmdir_record), GFP_KERNEL);
			if(sample_record == NULL){
				err = -ENOMEM;
				goto out_err;
				}

			temp = kmalloc(BUFFER_SIZE, GFP_KERNEL);
                    	//ar *path = d_path(dentry->dna, temp, PAGE_SIZE);
			path = dentry_path_raw(dentry, temp, BUFFER_SIZE);

            printk("Full Path is %s\n", path);

			sample_record->pathname_size = strlen(path);
			printk("pathname size is %d and path name is %s\n", sample_record->pathname_size, path);
					
			sample_record->pathname = kmalloc(sizeof(char)*sample_record->pathname_size, GFP_KERNEL);
			if(sample_record->pathname == NULL){
				err = -ENOMEM;
				goto out_err;
					}
			strcpy(sample_record->pathname,path);
			
			//memcpy((void *)sample_record->pathname, (void *)dentry->d_name.name, sample_record->pathname_size);
					//printk("Path name in record after memcpy is %s\n", sample_record->pathname);
			sample_record->record_size = sizeof(sample_record->record_id) + sizeof(sample_record->record_size) + sizeof(sample_record->record_type)
                                          + sizeof(sample_record->return_value)
                                           + sizeof(sample_record->pathname_size) + sample_record->pathname_size; 
            sample_record->record_type = RMDIR_TR; //char 0 will represent 
			sample_record->return_value = err;
                                        
				printk("Sample Record -\n");
//				printk("record_id is %d\n", sample_record->record_id);
				printk("record_size is %d\n", sample_record->record_size);
				printk("record_type is %d\n", sample_record->record_type);
				printk("return_value is %d\n", sample_record->return_value);
				printk("pathname_size is %d\n", sample_record->pathname_size);
				printk("pathname is %s\n", sample_record->pathname);
				//printk("pathname is %d\n", sample_record->permission_mode);
				if(trfs_sb_info->tracefile->buffer_offset + sample_record->record_size >= 2*BUFFER_SIZE){
					mutex_lock(&trfs_sb_info->tracefile->record_lock);

                	oldfs = get_fs();
        			set_fs(get_ds());

        		//Flush the buffer to file and reset the offset to 0
                	retVal = vfs_write(filename, trfs_sb_info->tracefile->buffer, trfs_sb_info->tracefile->buffer_offset, &trfs_sb_info->tracefile->offset);
             		printk("number of bytes written %d\n", retVal);
             		trfs_sb_info->tracefile->buffer_offset = 0;

                	set_fs(oldfs);
                	mutex_unlock(&trfs_sb_info->tracefile->record_lock);
			}
				mutex_lock(&trfs_sb_info->tracefile->record_lock);
				sample_record->record_id = trfs_sb_info->tracefile->record_id++;				
					
			
				memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->record_size, sizeof(short));
				trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(short);
				memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->record_type, sizeof(int));
				trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(int);

			

                memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->return_value, sizeof(int));
                trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(int);
				memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->record_id, sizeof(int));
				trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(int);	

				memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->pathname_size, sizeof(short));
				trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(short);
				memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)sample_record->pathname, sample_record->pathname_size);
				trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sample_record->pathname_size;
    			mutex_unlock(&trfs_sb_info->tracefile->record_lock);
					}
	}


out_err:

	if(sample_record){
		if(sample_record->pathname){
			kfree(sample_record->pathname);
		}
		kfree(sample_record);
	}

	if(temp)
		kfree(temp);
                            

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

	int err = 0, retVal, bitmap;
	struct dentry *lower_old_dentry = NULL;
	struct dentry *lower_new_dentry = NULL;
	struct dentry *lower_old_dir_dentry = NULL;
	struct dentry *lower_new_dir_dentry = NULL;
	struct dentry *trap = NULL;
	struct path lower_old_path, lower_new_path;

	struct trfs_rename_record *sample_record = NULL; 
	char *temp = NULL, *old_path = NULL, *new_path = NULL;
	struct file* filename;
	struct super_block *sb;
    struct trfs_sb_info *trfs_sb_info;
    mm_segment_t oldfs;

    printk("Trfs_rename called\n");
    sb=old_dir->i_sb;

    trfs_sb_info=(struct trfs_sb_info*)sb->s_fs_info;
    filename=trfs_sb_info->tracefile->filename;
    bitmap=trfs_sb_info->tracefile->bitmap;

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

	if(bitmap & RENAME_TR)
	{
		if(filename != NULL){
					
			sample_record= kzalloc(sizeof(struct trfs_rename_record), GFP_KERNEL);
			if(sample_record == NULL){
				err = -ENOMEM;
				goto out_err;
			}

			temp = kmalloc(BUFFER_SIZE, GFP_KERNEL);
			old_path = dentry_path_raw(old_dentry, temp, BUFFER_SIZE);

			printk("Old Path is %s\n", old_path);

            sample_record->oldpathsize = strlen(old_path);
            printk("pathname size is %d and path name is %s\n", sample_record->oldpathsize, old_path);

            sample_record->oldpath = kmalloc(sizeof(char)*sample_record->oldpathsize, GFP_KERNEL);
            if(sample_record->oldpath == NULL){
                err = -ENOMEM;
                goto out_err;
            }
            strcpy(sample_record->oldpath,old_path);

			new_path = dentry_path_raw(new_dentry, temp, PAGE_SIZE);
			printk("new Path is %s\n", new_path);
            sample_record->newpathsize = strlen(new_path);
            printk("pathname size is %d and path name is %s\n", sample_record->newpathsize, new_path);
            
            
            sample_record->record_size = sizeof(sample_record->record_id) + sizeof(sample_record->record_size) + sizeof(sample_record->record_type)
										+ sizeof(sample_record->oldpathsize) + sample_record->oldpathsize
                           				+ sizeof(sample_record->newpathsize)  + sample_record->newpathsize
                           				+ sizeof(sample_record->return_value) ;

            sample_record->record_type = RENAME_TR; //char 0 will represent
            sample_record->return_value = err;

            if(sample_record->newpathsize > BUFFER_SIZE/2){
                //Write count is greater than page size...so flush the buffer
                printk("count > BUFFER_SIZE/2...BUFFER overflow in write\n");
                mutex_lock(&trfs_sb_info->tracefile->record_lock);

                oldfs = get_fs();
                set_fs(get_ds());

                //Flush the buffer to file and reset the offset to 0
                retVal = vfs_write(filename, trfs_sb_info->tracefile->buffer, trfs_sb_info->tracefile->buffer_offset, &trfs_sb_info->tracefile->offset);
                printk("number of bytes written %d\n", retVal);
                trfs_sb_info->tracefile->buffer_offset = 0;

                //Get the record ID for write record
                sample_record->record_id = trfs_sb_info->tracefile->record_id++;

                //Push rest of write record details
                memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->record_size, sizeof(short));
                trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(short);

                memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->record_type, sizeof(int));
                trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(int);


                memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->oldpathsize, sizeof(short));
                trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(short);

                memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)sample_record->oldpath, sample_record->oldpathsize);
                trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sample_record->oldpathsize;

                memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->return_value, sizeof(int));
                trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(int);
			

                memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->record_id, sizeof(int));
                trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(int);

               
  				memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->newpathsize, sizeof(short));
                trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(short);

                retVal = vfs_write(filename, trfs_sb_info->tracefile->buffer, trfs_sb_info->tracefile->buffer_offset, &trfs_sb_info->tracefile->offset);
                printk("number of bytes written %d\n", retVal);
                trfs_sb_info->tracefile->buffer_offset = 0;

                //Write the entire write-buf to the file 
                retVal = vfs_write(filename, new_path, sample_record->newpathsize, &trfs_sb_info->tracefile->offset);
                printk("writing the new path directly to log file\n");

                set_fs(oldfs);
                mutex_unlock(&trfs_sb_info->tracefile->record_lock);

            }
            else{

				sample_record->newpath = kmalloc(sizeof(char)*sample_record->newpathsize, GFP_KERNEL);
            	if(sample_record->newpath == NULL){
                    err = -ENOMEM;
                    goto out_err;
                }

            	strcpy(sample_record->newpath,new_path);

				if(trfs_sb_info->tracefile->buffer_offset + sample_record->record_size >= 2*BUFFER_SIZE){
	                mutex_lock(&trfs_sb_info->tracefile->record_lock);

    				oldfs = get_fs();
	                set_fs(get_ds());

	                //Flush the buffer to file and reset the offset to 0
	                retVal = vfs_write(filename, trfs_sb_info->tracefile->buffer, trfs_sb_info->tracefile->buffer_offset, &trfs_sb_info->tracefile->offset);
	                printk("number of bytes written %d\n", retVal);
	                trfs_sb_info->tracefile->buffer_offset = 0;

	                set_fs(oldfs);
	                mutex_unlock(&trfs_sb_info->tracefile->record_lock);
				}


                mutex_lock(&trfs_sb_info->tracefile->record_lock);
                sample_record->record_id = trfs_sb_info->tracefile->record_id++;

                printk("Sample Record -\n");
                printk("record_id is %d\n", sample_record->record_id);
                printk("record_size is %d\n", sample_record->record_size);
                printk("record_type is %d\n", sample_record->record_type);
                printk("return_value is %d\n", sample_record->return_value);

          
                memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->record_size, sizeof(short));
                trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(short);

                memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->record_type, sizeof(int));
                trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(int);


                memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->oldpathsize, sizeof(short));
                trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(short);

                memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)sample_record->oldpath, sample_record->oldpathsize);
                trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sample_record->oldpathsize;

                memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->return_value, sizeof(int));
                trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(int);
			

                memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->record_id, sizeof(int));
                trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(int);

               
  				memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->newpathsize, sizeof(short));
                trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(short);

  				memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)sample_record->newpath, sample_record->newpathsize);
                trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sample_record->newpathsize;

                mutex_unlock(&trfs_sb_info->tracefile->record_lock);

			}
        }
    }
out_err:
	if(sample_record){
        if(sample_record->oldpath){
                kfree(sample_record->oldpath);
        }
		 if(sample_record->newpath){
            kfree(sample_record->newpath);
    	}

		kfree(sample_record);
	}
	if(temp)
        kfree(temp);

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

	int err, retVal, bitmap;
	struct dentry *lower_dentry;
	struct path lower_path;

	struct super_block *sb;
    struct trfs_sb_info *trfs_sb_info;
    struct file *filename;
    char *temp, *path;
    mm_segment_t oldfs;
    struct trfs_readlink_record *sample_record;

    temp = NULL;
    filename=NULL;
    sample_record = NULL;
    
    printk("Trfs_readlink called\n");
    sb=dentry->d_sb;
    
    trfs_sb_info=(struct trfs_sb_info*)sb->s_fs_info;
    filename=trfs_sb_info->tracefile->filename;
    bitmap=trfs_sb_info->tracefile->bitmap;

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

	//Tracking code
    if(bitmap & READLINK_TR){   
        if(filename != NULL){
            sample_record= kzalloc(sizeof(struct trfs_read_record), GFP_KERNEL);
            if(sample_record == NULL){
                err = -ENOMEM;
                goto out_err;
            }

            temp = kzalloc(BUFFER_SIZE, GFP_KERNEL);
            if(temp == NULL){
                err = -ENOMEM;
                goto out_err;
            }
            temp = kmalloc(BUFFER_SIZE, GFP_KERNEL);
                            //ar *path = d_path(dentry->dna, temp, PAGE_SIZE);
            path = dentry_path_raw(dentry, temp, BUFFER_SIZE);
            printk("Full Path is %s\n", path);

           
            sample_record->pathname_size = strlen(path);

            sample_record->pathname = kmalloc(sizeof(char)*sample_record->pathname_size, GFP_KERNEL);
            if(sample_record->pathname == NULL){
                    err = -ENOMEM;
                    goto out_err;
            }
            memcpy((void *)sample_record->pathname, (void *)path, sample_record->pathname_size);


            sample_record->record_size = sizeof(sample_record->record_id) + sizeof(sample_record->record_size) + sizeof(sample_record->record_type)
                                               + sizeof(sample_record->pathname_size) + sample_record->pathname_size 
                                               + sizeof(sample_record->size)
                                               + sizeof(sample_record->return_value);

            sample_record->record_type = READLINK_TR;
            sample_record->return_value = err;
            sample_record->size = bufsiz;

            printk("Sample Record -\n");
            printk("record_id is %d\n", sample_record->record_id);
            printk("record_size is %d\n", sample_record->record_size);
            printk("record_type is %d\n", sample_record->record_type);
            printk("return_value is %d\n", sample_record->return_value);
            printk("pathname_size - %d\n", sample_record->pathname_size);
            printk("pathname - %s\n", sample_record->pathname);
                


            //Check if this record size can fit inside the buffer with current offset
            if(trfs_sb_info->tracefile->buffer_offset + sample_record->record_size >= 2*BUFFER_SIZE){
                mutex_lock(&trfs_sb_info->tracefile->record_lock);

                oldfs = get_fs();
                set_fs(get_ds());

                //Flush the buffer to file and reset the offset to 0
                retVal = vfs_write(filename, trfs_sb_info->tracefile->buffer, trfs_sb_info->tracefile->buffer_offset, &trfs_sb_info->tracefile->offset);
                printk("number of bytes written %d\n", retVal);
                trfs_sb_info->tracefile->buffer_offset = 0;

                set_fs(oldfs);
                mutex_unlock(&trfs_sb_info->tracefile->record_lock);
            }

            mutex_lock(&trfs_sb_info->tracefile->record_lock);
            sample_record->record_id = trfs_sb_info->tracefile->record_id++;


            memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->record_size, sizeof(short));
            trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(short);

            memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->record_type, sizeof(int));
            trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(int);

            memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->pathname_size, sizeof(short));
            trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(short);

            memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)sample_record->pathname, sample_record->pathname_size);
            trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sample_record->pathname_size;

            memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->size, sizeof(unsigned long long));
            trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(unsigned long long);

            memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->return_value, sizeof(int));
            trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(int);

            memcpy((void *)(trfs_sb_info->tracefile->buffer + trfs_sb_info->tracefile->buffer_offset), (void *)&sample_record->record_id, sizeof(int));
            trfs_sb_info->tracefile->buffer_offset = trfs_sb_info->tracefile->buffer_offset + sizeof(int);          
            
            mutex_unlock(&trfs_sb_info->tracefile->record_lock);
        }
    }
    
out_err:
    if(sample_record){
        if(sample_record->pathname)
            kfree(sample_record->pathname);
        kfree(sample_record);
    }
    if(temp)
        kfree(temp);
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
