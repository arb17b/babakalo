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
	
	
	
	if(file->of_accmode == O_WRONLY){
		kprintf("\nBnara file khulechish write er jonno be!");
		return 1;
	}
	struct uio reader;
	struct iovec io;
	uio_kinit(&io, &reader, &buf, size, file->of_offset, UIO_READ);
	reader.uio_segflg = UIO_USERSPACE;
	reader.uio_space = curproc->p_addrspace;
	result = VOP_READ(file->of_vnode, &reader);
	*retval = size - reader.uio_resid;
	filetable_put(curproc->p_filetable,fd, file);

       return result;
}

/*
 * write() - write data to a file
 */

int
sys_write(int fd, userptr_t buf, size_t size, int *retval)
{
        int result = 0;

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
	
	if(file->of_accmode == O_RDONLY){
		kprintf("\nBnara file khulechish write er jonno be!");
		return 1;
	}
	struct uio reader;
	struct iovec io;
	uio_kinit(&io, &reader, &buf, size, file->of_offset, UIO_READ);
	reader.uio_segflg = UIO_USERSPACE;
	reader.uio_space = curproc->p_addrspace;
	result = VOP_WRITE(file->of_vnode, &reader);
	*retval = size - reader.uio_resid;
	filetable_put(curproc->p_filetable,fd, file);

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
