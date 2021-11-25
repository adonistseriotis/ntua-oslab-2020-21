/*
 * crypto-chrdev.c
 *
 * Implementation of character devices
 * for virtio-cryptodev device 
 *
 * Vangelis Koukis <vkoukis@cslab.ece.ntua.gr>
 * Dimitris Siakavaras <jimsiak@cslab.ece.ntua.gr>
 * Stefanos Gerangelos <sgerag@cslab.ece.ntua.gr>
 *
 */
#include <linux/cdev.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/wait.h>
#include <linux/virtio.h>
#include <linux/virtio_config.h>

#include "crypto.h"
#include "crypto-chrdev.h"
#include "debug.h"

#include "cryptodev.h"

/*
 * Global data
 */
struct cdev crypto_chrdev_cdev;

/**
 * Given the minor number of the inode return the crypto device 
 * that owns that number.
 **/
static struct crypto_device *get_crypto_dev_by_minor(unsigned int minor)
{
	struct crypto_device *crdev;
	unsigned long flags;

	debug("Entering");

	spin_lock_irqsave(&crdrvdata.lock, flags);
	list_for_each_entry(crdev, &crdrvdata.devs, list) {
		if (crdev->minor == minor)
			goto out;
	}
	crdev = NULL;

out:
	spin_unlock_irqrestore(&crdrvdata.lock, flags);

	debug("Leaving");
	return crdev;
}

/*************************************
 * Implementation of file operations
 * for the Crypto character device
 *************************************/

static int crypto_chrdev_open(struct inode *inode, struct file *filp)
{
	int ret = 0;
	int err;
	unsigned int len, num_in, num_out;
	struct crypto_open_file *crof;
	struct crypto_device *crdev;
	unsigned int *syscall_type;
	int *host_fd;
	struct scatterlist syscall_type_sg,host_fd_sg, *sgs[2];
	unsigned long flags;

	debug("Entering");

	syscall_type = kzalloc(sizeof(*syscall_type), GFP_KERNEL);
	*syscall_type = VIRTIO_CRYPTODEV_SYSCALL_OPEN;
	host_fd = kzalloc(sizeof(*host_fd), GFP_KERNEL);
	*host_fd = -1;

	num_in = 0;
	num_out = 0;

	ret = -ENODEV;
	if ((ret = nonseekable_open(inode, filp)) < 0)
		goto fail;

	/* Associate this open file with the relevant crypto device. */
	crdev = get_crypto_dev_by_minor(iminor(inode));
	if (!crdev) {
		debug("Could not find crypto device with %u minor", 
		      iminor(inode));
		ret = -ENODEV;
		goto fail;
	}

	crof = kzalloc(sizeof(*crof), GFP_KERNEL);
	if (!crof) {
		ret = -ENOMEM;
		goto fail;
	}
	crof->crdev = crdev;
	crof->host_fd = -1;
	filp->private_data = crof;

	/**
	 * We need two sg lists, one for syscall_type and one to get the 
	 * file descriptor from the host.
	 **/
	/* ?? */
	debug("ALL GOOD UNTIL HERE");
	sg_init_one(&syscall_type_sg, syscall_type, sizeof(*syscall_type));
	sgs[num_out++] = &syscall_type_sg;

	sg_init_one(&host_fd_sg, &crof->host_fd, sizeof(crof->host_fd));
	sgs[num_out + num_in++] = &host_fd_sg;

	debug("ENTERING SEMAPHORE");
	/**
	 * Wait for the host to process our data.
	 **/
	/* ?? */
	spin_lock_irqsave(&crdev->lock,flags);

	err = virtqueue_add_sgs(crdev->vq, sgs, num_out, num_in,
	                        &syscall_type_sg, GFP_ATOMIC);
	virtqueue_kick(crdev->vq);

	while (virtqueue_get_buf(crdev->vq, &len) == NULL)
		/* do nothing */;

	spin_unlock_irqrestore(&crdev->lock,flags);

	/* If host failed to open() return -ENODEV. */
	/* ?? */
	debug("OUT OF SEMAPHORE");
	if(crof->host_fd <0)
		return -ENODEV;

	debug("OMG IT OPENED fd = %d", crof->host_fd);
fail:
	debug("Leaving");
	return ret;
}

static int crypto_chrdev_release(struct inode *inode, struct file *filp)
{
	int err,ret;
	unsigned int len,num_out;
	unsigned int num_in;
	struct crypto_open_file *crof = filp->private_data;
	struct crypto_device *crdev = crof->crdev;
	struct virtqueue *vq = crdev->vq;
	struct scatterlist syscall_type_sg,host_fd_sg, *sgs[3];
	unsigned int *syscall_type;
	unsigned long flags;

	debug("Entering release");

	syscall_type = kzalloc(sizeof(*syscall_type), GFP_KERNEL);
	*syscall_type = VIRTIO_CRYPTODEV_SYSCALL_CLOSE;


	num_out = 0;
	num_in = 0;
	ret = 0;
	/**
	 * Send data to the host.
	 **/
	/* ?? */
	sg_init_one(&syscall_type_sg, syscall_type, sizeof(*syscall_type));
	sg_init_one(&host_fd_sg, &crof->host_fd, sizeof(crof->host_fd));
	
	sgs[num_out++] = &syscall_type_sg;
	sgs[num_out++] = &host_fd_sg;

	/**
	 * Wait for the host to process our data.
	 **/
	/* ?? */
	spin_lock_irqsave(&crdev->lock, flags);

	err = virtqueue_add_sgs(vq, sgs, num_out, num_in,
	                        &syscall_type_sg, GFP_ATOMIC);
	virtqueue_kick(vq);

	while (virtqueue_get_buf(vq, &len) == NULL)
		/* do nothing */;

	spin_unlock_irqrestore(&crdev->lock, flags);

	kfree(crof);
	debug("Leaving");
	return ret;

}

static long crypto_chrdev_ioctl(struct file *filp, unsigned int cmd, 
                                unsigned long arg)
{
	long ret = 0;
	int err;
	struct crypto_open_file *crof = filp->private_data;
	struct crypto_device *crdev = crof->crdev;
	struct virtqueue *vq = crdev->vq;
	struct scatterlist syscall_type_sg, /* output_msg_sg, input_msg_sg, */ host_fd_sg, ioctl_cmd_sg, session_key_sg,
		session_op_sg, host_return_val_sg, ses_id_sg, crypt_op_sg, src_sg, dst_sg, iv_sg,
	                   *sgs[8];
	unsigned int num_out, num_in, len;
#define MSG_LEN 100
	unsigned int *syscall_type, *ioctl_cmd=NULL;
	long *host_return_val=NULL;
	struct session_op *sess=NULL;
	struct crypt_op *crypt=NULL;
	uint32_t *ses_id=NULL;
	unsigned char *src=NULL,*iv=NULL,*dst=NULL, *session_key=NULL;
	unsigned long flags;

	debug("Entering");

	/**
	 * Allocate all data that will be sent to the host.
	 **/
	/* output_msg = kzalloc(MSG_LEN, GFP_KERNEL);
	input_msg = kzalloc(MSG_LEN, GFP_KERNEL); */
	syscall_type = kzalloc(sizeof(*syscall_type), GFP_KERNEL);
	*syscall_type = VIRTIO_CRYPTODEV_SYSCALL_IOCTL;

	num_out = 0;
	num_in = 0;

	/**
	 *  These are common to all ioctl commands.
	 **/
	sg_init_one(&syscall_type_sg, syscall_type, sizeof(*syscall_type));
	sgs[num_out++] = &syscall_type_sg;
	/* ?? */
	sg_init_one(&host_fd_sg, &crof->host_fd, sizeof(crof->host_fd));
	sgs[num_out++] = &host_fd_sg;

	/**
	 *  Add all the cmd specific sg lists.
	 **/
	//sess = (struct session_op *) arg;
	//crypt = (struct crypt_op *) arg;

	switch (cmd) {
	case CIOCGSESSION:
		debug("CIOCGSESSION");
		
		ioctl_cmd = kzalloc(sizeof(*ioctl_cmd), GFP_KERNEL);
		*ioctl_cmd = CIOCGSESSION;

		sess = kzalloc(sizeof(*sess), GFP_KERNEL);
		ret=copy_from_user(sess, (struct session_op *) arg, sizeof(*sess));
		if(ret)
		{
			debug("It's user session");
			return ret;
		}
		debug("Copied from user");

		session_key = kzalloc(sess->keylen, GFP_KERNEL);

		debug("Allocated key");
		
		ret = copy_from_user(session_key, sess->key, sess->keylen);
		if(ret)
		{
			debug("It's user session key");
			return ret;
		}
		debug("IF its here then i don't know");

		sg_init_one(&ioctl_cmd_sg, ioctl_cmd, sizeof(*ioctl_cmd));
		sgs[num_out++] = &ioctl_cmd_sg;

		sg_init_one(&session_key_sg, session_key, sess->keylen);
		sgs[num_out++] = &session_key_sg;

		sg_init_one(&session_op_sg, sess, sizeof(*sess));
		sgs[num_out + num_in++] = &session_op_sg;
		break;

	case CIOCFSESSION:
		debug("CIOCFSESSION");

		ioctl_cmd = kzalloc(sizeof(*ioctl_cmd), GFP_KERNEL);
		*ioctl_cmd = CIOCFSESSION;

		ses_id = kmalloc(sizeof(uint32_t), GFP_KERNEL);

		ret = copy_from_user(ses_id, (uint32_t *) arg, sizeof(*ses_id));
		if(ret)
		{
			debug("It's user session id");
			return ret;
		}

		sg_init_one(&ioctl_cmd_sg, ioctl_cmd, sizeof(*ioctl_cmd));
		sgs[num_out++] = &ioctl_cmd_sg;

		sg_init_one(&ses_id_sg, ses_id, sizeof(*ses_id));
		sgs[num_out++] = &ses_id_sg;
		break;

	case CIOCCRYPT:
		debug("CIOCCRYPT");

		ioctl_cmd = kzalloc(sizeof(*ioctl_cmd), GFP_KERNEL);
		*ioctl_cmd = CIOCCRYPT;
		
		crypt = kzalloc(sizeof(*crypt), GFP_KERNEL);
		debug("allocated crypt");
		
		if(copy_from_user(crypt, (struct crypt_op *) arg, sizeof(*crypt)))
		{
			debug("Its Black P from the crypt");
			ret = -10;
			goto out;
		}
		debug("got crypt from user");

		src = kzalloc(crypt->len,GFP_KERNEL);
		debug("allocated src");
		
		if(copy_from_user(src, crypt->src, crypt->len))
		{
			debug("Its src from the crypt");
			ret = -10;
			goto out;
		}
		debug("got src from user");

		iv = kzalloc(16,GFP_KERNEL);
		debug("allocated iv");
		
		if(copy_from_user(iv, crypt->iv, 16))
		{
			debug("Its iv from the crypt");
			ret = -10;
			goto out;
		}
		debug("got iv from user");

		dst = kzalloc(crypt->len,GFP_KERNEL);
		debug("allocated dst");

		sg_init_one(&ioctl_cmd_sg, ioctl_cmd, sizeof(*ioctl_cmd));
		sgs[num_out++] = &ioctl_cmd_sg;

		sg_init_one(&crypt_op_sg, &crypt, sizeof(*crypt));
		sgs[num_out++] = &crypt_op_sg;

		sg_init_one(&src_sg, src, crypt->len);
		sgs[num_out++] = &src_sg;
		
		sg_init_one(&iv_sg, iv, 16);
		sgs[num_out++] = &iv_sg;

		sg_init_one(&dst_sg, dst, crypt->len);
		sgs[num_out + num_in++] = &dst_sg;
		
		break;

	default:
		debug("Unsupported ioctl command");

		break;
	}

	host_return_val = kmalloc(sizeof(long), GFP_KERNEL);
	/* Also common for every cmd */
	sg_init_one(&host_return_val_sg, host_return_val, sizeof(*host_return_val));
	sgs[num_out + num_in++] = &host_return_val_sg;

	/**
	 * Wait for the host to process our data.
	 **/
	/* ?? */
	/* ?? Lock ?? */
	spin_lock_irqsave(&crdev->lock, flags);

	err = virtqueue_add_sgs(vq, sgs, num_out, num_in,
	                        &syscall_type_sg, GFP_ATOMIC);
	virtqueue_kick(vq);

	while (virtqueue_get_buf(vq, &len) == NULL)
		/* do nothing */;

	spin_unlock_irqrestore(&crdev->lock, flags);

	if(cmd == CIOCGSESSION)
	{
			/* if(host_return_val)
			{
				debug("ioctl(CIOCGSESSION)");
				return 1;
			} */
			sess->key = ((struct session_op *) arg)->key;
			if(copy_to_user((struct session_op *) arg, sess, sizeof(*sess)))
			{
				debug("its Cioccession return of the key");
				ret = -14;
				goto out;
			}
	}

	else if(cmd == CIOCFSESSION)
	{
			/* if(host_return_val)
			{
				debug("ioctl(CIOCFSESSION)");
				return 1;
			} */
			debug("Finito");
	}
		
	else if(cmd == CIOCCRYPT)
	{
			/* if(host_return_val)
			{
				debug("ioctl(CIOCCRYPT)");
				return 1;
			} */
			if(copy_to_user(((struct crypt_op *) arg)->dst, dst, crypt->len))
			{
				debug("its Cioccrypt");
				ret = -14;
				goto out;
			}
	}

out:
	kfree(dst);
	kfree(src);
	kfree(iv);
	kfree(crypt);
	kfree(sess);
	kfree(session_key);
	kfree(syscall_type);
	kfree(ioctl_cmd);
	kfree(ses_id);
	debug("Leaving");
	if(ret>0)
		ret=0;
	return ret;
}

static ssize_t crypto_chrdev_read(struct file *filp, char __user *usrbuf, 
                                  size_t cnt, loff_t *f_pos)
{
	debug("Entering");
	debug("Leaving");
	return -EINVAL;
}

static struct file_operations crypto_chrdev_fops = 
{
	.owner          = THIS_MODULE,
	.open           = crypto_chrdev_open,
	.release        = crypto_chrdev_release,
	.read           = crypto_chrdev_read,
	.unlocked_ioctl = crypto_chrdev_ioctl,
};

int crypto_chrdev_init(void)
{
	int ret;
	dev_t dev_no;
	unsigned int crypto_minor_cnt = CRYPTO_NR_DEVICES;
	
	debug("Initializing character device...");
	cdev_init(&crypto_chrdev_cdev, &crypto_chrdev_fops);
	crypto_chrdev_cdev.owner = THIS_MODULE;
	
	dev_no = MKDEV(CRYPTO_CHRDEV_MAJOR, 0);
	ret = register_chrdev_region(dev_no, crypto_minor_cnt, "crypto_devs");
	if (ret < 0) {
		debug("failed to register region, ret = %d", ret);
		goto out;
	}
	ret = cdev_add(&crypto_chrdev_cdev, dev_no, crypto_minor_cnt);
	if (ret < 0) {
		debug("failed to add character device");
		goto out_with_chrdev_region;
	}

	debug("Completed successfully");
	return 0;

out_with_chrdev_region:
	unregister_chrdev_region(dev_no, crypto_minor_cnt);
out:
	return ret;
}

void crypto_chrdev_destroy(void)
{
	dev_t dev_no;
	unsigned int crypto_minor_cnt = CRYPTO_NR_DEVICES;

	debug("entering");
	dev_no = MKDEV(CRYPTO_CHRDEV_MAJOR, 0);
	cdev_del(&crypto_chrdev_cdev);
	unregister_chrdev_region(dev_no, crypto_minor_cnt);
	debug("leaving");
}
