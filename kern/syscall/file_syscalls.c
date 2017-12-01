/*
 * File-related system call implementations.
 */

#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <kern/limits.h>
#include <kern/seek.h>
#include <kern/stat.h>
#include <lib.h>
#include <uio.h>
#include <proc.h>
#include <current.h>
#include <synch.h>
#include <copyinout.h>
#include <vfs.h>
#include <vnode.h>
#include <openfile.h>
#include <filetable.h>
#include <syscall.h>

/*
 * open() - get the path with copyinstr, then use openfile_open and
 * filetable_place to do the real work.
 */
int
sys_open(const_userptr_t upath, int flags, mode_t mode, int *retval)
{
	const int allflags = O_ACCMODE | O_CREAT | O_EXCL | O_TRUNC | O_APPEND | O_NOCTTY;
	
	char *kpath;
	struct openfile *file;
	int result = 0;
	size_t act;
	

	/* 
	 * Your implementation of system call open starts here.  
	 *
	 * Check the design document design/filesyscall.txt for the steps
	 */
	
	if((flags & allflags)  != allflags){
	//	kprintf("\nBawal aaache babla re");
	//	return EINVAL;
	}
	kpath = (char*)kmalloc(sizeof(char)*PATH_MAX);
	result = copyinstr(upath, kpath, PATH_MAX, &act);
	kprintf("\n%d", result);
	
	if(result){
		kfree(kpath);
		return result;
	}

	result = openfile_open(kpath, flags, mode, &file);
	kprintf("\n%d", result);
	if(result){
		kfree(kpath);
		return result;
	}

	result = filetable_place(curproc->p_filetable, file, retval);
	kprintf("\n%d", result);
	if(result){
		kfree(kpath);
		return result;
	}

	return result;
}

/*
 * read() - read data from a file
 */
int
sys_read(int fd, userptr_t buf, size_t size, int *retval)
{
        int result = 0;
	int pos;
	bool locked;
       /* 
        * Your implementation of system call read starts here.  
        *
        * Check the design document design/filesyscall.txt for the steps
        */
	struct openfile* file;
	result = filetable_get(curproc->p_filetable, fd, &file);
	
	if(result){
	   kprintf("\nBawal aache");
	   return result;
	}
	
	locked = VOP_ISSEEKABLE(file->of_vnode);
	if(locked){
		lock_acquire(file->of_offsetlock);
		pos = file->of_offset;
	}
	else
		pos = 0;
	
	
	if(file->of_accmode == O_WRONLY){
		kprintf("\nBnara file khulechish write er jonno be!");
		return 1;
	}
	struct uio reader;
	struct iovec io;
	uio_kinit(&io, &reader, buf, size, pos, UIO_READ);
	reader.uio_segflg = UIO_USERSPACE;
	reader.uio_space = proc_getas();
	result = VOP_READ(file->of_vnode, &reader);
	
	if(locked){
		file->of_offset = reader.uio_offset;
		lock_release(file->of_offsetlock);
	}
	filetable_put(curproc->p_filetable,fd, file);
	if(!result)
		*retval = size - reader.uio_resid;
	

       return result;
}

/*
 * write() - write data to a file
 */

int
sys_write(int fd, userptr_t buf, size_t size, int *retval)
{
	int result = 0;
	int pos;
 	bool locked;
       /* 
        * Your implementation of system call read starts here.  
        *
        * Check the design document design/filesyscall.txt for the steps
        */
	struct openfile* file;
	result = filetable_get(curproc->p_filetable, fd, &file);
	
	if(result){
	   kprintf("\nBawal aache");
	   return result;
	}
	
	locked = VOP_ISSEEKABLE(file->of_vnode);
	if(locked){
		lock_acquire(file->of_offsetlock);
		pos = file->of_offset;
	}
	else
		pos = 0;
	
	
	if(file->of_accmode == O_RDONLY){
		kprintf("\nBnara file khulechish write er jonno be!");
		return 1;
	}
	struct uio reader;
	struct iovec io;
	uio_kinit(&io, &reader, buf, size, pos, UIO_WRITE);
	reader.uio_segflg = UIO_USERSPACE;
	reader.uio_space = proc_getas();
	result = VOP_WRITE(file->of_vnode, &reader);
	
	if(locked){
		file->of_offset = reader.uio_offset;
		lock_release(file->of_offsetlock);
	}
	filetable_put(curproc->p_filetable,fd, file);
	if(!result)
		*retval = size - reader.uio_resid;
	

       return result;
}

/*
 * close() - remove from the file table.
 */

int 
sys_close(int fd){
	
	struct openfile *oldfile_ret;
	kprintf("\nSHHSHS");
	if(!filetable_okfd(curproc->p_filetable, fd)) {
		kprintf("CLOSE- Bad filehandle\n");
		return EBADF;
	}
	kprintf("\nSHHSHS");
	filetable_placeat(curproc->p_filetable, NULL, fd, &oldfile_ret);
	kprintf("\nSHHSHS");
	openfile_decref(oldfile_ret);
	return 0;
	
}

/* 
* meld () - combine the content of two files word by word into a new file
*/

int 
sys_meld(userptr_t upath1, userptr_t upath2, userptr_t upath3, int *retval){
	
	int fd1, fd2, fd3;
	struct openfile *file1;
	struct openfile *file2;
	struct openfile *file3;
	
	char* duffer1 = kmalloc(1024);
	char* duffer2 = kmalloc(1024);
	char* duffer3 = kmalloc(2048);
	
	struct iovec iov1, iov2, iov3;
	struct uio ui1, ui2, ui3;
	
	int i, result;
	int l1, l2;
	
	result = sys_open(upath1,O_RDONLY, 0664, &fd1);
	if(result)
		return result;
	result = filetable_get(curproc->p_filetable, fd1, &file1);
	if(result)
		return result;
	result = sys_open(upath2,O_RDONLY, 0664, &fd2);
	if(result)
		return result;
	result = filetable_get(curproc->p_filetable, fd2, &file2);
	if(result)
		return result;
	result = sys_open(upath3,O_CREAT, 0664, &fd3);
	if(result)
		return result;
	result = filetable_get(curproc->p_filetable, fd3, &file3);
	if(result)
		return result;
	
	uio_kinit(&iov1, &ui1, duffer1, 1024, 0, UIO_READ);
	uio_kinit(&iov2, &ui2, duffer2, 1024, 0, UIO_READ);
	
	result = VOP_READ(file1->of_vnode, &ui1);
	if(result)
		return result;
	l1 = 1024 - ui1.uio_resid;
	for(i=0;i<(1024-l1);i++)
		duffer1[i] = ' ';
	result = VOP_READ(file2->of_vnode, &ui2);
	if(result)
		return result;
	l2 = 1024 - ui2.uio_resid;
	for(i=0;i<(1024-l2);i++)
		duffer2[i] = ' ';
	for(i=0;(i*8 < 2048); i++){
		duffer3[i*8] = duffer1[i*4];
		duffer3[i*8 + 1] = duffer1[i*4 + 1];
		duffer3[i*8 + 2] = duffer1[i*4 + 2];
		duffer3[i*8 + 3] = duffer1[i*4 + 3];
		duffer3[i*8 + 4] = duffer2[i*4];
		duffer3[i*8 + 5] = duffer2[i*4 + 1];
		duffer3[i*8 + 6] = duffer2[i*4 + 2];
		duffer3[i*8 + 7] = duffer2[i*4 + 3];
	}
	uio_kinit(&iov3, &ui3, duffer3, 2048, 0, UIO_WRITE);
	result = VOP_WRITE(file3->of_vnode, &ui3);
	if(result)
		return result;
	l2 = 2048 - ui3.uio_resid;
	*retval = 0;
	return 0;
}
