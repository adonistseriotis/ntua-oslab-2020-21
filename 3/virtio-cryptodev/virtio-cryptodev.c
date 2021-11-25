/*
 * Virtio Cryptodev Device
 *
 * Implementation of virtio-cryptodev qemu backend device.
 *
 * Dimitris Siakavaras <jimsiak@cslab.ece.ntua.gr>
 * Stefanos Gerangelos <sgerag@cslab.ece.ntua.gr> 
 * Konstantinos Papazafeiropoulos <kpapazaf@cslab.ece.ntua.gr>
 *
 */

#include "qemu/osdep.h"
#include "qemu/iov.h"
#include "hw/qdev.h"
#include "hw/virtio/virtio.h"
#include "standard-headers/linux/virtio_ids.h"
#include "hw/virtio/virtio-cryptodev.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <crypto/cryptodev.h>

static uint64_t get_features(VirtIODevice *vdev, uint64_t features,
                             Error **errp)
{
    DEBUG_IN();
    return features;
}

static void get_config(VirtIODevice *vdev, uint8_t *config_data)
{
    DEBUG_IN();
}

static void set_config(VirtIODevice *vdev, const uint8_t *config_data)
{
    DEBUG_IN();
}

static void set_status(VirtIODevice *vdev, uint8_t status)
{
    DEBUG_IN();
}

static void vser_reset(VirtIODevice *vdev)
{
    DEBUG_IN();
}

static void vq_handle_output(VirtIODevice *vdev, VirtQueue *vq)
{
    VirtQueueElement *elem;
    unsigned int *syscall_type, *ioctl_cmd;
    struct crypt_op *crypt;
    struct session_op *sess;
    long *host_return_val;
    unsigned char *session_key,*src,*iv,*dst;
    uint32_t *ses_id;

    int *cfd, *openfd; 

    DEBUG_IN();

    elem = virtqueue_pop(vq, sizeof(VirtQueueElement));
    if (!elem) {
        DEBUG("No item to pop from VQ :(");
        return;
    } 

    DEBUG("I have got an item from VQ :)");

    syscall_type = elem->out_sg[0].iov_base;
    switch (*syscall_type) {
    case VIRTIO_CRYPTODEV_SYSCALL_TYPE_OPEN:
        DEBUG("VIRTIO_CRYPTODEV_SYSCALL_TYPE_OPEN");
        /* ?? */
        cfd = elem->in_sg[0].iov_base;
        *cfd = open("/dev/crypto",O_RDWR);

        if(*cfd < 0)
        {
            DEBUG("SOMETHING'S WRONG WITH FD");
        }
        break;

    case VIRTIO_CRYPTODEV_SYSCALL_TYPE_CLOSE:
        DEBUG("VIRTIO_CRYPTODEV_SYSCALL_TYPE_CLOSE");
        /* ?? */
        openfd = elem->out_sg[1].iov_base;
        if(close(*openfd) < 0)
            DEBUG("ERROR ON CLOSE FUNC")
        break;

    case VIRTIO_CRYPTODEV_SYSCALL_TYPE_IOCTL:
        DEBUG("VIRTIO_CRYPTODEV_SYSCALL_TYPE_IOCTL");
        /* ?? */
        cfd = elem->out_sg[1].iov_base;
        ioctl_cmd = elem->out_sg[2].iov_base;
        DEBUG("OK THROUGH HERE");
        DEBUG("OK HERE ALSO");
        switch (*ioctl_cmd)
        {
        case CIOCGSESSION:
            DEBUG("STARTING SESSION");
            host_return_val = elem->in_sg[1].iov_base;
            session_key = elem->out_sg[3].iov_base;
            sess = elem->in_sg[0].iov_base;

            sess->key = session_key;
            if ((*host_return_val = ioctl(*cfd, *ioctl_cmd, sess))) {
                DEBUG("FAILED IOCTL SSESS");
            }
            DEBUG("END OF SESSION START");
            break;
        
        case CIOCFSESSION:
            DEBUG("START OF FSESSION");
            host_return_val = elem->in_sg[0].iov_base;
            ses_id = elem->out_sg[3].iov_base;
            if((*host_return_val = ioctl(*cfd, *ioctl_cmd,ses_id)))
            {
                DEBUG("FAILED IOCTL FSESS");
            }
            DEBUG("END OF FSESSION");
            break;
        
        case CIOCCRYPT:
            DEBUG("IN CCRYPT");
            host_return_val = elem->in_sg[1].iov_base;
            crypt = elem->out_sg[3].iov_base;
            src = elem->out_sg[4].iov_base;
            iv = elem->out_sg[5].iov_base;
            dst = elem->in_sg[0].iov_base;

            crypt->src = src;
            crypt->dst = dst;
            crypt->iv = iv;

            *host_return_val = ioctl(*cfd, *ioctl_cmd,crypt);

            if(*host_return_val)
            {
                DEBUG("FAILED IOCT CRYPTIOGDsv");
            }
            DEBUG("LEAVING CCRYPT");
            break;

        default:
            DEBUG("You must have been mistaken mate.");
            break;
        }

        break;

    default:
        DEBUG("Unknown syscall_type");
        break;
    }

    virtqueue_push(vq, elem, 0);
    virtio_notify(vdev, vq);
    g_free(elem);
}

static void virtio_cryptodev_realize(DeviceState *dev, Error **errp)
{
    VirtIODevice *vdev = VIRTIO_DEVICE(dev);

    DEBUG_IN();

    virtio_init(vdev, "virtio-cryptodev", VIRTIO_ID_CRYPTODEV, 0);
    virtio_add_queue(vdev, 128, vq_handle_output);
}

static void virtio_cryptodev_unrealize(DeviceState *dev, Error **errp)
{
    DEBUG_IN();
}

static Property virtio_cryptodev_properties[] = {
    DEFINE_PROP_END_OF_LIST(),
};

static void virtio_cryptodev_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    VirtioDeviceClass *k = VIRTIO_DEVICE_CLASS(klass);

    DEBUG_IN();
    dc->props = virtio_cryptodev_properties;
    set_bit(DEVICE_CATEGORY_INPUT, dc->categories);

    k->realize = virtio_cryptodev_realize;
    k->unrealize = virtio_cryptodev_unrealize;
    k->get_features = get_features;
    k->get_config = get_config;
    k->set_config = set_config;
    k->set_status = set_status;
    k->reset = vser_reset;
}

static const TypeInfo virtio_cryptodev_info = {
    .name          = TYPE_VIRTIO_CRYPTODEV,
    .parent        = TYPE_VIRTIO_DEVICE,
    .instance_size = sizeof(VirtCryptodev),
    .class_init    = virtio_cryptodev_class_init,
};

static void virtio_cryptodev_register_types(void)
{
    type_register_static(&virtio_cryptodev_info);
}

type_init(virtio_cryptodev_register_types)
